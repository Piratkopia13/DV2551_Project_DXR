#include "pch.h"
#include "DXR.h"
#include "DX12Renderer.h"
#include "D3DUtils.h"
#include "DX12VertexBuffer.h"
#include "DXILShaderCompiler.h"

using namespace DirectX;

const D3D12_HEAP_PROPERTIES DXR::sUploadHeapProperties = {
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0,
};

const D3D12_HEAP_PROPERTIES DXR::sDefaultHeapProps = {
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0
};

DXR::DXR(DX12Renderer* renderer)
	: m_renderer(renderer)
{
}

DXR::~DXR() {
	SafeDelete(m_vb);
}

void DXR::init(ID3D12GraphicsCommandList4* cmdList) {
	createAccelerationStructures(cmdList);
	createDxrGlobalRootSignature();
	createRaytracingPSO();
	createShaderResources();
	createShaderTables();
}

void DXR::doTheRays(ID3D12GraphicsCommandList4* cmdList) {

	//Set constant buffer descriptor heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_rtDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	// Let's raytrace
	D3DUtils::setResourceTransitionBarrier(cmdList, m_rtOutputUAV.resource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width = m_renderer->getWindow()->getWindowWidth();
	raytraceDesc.Height = m_renderer->getWindow()->getWindowHeight();
	raytraceDesc.Depth = 1;

	//set shader tables
	raytraceDesc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable.Resource->GetGPUVirtualAddress();
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable.SizeInBytes;

	raytraceDesc.MissShaderTable.StartAddress = m_missShaderTable.Resource->GetGPUVirtualAddress();
	raytraceDesc.MissShaderTable.StrideInBytes = m_missShaderTable.StrideInBytes;
	raytraceDesc.MissShaderTable.SizeInBytes = m_missShaderTable.SizeInBytes;

	raytraceDesc.HitGroupTable.StartAddress = m_hitGroupShaderTable.Resource->GetGPUVirtualAddress();
	raytraceDesc.HitGroupTable.StrideInBytes = m_hitGroupShaderTable.StrideInBytes;
	raytraceDesc.HitGroupTable.SizeInBytes = m_hitGroupShaderTable.SizeInBytes;

	// Bind the empty root signature
	cmdList->SetComputeRootSignature(m_dxrGlobalRootSignature.Get());

	// Set float RedChannel; in global root signature
	float redColor = 1.0f;
	cmdList->SetComputeRoot32BitConstant(DXRGlobalRootParam::FLOAT_RED_CHANNEL, *reinterpret_cast<UINT*>(&redColor), 0);
	// Set acceleration structure
	cmdList->SetComputeRootShaderResourceView(DXRGlobalRootParam::SRV_ACCELERATION_STRUCTURE, m_DXR_TopBuffers.result->GetGPUVirtualAddress());
	// Set vertex buffer
	cmdList->SetComputeRootShaderResourceView(DXRGlobalRootParam::SRV_VERTEX_BUFFER, m_vb->getBuffer()->GetGPUVirtualAddress());

	// Dispatch
	cmdList->SetPipelineState1(m_rtPipelineState.Get());
	cmdList->DispatchRays(&raytraceDesc);

}

void DXR::copyOutputTo(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* target) {

	// Copy the results to the back-buffer
	D3DUtils::setResourceTransitionBarrier(cmdList, target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	D3DUtils::setResourceTransitionBarrier(cmdList, m_rtOutputUAV.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	cmdList->CopyResource(target, m_rtOutputUAV.resource.Get());
	D3DUtils::setResourceTransitionBarrier(cmdList, target, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

}

void DXR::updateTLAS(ID3D12GraphicsCommandList4* cmdList, std::function<XMFLOAT3X4(int)> instanceTransform, UINT instanceCount) {
	
	auto blas = m_DXR_BottomBuffers.result;

	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = instanceCount;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	m_DXR_TopBuffers.instanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);

	for (UINT i = 0; i < instanceCount; i++) {

		pInstanceDesc->InstanceID = i;                            // exposed to the shader via InstanceID()
		pInstanceDesc->InstanceContributionToHitGroupIndex = i;   // offset inside the shader-table. we only have a single geometry, so the offset 0
		pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

		// apply transform from lambda function
		XMFLOAT3X4 m = instanceTransform(i);
		memcpy(pInstanceDesc->Transform, &m, sizeof(pInstanceDesc->Transform));

		pInstanceDesc->AccelerationStructure = blas->GetGPUVirtualAddress();
		pInstanceDesc->InstanceMask = 0xFF;

		pInstanceDesc++;
	}
	// Unmap
	m_DXR_TopBuffers.instanceDesc->Unmap(0, nullptr);

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = m_DXR_TopBuffers.instanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = m_DXR_TopBuffers.result->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = m_DXR_TopBuffers.scratch->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// UAV barrier needed before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_DXR_TopBuffers.result.Get();
	cmdList->ResourceBarrier(1, &uavBarrier);

}

void DXR::createAccelerationStructures(ID3D12GraphicsCommandList4* cmdList) {
	const float vertices[] = {
		0,          1,  0,		0, 0, -1,	// Vertex and normal
		0.866f,  -0.5f, 0,		0, 0, -1,
		-0.866f, -0.5f, 0,		0, 0, -1,

		0,       -0.5f,  0,		0, 0, -1,	// Vertex and normal
		0.866f,  -1.5f, 0,		0, 0, -1,
		-0.866f, -1.5f, 0,		0, 0, -1
	};

	m_vb = new DX12VertexBuffer(sizeof(vertices), VertexBuffer::DATA_USAGE::STATIC, m_renderer); //TODO: fix mem leak
	m_vb->setData(vertices, sizeof(vertices), 0);
	m_vb->setVertexStride(sizeof(float) * 6);

	createBLAS(cmdList, m_vb);
	createTLAS(cmdList, m_DXR_BottomBuffers.result.Get());
}

void DXR::createShaderResources() {

	D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
	heapDescriptorDesc.NumDescriptors = 10;
	heapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_renderer->getDevice()->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_rtDescriptorHeap));

	// Create the output resource. The dimensions and format should match the swap-chain
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB formats can't be used with UAVs. We will convert to sRGB ourselves in the shader
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = m_renderer->getWindow()->getWindowWidth();
	resDesc.Height = m_renderer->getWindow()->getWindowHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	m_renderer->getDevice()->CreateCommittedResource(&sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_rtOutputUAV.resource)); // Starting as copy-source to simplify onFrameRender()
	m_rtOutputUAV.resource->SetName(L"RTOutputUAV");

	// Create the UAV. Based on the root signature we created it should be the first entry
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE outputUAV_CPU = m_rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_renderer->getDevice()->CreateUnorderedAccessView(m_rtOutputUAV.resource.Get(), nullptr, &uavDesc, outputUAV_CPU);
	m_rtOutputUAV.gpuHandle = m_rtDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// Create the TLAS SRV right after the UAV. Note that we are using a different SRV desc here
	/*D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = m_DXR_TopBuffers.result->GetGPUVirtualAddress();*/

	//D3D12_CPU_DESCRIPTOR_HANDLE rtAcceleration_CPU = m_rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//rtAcceleration_CPU.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//m_device->CreateShaderResourceView(nullptr, &srvDesc, rtAcceleration_CPU);
	/*m_rtAcceleration_GPU = m_rtDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_rtAcceleration_GPU.ptr += m_renderer->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);*/
}

