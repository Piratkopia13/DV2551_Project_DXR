#include "pch.h"
#include "DXR.h"
#include "DX12Renderer.h"
#include "DX12VertexBuffer.h"
#include "DXILShaderCompiler.h"
#include "DX12ConstantBuffer.h"
#include "DX12Texture2D.h"
#include "DX12Mesh.h"
#include "../Core/Camera.h"
#include "../Core/CameraController.h"
#include "../Utils/Input.h"

using namespace DirectX;

DXR::DXR(DX12Renderer* renderer)
	: m_renderer(renderer)
	, m_updateTLAS(false)
	, m_updateBLAS(false)
	, m_camera(nullptr)
	, m_meshes(nullptr)
	, m_skyboxTexture(nullptr)
{
}

DXR::~DXR() {
	//SafeDelete(m_vb);
}

void DXR::init(ID3D12GraphicsCommandList4* cmdList) {
	createAccelerationStructures(cmdList);
	createDxrGlobalRootSignature();
	createRaytracingPSO();
	/*createShaderResources();
	createShaderTables();*/
}

void DXR::updateAS(ID3D12GraphicsCommandList4* cmdList) {
	if (m_updateBLAS) {
		createShaderResources();
		createShaderTables();
		createBLAS(cmdList, m_newInPlace);
		m_updateBLAS = false;
	}
	if (m_updateTLAS) {
		createTLAS(cmdList, m_newInstanceTransform);
		m_updateTLAS = false;
	}
	m_numMeshesChanged = false;
}

void DXR::doTheRays(ID3D12GraphicsCommandList4* cmdList) {

	assert(m_meshes != nullptr); // Meshes not set

	// Update constant buffers
	if (m_camera) {
		m_sceneCBData->cameraPosition = m_camera->getPositionF3();
		m_sceneCBData->projectionToWorld = m_camera->getInvProjMatrix() * m_camera->getInvViewMatrix();
		m_sceneCB->setData(m_sceneCBData, 0);
	}

	m_rayGenCBData.frameCount++;
	m_rayGenSettingsCB->setData(&m_rayGenCBData, 0);
	m_rayGenSettingsCB->forceUpdate(0);


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

	// Bind the global root signature
	cmdList->SetComputeRootSignature(m_dxrGlobalRootSignature.Get());

	// Set float RedChannel; in global root signature
	float redColor = 1.0f;
	cmdList->SetComputeRoot32BitConstant(DXRGlobalRootParam::FLOAT_RED_CHANNEL, *reinterpret_cast<UINT*>(&redColor), 0);
	// Set acceleration structure
	cmdList->SetComputeRootShaderResourceView(DXRGlobalRootParam::SRV_ACCELERATION_STRUCTURE, m_DXR_TopBuffers.result->GetGPUVirtualAddress());
	// Set scene constant buffer
	cmdList->SetComputeRootConstantBufferView(DXRGlobalRootParam::CBV_SCENE_BUFFER, m_sceneCB->getBuffer(m_renderer->getFrameIndex())->GetGPUVirtualAddress());

	// Dispatch
	cmdList->SetPipelineState1(m_rtPipelineState.Get());
	cmdList->DispatchRays(&raytraceDesc);

}

