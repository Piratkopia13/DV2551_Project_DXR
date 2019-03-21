#include "pch.h"
#include "D3DUtils.h"

#include "DX12.h"

const D3D12_HEAP_PROPERTIES D3DUtils::sUploadHeapProperties = {
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0,
};

const D3D12_HEAP_PROPERTIES D3DUtils::sDefaultHeapProps = {
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0
};

void D3DUtils::UpdateDefaultBufferData(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* data,
	UINT64 byteSize,
	UINT64 offset,
	ID3D12Resource1* defaultBuffer,
	ID3D12Resource1** uploadBuffer) 
{
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer)));
	(*uploadBuffer)->SetName(L"Vertex upload heap");

	// Put in barriers in order to schedule for the data to be copied
	// to the default buffer resource.
	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_COPY_DEST));

	// Prepare the data to be uploaded to the GPU
	BYTE* pData;
	ThrowIfFailed((*uploadBuffer)->Map(0, NULL, reinterpret_cast<void**>(&pData)));
	memcpy(pData, data, byteSize);
	(*uploadBuffer)->Unmap(0, NULL);

	// Copy the data from the uploadBuffer to the defaultBuffer
	cmdList->CopyBufferRegion(defaultBuffer, offset, *uploadBuffer, 0, byteSize);
	
	// "Remove" the resource barrier
	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));
}


D3DUtils::PSOBuilder::PSOBuilder()
	: m_numSubobjects(0)
	, m_maxAttributeSize(sizeof(float) * 2)
	, m_maxPayloadSize(0)
	, m_maxRecursionDepth(1)
	, m_globalRootSignature(nullptr)
{
	m_dxilCompiler.init();
	m_associationNames.reserve(10);
	m_exportAssociations.reserve(10);
}

D3DUtils::PSOBuilder::~PSOBuilder() {
}

D3D12_STATE_SUBOBJECT* D3DUtils::PSOBuilder::append(D3D12_STATE_SUBOBJECT_TYPE type, const void* desc) {
	D3D12_STATE_SUBOBJECT* so = m_start + m_numSubobjects++;
	so->Type = type;
	so->pDesc = desc;
	return so;
}

void D3DUtils::PSOBuilder::addLibrary(const std::string& shaderPath, const std::vector<LPCWSTR> names) {

	// Add names to the list of names/export to be configured in generate()
	m_shaderNames.insert(m_shaderNames.end(), names.begin(), names.end());

	DXILShaderCompiler::Desc shaderDesc;
	shaderDesc.CompileArguments.push_back(L"/Gis");
	std::wstring stemp = std::wstring(shaderPath.begin(), shaderPath.end());
	shaderDesc.FilePath = stemp.c_str();
	shaderDesc.EntryPoint = L"";
	shaderDesc.TargetProfile = L"lib_6_3";

	IDxcBlob* pShaders = nullptr;
	ThrowIfFailed(m_dxilCompiler.compileFromFile(&shaderDesc, &pShaders));

	m_exportDescs.emplace_back(names.size());
	std::vector<D3D12_EXPORT_DESC>& dxilExports = m_exportDescs.back();
	for (int i = 0; i < names.size(); i++) {
		auto& desc = dxilExports[i];
		desc.Name = names[i];
		desc.ExportToRename = nullptr;
		desc.Flags = D3D12_EXPORT_FLAG_NONE;
	}

	// Set up DXIL description
	m_libraryDescs.emplace_back();
	D3D12_DXIL_LIBRARY_DESC& dxilLibraryDesc = m_libraryDescs.back();;
	dxilLibraryDesc.DXILLibrary.pShaderBytecode = pShaders->GetBufferPointer();
	dxilLibraryDesc.DXILLibrary.BytecodeLength = pShaders->GetBufferSize();
	dxilLibraryDesc.pExports = &dxilExports[0];
	dxilLibraryDesc.NumExports = UINT(dxilExports.size());

	append(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibraryDesc);

}

void D3DUtils::PSOBuilder::addHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport, LPCWSTR intersectionShaderImport, D3D12_HIT_GROUP_TYPE type) {
	//Init hit group
	m_hitGroupDescs.emplace_back();
	D3D12_HIT_GROUP_DESC& hitGroupDesc = m_hitGroupDescs.back();
	hitGroupDesc.AnyHitShaderImport = anyHitShaderImport;
	hitGroupDesc.ClosestHitShaderImport = closestHitShaderImport;
	hitGroupDesc.HitGroupExport = exportName;
	hitGroupDesc.IntersectionShaderImport = intersectionShaderImport;
	hitGroupDesc.Type = type;

	append(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc);
}

void D3DUtils::PSOBuilder::addSignatureToShaders(std::vector<LPCWSTR> shaderNames, ID3D12RootSignature** rootSignature) {
	auto signatureSO = append(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, rootSignature);
	
	m_associationNames.emplace_back(shaderNames);

	// Bind local root signature shaders
	m_exportAssociations.emplace_back();
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& rayGenLocalRootAssociation = m_exportAssociations.back();
	rayGenLocalRootAssociation.pExports = &m_associationNames.back()[0];
	rayGenLocalRootAssociation.NumExports = UINT(shaderNames.size());
	rayGenLocalRootAssociation.pSubobjectToAssociate = signatureSO; //<-- address to local root subobject

	append(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &rayGenLocalRootAssociation);
}