void DXR::createShaderTables() {

	// TODO: Shader table builder

	ID3D12StateObjectProperties* rtsoProps = nullptr;
	if (SUCCEEDED(m_rtPipelineState->QueryInterface(IID_PPV_ARGS(&rtsoProps)))) {
		//raygen
		{

			struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) RAY_GEN_SHADER_TABLE_DATA {
				unsigned char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
				UINT64 RTVDescriptor;
			} tableData;

			memcpy(tableData.ShaderIdentifier, rtsoProps->GetShaderIdentifier(m_rayGenName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			tableData.RTVDescriptor = m_rtOutputUAV.gpuHandle.ptr;

			//how big is the biggest?
			union MaxSize {
				RAY_GEN_SHADER_TABLE_DATA data0;
			};

			m_rayGenShaderTable.StrideInBytes = sizeof(MaxSize);
			m_rayGenShaderTable.SizeInBytes = m_rayGenShaderTable.StrideInBytes * 1; //<-- only one for now...
			m_rayGenShaderTable.Resource = D3DUtils::createBuffer(m_renderer->getDevice(), m_rayGenShaderTable.SizeInBytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, sUploadHeapProperties);

			// Map the buffer
			void* pData;
			m_rayGenShaderTable.Resource->Map(0, nullptr, &pData);
			memcpy(pData, &tableData, sizeof(tableData));
			m_rayGenShaderTable.Resource->Unmap(0, nullptr);
		}

		//miss
		{

			struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) MISS_SHADER_TABLE_DATA {
				unsigned char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
			} tableData;

			memcpy(tableData.ShaderIdentifier, rtsoProps->GetShaderIdentifier(m_missName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

			//how big is the biggest?
			union MaxSize {
				MISS_SHADER_TABLE_DATA data0;
			};

			m_missShaderTable.StrideInBytes = sizeof(MaxSize);
			m_missShaderTable.SizeInBytes = m_missShaderTable.StrideInBytes * 1; //<-- only one for now...
			m_missShaderTable.Resource = D3DUtils::createBuffer(m_renderer->getDevice(), m_missShaderTable.SizeInBytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, sUploadHeapProperties);

			// Map the buffer
			void* pData;
			m_missShaderTable.Resource->Map(0, nullptr, &pData);
			memcpy(pData, &tableData, sizeof(tableData));
			m_missShaderTable.Resource->Unmap(0, nullptr);
		}

		//hit program
		{

			struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) HIT_GROUP_SHADER_TABLE_DATA {
				unsigned char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
				float ShaderTableColor[3];
			} tableData[3]{};

			for (int i = 0; i < 3; i++) {
				memcpy(tableData[i].ShaderIdentifier, rtsoProps->GetShaderIdentifier(m_hitGroupName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				tableData[i].ShaderTableColor[i] = 1;
			}

			//how big is the biggest?
			union MaxSize {
				HIT_GROUP_SHADER_TABLE_DATA data0;
			};

			int instanceCount = 3;
			m_hitGroupShaderTable.StrideInBytes = sizeof(MaxSize);
			m_hitGroupShaderTable.SizeInBytes = m_hitGroupShaderTable.StrideInBytes * instanceCount;
			m_hitGroupShaderTable.Resource = D3DUtils::createBuffer(m_renderer->getDevice(), m_hitGroupShaderTable.SizeInBytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, sUploadHeapProperties);

			// Map the buffer
			void* pData;
			m_hitGroupShaderTable.Resource->Map(0, nullptr, &pData);
			memcpy(pData, &tableData, sizeof(tableData));
			m_hitGroupShaderTable.Resource->Unmap(0, nullptr);
		}

		rtsoProps->Release();
	}

}

void DXR::createBLAS(ID3D12GraphicsCommandList4* cmdList, DX12VertexBuffer* vb) {

	// TODO: Allow multiple vertex buffers
	// TODO: Allow for BLAS updates (multiple calls) in place?

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc[1] = {};
	geomDesc[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc[0].Triangles.VertexBuffer.StartAddress = vb->getBuffer()->GetGPUVirtualAddress();
	geomDesc[0].Triangles.VertexBuffer.StrideInBytes = vb->getVertexStride();/* sizeof(float) * 3 * 2;*/
	geomDesc[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geomDesc[0].Triangles.VertexCount = vb->getVertexCount();
	geomDesc[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = ARRAYSIZE(geomDesc);
	inputs.pGeometryDescs = geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	m_renderer->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state

	m_DXR_BottomBuffers.scratch = D3DUtils::createBuffer(m_renderer->getDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, sDefaultHeapProps);
	m_DXR_BottomBuffers.result = D3DUtils::createBuffer(m_renderer->getDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, sDefaultHeapProps);

	// Create the bottom-level AS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = m_DXR_BottomBuffers.result->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = m_DXR_BottomBuffers.scratch->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_DXR_BottomBuffers.result.Get();
	cmdList->ResourceBarrier(1, &uavBarrier);

}

void DXR::createTLAS(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource1* blas) {

	int instanceCount = 3;

	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = instanceCount;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	// on first call, create the buffer
	if (m_DXR_TopBuffers.instanceDesc == nullptr) {
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		m_renderer->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		// Create the buffers
		if (m_DXR_TopBuffers.scratch == nullptr) {
			m_DXR_TopBuffers.scratch = D3DUtils::createBuffer(m_renderer->getDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, sDefaultHeapProps);
		}

		if (m_DXR_TopBuffers.result == nullptr) {
			m_DXR_TopBuffers.result = D3DUtils::createBuffer(m_renderer->getDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, sDefaultHeapProps);
		}

		m_DXR_TopBuffers.instanceDesc = D3DUtils::createBuffer(m_renderer->getDevice(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, sUploadHeapProperties);
	}

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	m_DXR_TopBuffers.instanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);

	for (int i = 0; i < instanceCount; i++) {

		pInstanceDesc->InstanceID = i;                            // exposed to the shader via InstanceID()
		pInstanceDesc->InstanceContributionToHitGroupIndex = i;   // offset inside the shader-table. we only have a single geometry, so the offset 0
		pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

		//apply transform
		XMFLOAT3X4 m;
		XMStoreFloat3x4(&m, XMMatrixIdentity());
		memcpy(pInstanceDesc->Transform, &m, sizeof(pInstanceDesc->Transform));

		pInstanceDesc->AccelerationStructure = blas->GetGPUVirtualAddress();
		pInstanceDesc->InstanceMask = 0xFF;

		pInstanceDesc++;
	}
	// Unmap
	m_DXR_TopBuffers.instanceDesc->Unmap(0, nullptr);

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = m_DXR_TopBuffers.instanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = m_DXR_TopBuffers.result->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = m_DXR_TopBuffers.scratch->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// UAV barrier needed before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_DXR_TopBuffers.result.Get();
	cmdList->ResourceBarrier(1, &uavBarrier);
}

void DXR::createRaytracingPSO() {
	// TODO: PSO builder

	D3D12_STATE_SUBOBJECT soMem[100]{};
	UINT numSubobjects = 0;
	auto nextSuboject = [&]() {
		return soMem + numSubobjects++;
	};

	DXILShaderCompiler dxilCompiler;
	dxilCompiler.init();

	DXILShaderCompiler::Desc shaderDesc;

	//D3DCOMPILE_IEEE_STRICTNESS
	shaderDesc.CompileArguments.push_back(L"/Gis");

	//Vertex shader
	std::string s = m_renderer->getShaderPath() + "RayTracingShaders.hlsl";
	std::wstring stemp = std::wstring(s.begin(), s.end());
	shaderDesc.FilePath = stemp.c_str();
	shaderDesc.EntryPoint = L"";
	shaderDesc.TargetProfile = L"lib_6_3";

	IDxcBlob* pShaders = nullptr;
	ThrowIfFailed(dxilCompiler.compileFromFile(&shaderDesc, &pShaders));

	//Init DXIL subobject
	D3D12_EXPORT_DESC dxilExports[] = {
		m_rayGenName, nullptr, D3D12_EXPORT_FLAG_NONE,
		m_closestHitName, nullptr, D3D12_EXPORT_FLAG_NONE,
		m_missName, nullptr, D3D12_EXPORT_FLAG_NONE,
	};
	D3D12_DXIL_LIBRARY_DESC dxilLibraryDesc;
	dxilLibraryDesc.DXILLibrary.pShaderBytecode = pShaders->GetBufferPointer();
	dxilLibraryDesc.DXILLibrary.BytecodeLength = pShaders->GetBufferSize();
	dxilLibraryDesc.pExports = dxilExports;
	dxilLibraryDesc.NumExports = _countof(dxilExports);

	D3D12_STATE_SUBOBJECT* soDXIL = nextSuboject();
	soDXIL->Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	soDXIL->pDesc = &dxilLibraryDesc;

	//Init hit group
	D3D12_HIT_GROUP_DESC hitGroupDesc;
	hitGroupDesc.AnyHitShaderImport = nullptr;
	hitGroupDesc.ClosestHitShaderImport = m_closestHitName;
	hitGroupDesc.HitGroupExport = m_hitGroupName;
	hitGroupDesc.IntersectionShaderImport = nullptr;
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

	D3D12_STATE_SUBOBJECT* soHitGroup = nextSuboject();
	soHitGroup->Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	soHitGroup->pDesc = &hitGroupDesc;

	//Init rayGen local root signature
	ID3D12RootSignature* rayGenLocalRoot = createRayGenLocalRootSignature();
	D3D12_STATE_SUBOBJECT* soRayGenLocalRoot = nextSuboject();
	soRayGenLocalRoot->Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	soRayGenLocalRoot->pDesc = &rayGenLocalRoot;

	//Bind local root signature to rayGen shader
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rayGenLocalRootAssociation;
	LPCWSTR rayGenLocalRootAssociationShaderNames[] = { m_rayGenName };
	rayGenLocalRootAssociation.pExports = rayGenLocalRootAssociationShaderNames;
	rayGenLocalRootAssociation.NumExports = _countof(rayGenLocalRootAssociationShaderNames);
	rayGenLocalRootAssociation.pSubobjectToAssociate = soRayGenLocalRoot; //<-- address to local root subobject

	D3D12_STATE_SUBOBJECT* soRayGenLocalRootAssociation = nextSuboject();
	soRayGenLocalRootAssociation->Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	soRayGenLocalRootAssociation->pDesc = &rayGenLocalRootAssociation;


	//Init hit group local root signature
	ID3D12RootSignature* hitGroupLocalRoot = createHitGroupLocalRootSignature();
	D3D12_STATE_SUBOBJECT* soHitGroupLocalRoot = nextSuboject();
	soHitGroupLocalRoot->Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	soHitGroupLocalRoot->pDesc = &hitGroupLocalRoot;


	//Bind local root signature to hit group shaders
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION hitGroupLocalRootAssociation;
	LPCWSTR hitGroupLocalRootAssociationShaderNames[] = { m_closestHitName };
	hitGroupLocalRootAssociation.pExports = hitGroupLocalRootAssociationShaderNames;
	hitGroupLocalRootAssociation.NumExports = _countof(hitGroupLocalRootAssociationShaderNames);
	hitGroupLocalRootAssociation.pSubobjectToAssociate = soHitGroupLocalRoot; //<-- address to local root subobject

	D3D12_STATE_SUBOBJECT* soHitGroupLocalRootAssociation = nextSuboject();
	soHitGroupLocalRootAssociation->Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	soHitGroupLocalRootAssociation->pDesc = &hitGroupLocalRootAssociation;


	//Init miss local root signature
	ID3D12RootSignature* missLocalRoot = createMissLocalRootSignature();
	D3D12_STATE_SUBOBJECT* soMissLocalRoot = nextSuboject();
	soMissLocalRoot->Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	soMissLocalRoot->pDesc = &missLocalRoot;


	//Bind local root signature to miss shader
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION missLocalRootAssociation;
	LPCWSTR missLocalRootAssociationShaderNames[] = { m_missName };
	missLocalRootAssociation.pExports = missLocalRootAssociationShaderNames;
	missLocalRootAssociation.NumExports = _countof(missLocalRootAssociationShaderNames);
	missLocalRootAssociation.pSubobjectToAssociate = soMissLocalRoot; //<-- address to local root subobject

	D3D12_STATE_SUBOBJECT* soMissLocalRootAssociation = nextSuboject();
	soMissLocalRootAssociation->Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	soMissLocalRootAssociation->pDesc = &missLocalRootAssociation;


	//Init shader config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2;
	shaderConfig.MaxPayloadSizeInBytes = sizeof(float) * 4 + sizeof(UINT) * 1;

	D3D12_STATE_SUBOBJECT* soShaderConfig = nextSuboject();
	soShaderConfig->Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	soShaderConfig->pDesc = &shaderConfig;

	//Bind the payload size to the programs
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociation;
	const WCHAR* shaderNamesToConfig[] = { m_missName, m_closestHitName, m_rayGenName };
	shaderConfigAssociation.pExports = shaderNamesToConfig;
	shaderConfigAssociation.NumExports = _countof(shaderNamesToConfig);
	shaderConfigAssociation.pSubobjectToAssociate = soShaderConfig; //<-- address to shader config subobject

	D3D12_STATE_SUBOBJECT* soShaderConfigAssociation = nextSuboject();
	soShaderConfigAssociation->Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	soShaderConfigAssociation->pDesc = &shaderConfigAssociation;


	//Init pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = 2;

	D3D12_STATE_SUBOBJECT* soPipelineConfig = nextSuboject();
	soPipelineConfig->Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	soPipelineConfig->pDesc = &pipelineConfig;

	ID3D12RootSignature* groot = m_dxrGlobalRootSignature.Get();

	D3D12_STATE_SUBOBJECT* soGlobalRoot = nextSuboject();
	soGlobalRoot->Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	soGlobalRoot->pDesc = &groot;


	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = numSubobjects;
	desc.pSubobjects = soMem;
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ThrowIfFailed(m_renderer->getDevice()->CreateStateObject(&desc, IID_PPV_ARGS(&m_rtPipelineState)));
}

void DXR::createDxrGlobalRootSignature() {

	//D3D12_DESCRIPTOR_RANGE range[1]{};
	D3D12_ROOT_PARAMETER rootParams[DXRGlobalRootParam::SIZE]{};

	//float RedChannel
	rootParams[DXRGlobalRootParam::FLOAT_RED_CHANNEL].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[DXRGlobalRootParam::FLOAT_RED_CHANNEL].Constants.RegisterSpace = 0;
	rootParams[DXRGlobalRootParam::FLOAT_RED_CHANNEL].Constants.ShaderRegister = 0;
	rootParams[DXRGlobalRootParam::FLOAT_RED_CHANNEL].Constants.Num32BitValues = 2; // ??

	// gRtScene
	rootParams[DXRGlobalRootParam::SRV_ACCELERATION_STRUCTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[DXRGlobalRootParam::SRV_ACCELERATION_STRUCTURE].Descriptor.ShaderRegister = 0;
	rootParams[DXRGlobalRootParam::SRV_ACCELERATION_STRUCTURE].Descriptor.RegisterSpace = 0;

	// Vertex buffer
	rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].Descriptor.ShaderRegister = 1;
	rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].Descriptor.RegisterSpace = 0;

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(rootParams);
	desc.pParameters = rootParams;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ID3DBlob* sigBlob;
	ID3DBlob* errorBlob;
	ThrowIfBlobError(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob), errorBlob);
	m_renderer->getDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_dxrGlobalRootSignature));
	m_dxrGlobalRootSignature->SetName(L"dxrGlobal");

}

ID3D12RootSignature* DXR::createRayGenLocalRootSignature() {
	D3D12_DESCRIPTOR_RANGE range[1]{};
	D3D12_ROOT_PARAMETER rootParams[1]{};

	// lOutput
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].RegisterSpace = 0;
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[DXRRayGenRootParam::DT_UAV_OUTPUT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[DXRRayGenRootParam::DT_UAV_OUTPUT].DescriptorTable.NumDescriptorRanges = _countof(range);
	rootParams[DXRRayGenRootParam::DT_UAV_OUTPUT].DescriptorTable.pDescriptorRanges = range;

	// Create the desc
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(rootParams);
	desc.pParameters = rootParams;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;


	ID3DBlob* sigBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	ThrowIfBlobError(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob), errorBlob);
	ID3D12RootSignature* pRootSig;
	ThrowIfFailed(m_renderer->getDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig)));
	pRootSig->SetName(L"RayGenLocal");

	return pRootSig;
}