void DXR::copyOutputTo(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* target) {

	// Copy the results to the back-buffer
	D3DUtils::setResourceTransitionBarrier(cmdList, target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	D3DUtils::setResourceTransitionBarrier(cmdList, m_rtOutputUAV.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	cmdList->CopyResource(target, m_rtOutputUAV.resource.Get());
	D3DUtils::setResourceTransitionBarrier(cmdList, target, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void DXR::setMeshes(const std::vector<std::unique_ptr<DX12Mesh>>& meshes) {
	if (m_meshes == nullptr || meshes.size() != m_meshes->size())
		m_numMeshesChanged = true;
	m_meshes = &meshes;
	m_updateBLAS = true;
	m_newInPlace = false;
}

void DXR::setSkyboxTexture(DX12Texture2D* texture) {
	m_skyboxTexture = texture;
	m_skyboxGPUDescHandle = m_skyboxTexture->getGpuDescHandle();
}

void DXR::updateBLASnextFrame(bool inPlace) {
	m_updateBLAS = true;
	m_newInPlace = inPlace;
}

void DXR::updateTLASnextFrame(std::function<XMFLOAT3X4(int)> instanceTransform) {
	m_updateTLAS = true;
	m_newInstanceTransform = instanceTransform;
}

void DXR::useCamera(Camera* camera) {
	m_camera = camera;
}

void DXR::reloadShaders() {
	m_localSignatureRayGen.Reset();
	m_localSignatureMiss.Reset();
	m_localSignatureHitGroup.Reset();
	m_rtPipelineState.Reset();
	createRaytracingPSO();
	createShaderTables();
}

int& DXR::getRTFlags() {
	return m_rayGenCBData.flags;
}

float& DXR::getAORadius() {
	return m_rayGenCBData.AORadius;
}

UINT& DXR::getNumAORays() {
	return m_rayGenCBData.numAORays;
}

UINT& DXR::getNumGISamples() {
	return m_rayGenCBData.GISamples;
}

UINT& DXR::getNumGIBounces() {
	return m_rayGenCBData.GIBounces;
}

void DXR::createAccelerationStructures(ID3D12GraphicsCommandList4* cmdList) {
	createBLAS(cmdList);
	createTLAS(cmdList);
}

void DXR::createShaderResources() {

	m_rtDescriptorHeap.Reset();
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
	m_renderer->getDevice()->CreateCommittedResource(&D3DUtils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_rtOutputUAV.resource)); // Starting as copy-source to simplify onFrameRender()
	m_rtOutputUAV.resource->SetName(L"RTOutputUAV");

	// Create the UAV. Based on the root signature we created it should be the first entry
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_rtDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// Create a view for the output UAV
	m_renderer->getDevice()->CreateUnorderedAccessView(m_rtOutputUAV.resource.Get(), nullptr, &uavDesc, cpuHandle);
	m_rtOutputUAV.gpuHandle = gpuHandle;

	// Create a view for each mesh textures
	auto incr = m_renderer->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_rtMeshHandles.clear();
	for (auto& mesh : *m_meshes) {
		DX12Texture2D* texture = static_cast<DX12Texture2D*>(mesh->textures.begin()->second);

		cpuHandle.ptr += incr;
		gpuHandle.ptr += incr;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = texture->getFormat();
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texture->getMips();
		m_renderer->getDevice()->CreateShaderResourceView(texture->getResource(), &srvDesc, cpuHandle);

		MeshHandles handles;
		handles.vertexBufferHandle = static_cast<DX12VertexBuffer*>(mesh->geometryBuffer.buffer)->getBuffer()->GetGPUVirtualAddress();
		handles.textureHandle = gpuHandle;
		m_rtMeshHandles.emplace_back(handles);
	}

	// Create Shader Resource Handle here for skybox
	cpuHandle.ptr += incr;
	m_skyboxGPUDescHandle.ptr = gpuHandle.ptr + incr;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_skyboxTexture->getFormat();
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = m_skyboxTexture->getMips();
	m_renderer->getDevice()->CreateShaderResourceView(m_skyboxTexture->getResource(), &srvDesc, cpuHandle); 
	/*TODO:
		The ViewDimension in the View Desc is incompatible with the type of the Resource
	*/

	// Ray gen settings CB
	m_rayGenCBData.flags = 0;
	m_rayGenCBData.numAORays = 5;
	m_rayGenCBData.AORadius = 0.9f;
	m_rayGenCBData.frameCount = 0;
	m_rayGenCBData.GISamples = 1;
	m_rayGenCBData.GIBounces = 1;
	m_rayGenSettingsCB = new DX12ConstantBuffer("Ray Gen Settings CB", sizeof(RayGenSettings), m_renderer);
	m_rayGenSettingsCB->setData(&m_rayGenCBData, 0);

	// Scene CB
	m_sceneCBData = new SceneConstantBuffer();
	m_sceneCB = new DX12ConstantBuffer("Scene Constant Buffer", sizeof(SceneConstantBuffer), m_renderer);
	m_sceneCB->setData(m_sceneCBData, 0/*Not used*/);

}

void DXR::createShaderTables() {

	// TODO: possibly store vertex buffer in local signatures/shader table
	//		 "Shader tables can be modified freely by the application (with appropriate state barriers)"

	// Ray gen
	{
		m_rayGenShaderTable.Resource.Reset();
		D3DUtils::ShaderTableBuilder tableBuilder(m_rayGenName, m_rtPipelineState.Get());
		tableBuilder.addDescriptor(m_rtOutputUAV.gpuHandle.ptr);
		m_rayGenShaderTable = tableBuilder.build(m_renderer->getDevice());
	}

	// Miss
	{
		m_missShaderTable.Resource.Reset();
		D3DUtils::ShaderTableBuilder tableBuilder(m_missName, m_rtPipelineState.Get());
		tableBuilder.addDescriptor(m_skyboxGPUDescHandle.ptr); // TODO: Check if correct
		m_missShaderTable = tableBuilder.build(m_renderer->getDevice());
	}

	// Hit group
	{
		auto rayGenHandle = m_rayGenSettingsCB->getBuffer(0)->GetGPUVirtualAddress();
		m_hitGroupShaderTable.Resource.Reset();
		D3DUtils::ShaderTableBuilder tableBuilder(m_hitGroupName, m_rtPipelineState.Get(), m_meshes->size());
		for (unsigned int i = 0; i < m_meshes->size(); i++) {
			tableBuilder.addDescriptor(m_rtMeshHandles[i].vertexBufferHandle, i); // only supports one texture/mesh atm // TODO FIX
			tableBuilder.addDescriptor(m_rtMeshHandles[i].textureHandle.ptr, i); // only supports one texture/mesh atm // TODO FIX
			tableBuilder.addDescriptor(rayGenHandle, i);
		}
		m_hitGroupShaderTable = tableBuilder.build(m_renderer->getDevice());
	}

}

void DXR::createBLAS(ID3D12GraphicsCommandList4* cmdList, bool onlyUpdate) {

	// TODO: Allow multiple vertex buffers
	// TODO: Allow for BLAS updates (multiple calls) in place?

	if (m_meshes != nullptr) {

		if (m_numMeshesChanged) {
			m_DXR_BottomBuffers.clear();
			m_DXR_BottomBuffers.resize(m_meshes->size());
		}

		for (unsigned int i = 0; i < m_meshes->size(); i++) {
			DX12VertexBuffer* vb = static_cast<DX12VertexBuffer*>((*m_meshes)[i]->geometryBuffer.buffer);
			
			D3D12_RAYTRACING_GEOMETRY_DESC geomDesc[1] = {};
			geomDesc[0] = {};
			geomDesc[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geomDesc[0].Triangles.VertexBuffer.StartAddress = vb->getBuffer()->GetGPUVirtualAddress();
			geomDesc[0].Triangles.VertexBuffer.StrideInBytes = vb->getVertexStride();
			geomDesc[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			geomDesc[0].Triangles.VertexCount = vb->getVertexCount();
			geomDesc[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

			// Get the size requirements for the scratch and AS buffers
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
			inputs.NumDescs = 1;
			inputs.pGeometryDescs = geomDesc;
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
			m_renderer->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

			// Only create the buffer the first time or if the size needs to change
			if (m_numMeshesChanged || !onlyUpdate) {
				// Release old buffers if they exist
				m_DXR_BottomBuffers[i].scratch.Reset();
				m_DXR_BottomBuffers[i].result.Reset();
				// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
				m_DXR_BottomBuffers[i].scratch = D3DUtils::createBuffer(m_renderer->getDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3DUtils::sDefaultHeapProps);
				m_DXR_BottomBuffers[i].result = D3DUtils::createBuffer(m_renderer->getDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3DUtils::sDefaultHeapProps);
			}

			// Create the bottom-level AS
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
			asDesc.Inputs = inputs;
			asDesc.DestAccelerationStructureData = m_DXR_BottomBuffers[i].result->GetGPUVirtualAddress();
			asDesc.ScratchAccelerationStructureData = m_DXR_BottomBuffers[i].scratch->GetGPUVirtualAddress();

			cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

			// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
			D3D12_RESOURCE_BARRIER uavBarrier = {};
			uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			uavBarrier.UAV.pResource = m_DXR_BottomBuffers[i].result.Get();
			cmdList->ResourceBarrier(1, &uavBarrier);
		}
	}


}

void DXR::createTLAS(ID3D12GraphicsCommandList4* cmdList, std::function<DirectX::XMFLOAT3X4(int)> instanceTransform) {

	unsigned int instanceCount = (m_meshes) ? m_meshes->size() : 0;

	if (m_meshes != nullptr && m_meshes->size() > instanceCount)
		std::cout << "WARNING: There is more geometry in the BLAS than instances in the TLAS" << std::endl;

	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = instanceCount;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	if (m_numMeshesChanged) {
		m_DXR_TopBuffers.instanceDesc.Reset();
		m_DXR_TopBuffers.result.Reset();
		m_DXR_TopBuffers.scratch.Reset();
	}

	// on first call, create the buffer
	if (m_DXR_TopBuffers.instanceDesc == nullptr) {
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		m_renderer->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		// Create the buffers
		if (m_DXR_TopBuffers.scratch == nullptr) {
			m_DXR_TopBuffers.scratch = D3DUtils::createBuffer(m_renderer->getDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3DUtils::sDefaultHeapProps);
		}

		if (m_DXR_TopBuffers.result == nullptr) {
			m_DXR_TopBuffers.result = D3DUtils::createBuffer(m_renderer->getDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3DUtils::sDefaultHeapProps);
		}

		m_DXR_TopBuffers.instanceDesc = D3DUtils::createBuffer(m_renderer->getDevice(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * max(instanceCount, 1), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3DUtils::sUploadHeapProperties);
	}

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	m_DXR_TopBuffers.instanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);

	for (UINT i = 0; i < instanceCount; i++) {

		pInstanceDesc->InstanceID = i;                            // exposed to the shader via InstanceID()
		pInstanceDesc->InstanceContributionToHitGroupIndex = i;   // offset inside the shader-table. we only have a single geometry, so the offset 0
		pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

		// apply transform from lambda function
		XMFLOAT3X4 m = instanceTransform(i);
		memcpy(pInstanceDesc->Transform, &m, sizeof(pInstanceDesc->Transform));

		pInstanceDesc->AccelerationStructure = m_DXR_BottomBuffers[i].result->GetGPUVirtualAddress();
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

	m_localSignatureRayGen = createRayGenLocalRootSignature();
	m_localSignatureHitGroup = createHitGroupLocalRootSignature();
	m_localSignatureMiss = createMissLocalRootSignature();

	D3DUtils::PSOBuilder psoBuilder;
	psoBuilder.addLibrary(m_renderer->getShaderPath() + "RayTracingShaders.hlsl", { m_rayGenName, m_closestHitName, m_missName });
	psoBuilder.addHitGroup(m_hitGroupName, m_closestHitName);
	psoBuilder.addSignatureToShaders({ m_rayGenName }, m_localSignatureRayGen.GetAddressOf());
	psoBuilder.addSignatureToShaders({ m_closestHitName }, m_localSignatureHitGroup.GetAddressOf());
	psoBuilder.addSignatureToShaders({ m_missName }, m_localSignatureMiss.GetAddressOf());
	psoBuilder.setMaxPayloadSize(sizeof(RayPayload));
	psoBuilder.setMaxRecursionDepth(MAX_RAY_RECURSION_DEPTH);
	psoBuilder.setGlobalSignature(m_dxrGlobalRootSignature.GetAddressOf());

	m_rtPipelineState = psoBuilder.build(m_renderer->getDevice());
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
	/*rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].Descriptor.ShaderRegister = 1;
	rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].Descriptor.RegisterSpace = 0;*/

	// Scene CBV
	rootParams[DXRGlobalRootParam::CBV_SCENE_BUFFER].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[DXRGlobalRootParam::CBV_SCENE_BUFFER].Descriptor.ShaderRegister = 0;
	rootParams[DXRGlobalRootParam::CBV_SCENE_BUFFER].Descriptor.RegisterSpace = 2;

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
	D3D12_ROOT_PARAMETER rootParams[DXRRayGenRootParam::SIZE]{};

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
	D3D12_DESCRIPTOR_RANGE range[1]{};
	D3D12_ROOT_PARAMETER rootParams[DXRHitGroupRootParam::SIZE]{};

	// diffuseTexture
	range[0].BaseShaderRegister = 2;
	range[0].NumDescriptors = 1;
	range[0].RegisterSpace = 0;
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[DXRHitGroupRootParam::SRV_VERTEX_BUFFER].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[DXRHitGroupRootParam::SRV_VERTEX_BUFFER].Descriptor.ShaderRegister = 1;
	rootParams[DXRHitGroupRootParam::SRV_VERTEX_BUFFER].Descriptor.RegisterSpace = 0;
	
	rootParams[DXRHitGroupRootParam::DT_TEXTURES].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[DXRHitGroupRootParam::DT_TEXTURES].DescriptorTable.NumDescriptorRanges = _countof(range);
	rootParams[DXRHitGroupRootParam::DT_TEXTURES].DescriptorTable.pDescriptorRanges = range;

	/*D3D12_DESCRIPTOR_RANGE range2[1]{};
	range2[0].BaseShaderRegister = 0;
	range2[0].RegisterSpace = 1;
	range2[0].NumDescriptors = 1;
	range2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[DXRRayGenRootParam::CBV_RAY_GEN].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[DXRRayGenRootParam::CBV_RAY_GEN].DescriptorTable.NumDescriptorRanges = _countof(range2);
	rootParams[DXRRayGenRootParam::CBV_RAY_GEN].DescriptorTable.pDescriptorRanges = range2;*/

	// Ray Gen settings CBV
	rootParams[DXRHitGroupRootParam::CBV_SETTINGS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[DXRHitGroupRootParam::CBV_SETTINGS].Descriptor.ShaderRegister = 0;
	rootParams[DXRHitGroupRootParam::CBV_SETTINGS].Descriptor.RegisterSpace = 1;
	rootParams[DXRHitGroupRootParam::CBV_SETTINGS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};
	staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.MipLODBias = 0.f;
	staticSamplerDesc.MaxAnisotropy = 1;
	staticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplerDesc.MinLOD = 0.f;
	staticSamplerDesc.MaxLOD = FLT_MAX;
	staticSamplerDesc.RegisterSpace = 0;
	staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(rootParams);
	desc.pParameters = rootParams;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = &staticSamplerDesc;


	ID3DBlob* sigBlob;
	ID3DBlob* errorBlob;
	ThrowIfBlobError(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob), errorBlob);
	ID3D12RootSignature* rootSig;
	m_renderer->getDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
	rootSig->SetName(L"HitGroupLocal");

	return rootSig;
}

ID3D12RootSignature* DXR::createMissLocalRootSignature() {
	D3D12_ROOT_PARAMETER rootParams[DXRMissRootParam::SIZE]{};

	D3D12_DESCRIPTOR_RANGE range[1]{};
	range[0].BaseShaderRegister = 3;
	range[0].NumDescriptors = 1;
	range[0].RegisterSpace = 0;
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[DXRMissRootParam::SRV_SKYBOX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[DXRMissRootParam::SRV_SKYBOX].DescriptorTable.NumDescriptorRanges = _countof(range);
	rootParams[DXRMissRootParam::SRV_SKYBOX].DescriptorTable.pDescriptorRanges = range;
	/*rootParams[DXRMissRootParam::SRV_SKYBOX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[DXRMissRootParam::SRV_SKYBOX].Descriptor.ShaderRegister = 3;
	rootParams[DXRMissRootParam::SRV_SKYBOX].Descriptor.RegisterSpace = 0;*/

	D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};
	staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.MipLODBias = 0.f;
	staticSamplerDesc.MaxAnisotropy = 1;
	staticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplerDesc.MinLOD = 0.f;
	staticSamplerDesc.MaxLOD = FLT_MAX;
	staticSamplerDesc.RegisterSpace = 0;
	staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(rootParams);
	desc.pParameters = rootParams;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = &staticSamplerDesc;

	ID3DBlob* sigBlob;
	ID3DBlob* errorBlob;
	ThrowIfBlobError(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob), errorBlob);
	ID3D12RootSignature* pRootSig;
	m_renderer->getDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
	pRootSig->SetName(L"MissLocal");

	return pRootSig;
}