void D3DUtils::PSOBuilder::setGlobalSignature(ID3D12RootSignature** rootSignature) {
	m_globalRootSignature = rootSignature;
}

void D3DUtils::PSOBuilder::setMaxPayloadSize(UINT size) {
	m_maxPayloadSize = size;
}

void D3DUtils::PSOBuilder::setMaxAttributeSize(UINT size) {
	m_maxAttributeSize = size;
}

void D3DUtils::PSOBuilder::setMaxRecursionDepth(UINT depth) {
	m_maxRecursionDepth = depth;
}

ID3D12StateObject* D3DUtils::PSOBuilder::build(ID3D12Device5* device) {
	// Init shader config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxAttributeSizeInBytes = m_maxAttributeSize;
	shaderConfig.MaxPayloadSizeInBytes = m_maxPayloadSize;
	auto configSO = append(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig);

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociation;
	if (!m_shaderNames.empty()) {
		// Bind the payload size to the programs
		shaderConfigAssociation.pExports = &m_shaderNames[0];
		shaderConfigAssociation.NumExports = UINT(m_shaderNames.size());
		shaderConfigAssociation.pSubobjectToAssociate = configSO; //<-- address to shader config subobject
		append(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &shaderConfigAssociation);
	}
	// Init pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = m_maxRecursionDepth;
	append(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

	// Append the global root signature (I am GROOT)
	if (m_globalRootSignature)
		append(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, m_globalRootSignature);

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = m_numSubobjects;
	desc.pSubobjects = m_start;
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ID3D12StateObject* pso;
	ThrowIfFailed(device->CreateStateObject(&desc, IID_PPV_ARGS(&pso)));
	return pso;
}



D3DUtils::ShaderTableBuilder::ShaderTableBuilder(LPCWSTR shaderName, ID3D12StateObject* pso, UINT numInstances, UINT maxBytesPerInstance)
	: m_soProps(nullptr)
	, m_shaderName(shaderName)
	, m_numInstances(numInstances)
	, m_maxBytesPerInstance(maxBytesPerInstance)
{
	// Get the properties of the pre-built pipeline state object
	ThrowIfFailed(pso->QueryInterface(IID_PPV_ARGS(&m_soProps)));

	m_data = new void*[numInstances];
	m_dataOffsets = new UINT[numInstances];
	for (UINT i = 0; i < numInstances; i++) {
		m_data[i] = malloc(maxBytesPerInstance);
		m_dataOffsets[i] = 0;
	}
}

D3DUtils::ShaderTableBuilder::~ShaderTableBuilder() {
	for (UINT i = 0; i < m_numInstances; i++)
		free(m_data[i]);
	delete[] m_data;
	delete[] m_dataOffsets;
	//m_soProps->Release();
}

D3DUtils::ShaderTableData D3DUtils::ShaderTableBuilder::build(ID3D12Device5* device) {
	ShaderTableData shaderTable;

	UINT sizeOfLargestInstance = 0;
	for (UINT i = 0; i < m_numInstances; i++) {
		if (m_dataOffsets[i] > sizeOfLargestInstance)
			sizeOfLargestInstance = m_dataOffsets[i];
	}
	UINT size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeOfLargestInstance;

	UINT alignTo = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	UINT padding = (alignTo - (size % alignTo)) % alignTo;

	UINT alignedSize = size + padding;

	shaderTable.StrideInBytes = alignedSize;
	shaderTable.SizeInBytes = shaderTable.StrideInBytes * m_numInstances;
	shaderTable.Resource = D3DUtils::createBuffer(device, shaderTable.SizeInBytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, sUploadHeapProperties);

	// Map the buffer
	// Use a char* to to pointer arithmetic per byte
	char* pData;
	shaderTable.Resource->Map(0, nullptr, (void**)&pData);
	{
		for (UINT i = 0; i < m_numInstances; i++) {
			// Copy shader identifier
			memcpy(pData, m_soProps->GetShaderIdentifier(m_shaderName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			pData += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			// Copy other data (descriptors, constants)
			memcpy(pData, m_data[i], m_dataOffsets[i]);
			pData += alignedSize - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; // Append padding
		}
	}
	shaderTable.Resource->Unmap(0, nullptr);

	return shaderTable;
}

void D3DUtils::ShaderTableBuilder::addDescriptor(UINT64& descriptor, UINT instance) {
	assert(instance < m_numInstances);
	auto ptr = static_cast<char*>(m_data[instance]) + m_dataOffsets[instance];
	*(UINT64*)ptr = descriptor;
	m_dataOffsets[instance] += sizeof(descriptor);
	assert(m_dataOffsets[instance] <= m_maxBytesPerInstance);
}

void D3DUtils::ShaderTableBuilder::addConstants(UINT numConstants, float* constants, UINT instance) {
	assert(instance < m_numInstances);
	auto ptr = static_cast<char*>(m_data[instance]) + m_dataOffsets[instance];
	memcpy(ptr, constants, sizeof(float) * numConstants);
	m_dataOffsets[instance] += sizeof(float) * numConstants;
	assert(m_dataOffsets[instance] <= m_maxBytesPerInstance);
}