ID3D12RootSignature* DXR::createHitGroupLocalRootSignature() {
	D3D12_ROOT_PARAMETER rootParams[1]{};

	//float3 ShaderTableColor;
	rootParams[DXRHitGroupRootParam::FLOAT3_SHADER_TABLE_COLOR].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[DXRHitGroupRootParam::FLOAT3_SHADER_TABLE_COLOR].Constants.RegisterSpace = 1;
	rootParams[DXRHitGroupRootParam::FLOAT3_SHADER_TABLE_COLOR].Constants.ShaderRegister = 0;
	rootParams[DXRHitGroupRootParam::FLOAT3_SHADER_TABLE_COLOR].Constants.Num32BitValues = 3;

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(rootParams);
	desc.pParameters = rootParams;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;


	ID3DBlob* sigBlob;
	ID3DBlob* errorBlob;
	ThrowIfBlobError(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob), errorBlob);
	ID3D12RootSignature* rootSig;
	m_renderer->getDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
	rootSig->SetName(L"HitGroupLocal");

	return rootSig;
}

ID3D12RootSignature* DXR::createMissLocalRootSignature() {
	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = 0;
	desc.pParameters = nullptr;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	ID3DBlob* sigBlob;
	ID3DBlob* errorBlob;
	ThrowIfBlobError(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob), errorBlob);
	ID3D12RootSignature* pRootSig;
	m_renderer->getDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
	pRootSig->SetName(L"MissLocal");

	return pRootSig;
}
