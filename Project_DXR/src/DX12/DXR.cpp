#include "pch.h"
#include "DXR.h"
#include "DX12Renderer.h"
#include "DX12VertexBuffer.h"
#include "DXILShaderCompiler.h"
#include "DX12ConstantBuffer.h"
#include "../Core/Camera.h"
#include "../Core/CameraController.h"
#include "../Utils/Input.h"

using namespace DirectX;

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


	/*if (Input::IsKeyDown('S')) {
		m_persCamera->move(XMVectorSet(0.f, 0.f, -0.1f, 0.f));
		m_sceneCBData->cameraPosition = m_persCamera->getPosition();
		m_sceneCBData->projectionToWorld = m_persCamera->getInvProjMatrix() * m_persCamera->getInvViewMatrix();
		m_sceneCB->setData(m_sceneCBData, sizeof(SceneConstantBuffer), nullptr, 0);

		std::cout << "Moving camera  Z: " << m_sceneCBData->cameraPosition.z << std::endl;
	}
	if (Input::IsKeyDown('W')) {
		m_persCamera->move(XMVectorSet(0.f, 0.f, 0.1f, 0.f));
		m_sceneCBData->cameraPosition = m_persCamera->getPosition();
		m_sceneCBData->projectionToWorld = m_persCamera->getInvProjMatrix() * m_persCamera->getInvViewMatrix();
		m_sceneCB->setData(m_sceneCBData, sizeof(SceneConstantBuffer), nullptr, 0);

		std::cout << "Moving camera  Z: " << m_sceneCBData->cameraPosition.z << std::endl;
	}*/


	/* Camera movement, Move this to main */
	m_persCameraController->update(0.016f);
	m_sceneCBData->cameraPosition = m_persCamera->getPositionF3();
	m_sceneCBData->projectionToWorld = m_persCamera->getInvProjMatrix() * m_persCamera->getInvViewMatrix();
	m_sceneCB->setData(m_sceneCBData, sizeof(SceneConstantBuffer), nullptr, 0);
	/**/

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
	// Set constant buffer
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
	const Vertex vertices[] = {
		{XMFLOAT3(0,		1,		0), XMFLOAT3(0, 0, -1)},	// Vertex and normal
		{XMFLOAT3(0.866f,  -0.5f,	0), XMFLOAT3(0, 0, -1)},
		{XMFLOAT3(-0.866f, -0.5f,	0), XMFLOAT3(0, 0, -1)},

		{XMFLOAT3(0,       -0.5f, 1), XMFLOAT3(0, 0, -1)},		// Vertex and normal
		{XMFLOAT3(0.866f,  -1.5f, 1), XMFLOAT3(0, 0, -1)},
		{XMFLOAT3(-0.866f, -1.5f, 1), XMFLOAT3(0, 0, -1)}
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
	m_renderer->getDevice()->CreateCommittedResource(&D3DUtils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_rtOutputUAV.resource)); // Starting as copy-source to simplify onFrameRender()
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


	// Scene CB
	m_persCamera = new Camera((float)m_renderer->getWindow()->getWindowWidth() / (float)m_renderer->getWindow()->getWindowHeight(), 110.f, 0.1f, 1000.f);
	m_persCamera->setPosition(XMVectorSet(0.f, 0.f, -2.f, 0.f));
	m_persCamera->setDirection(XMVectorSet(0.f, 0.f, 1.0f, 1.0f));
	m_persCameraController = new CameraController(m_persCamera);
	m_sceneCBData = new SceneConstantBuffer();
	m_sceneCBData->projectionToWorld = m_persCamera->getInvProjMatrix() * m_persCamera->getInvViewMatrix();
	m_sceneCBData->cameraPosition = m_persCamera->getPositionF3();
	m_sceneCB = new DX12ConstantBuffer("Scene Constant Buffer", 0 /*Not used*/, m_renderer);
	m_sceneCB->setData(m_sceneCBData, sizeof(SceneConstantBuffer), nullptr, 0);
}

void DXR::createShaderTables() {

	// TODO: possibly store vertex buffer in local signatures/shader table
	//		 "Shader tables can be modified freely by the application (with appropriate state barriers)"

	// Ray gen
	{
		D3DUtils::ShaderTableBuilder tableBuilder(m_rayGenName, m_rtPipelineState.Get());
		tableBuilder.addDescriptor(m_rtOutputUAV.gpuHandle.ptr);
		m_rayGenShaderTable = tableBuilder.build(m_renderer->getDevice());
	}

	// Miss
	{
		D3DUtils::ShaderTableBuilder tableBuilder(m_missName, m_rtPipelineState.Get());
		m_missShaderTable = tableBuilder.build(m_renderer->getDevice());
	}

	// Hit group
	{
		D3DUtils::ShaderTableBuilder tableBuilder(m_hitGroupName, m_rtPipelineState.Get(), 3);
		float shaderTableColor1[] = { 1.0f, 0.0f, 0.0f };
		float shaderTableColor2[] = { 0.0f, 1.0f, 0.0f };
		float shaderTableColor3[] = { 0.0f, 0.0f, 1.0f };
		tableBuilder.addConstants(3, shaderTableColor1, 0);
		tableBuilder.addConstants(3, shaderTableColor2, 1);
		tableBuilder.addConstants(3, shaderTableColor3, 2);
		m_hitGroupShaderTable = tableBuilder.build(m_renderer->getDevice());
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

	m_DXR_BottomBuffers.scratch = D3DUtils::createBuffer(m_renderer->getDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3DUtils::sDefaultHeapProps);
	m_DXR_BottomBuffers.result = D3DUtils::createBuffer(m_renderer->getDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3DUtils::sDefaultHeapProps);

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
			m_DXR_TopBuffers.scratch = D3DUtils::createBuffer(m_renderer->getDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3DUtils::sDefaultHeapProps);
		}

		if (m_DXR_TopBuffers.result == nullptr) {
			m_DXR_TopBuffers.result = D3DUtils::createBuffer(m_renderer->getDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3DUtils::sDefaultHeapProps);
		}

		m_DXR_TopBuffers.instanceDesc = D3DUtils::createBuffer(m_renderer->getDevice(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3DUtils::sUploadHeapProperties);
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
	rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].Descriptor.ShaderRegister = 1;
	rootParams[DXRGlobalRootParam::SRV_VERTEX_BUFFER].Descriptor.RegisterSpace = 0;

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
