#pragma once

#include "DX12.h"
#include "DXILShaderCompiler.h"
#include <d3d12.h>
#include <DirectXMath.h>

class D3DUtils {
public:

	static const D3D12_HEAP_PROPERTIES sUploadHeapProperties;
	static const D3D12_HEAP_PROPERTIES sDefaultHeapProps;

	static void UpdateDefaultBufferData(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* data,
		UINT64 byteSize,
		UINT64 offset,
		ID3D12Resource1* defaultBuffer,
		ID3D12Resource1** uploadBuffer
	);

	static ID3D12Resource1* createBuffer(ID3D12Device5* device, UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC* bufDesc = nullptr) {

		D3D12_RESOURCE_DESC newBufDesc = {};
		if (!bufDesc) {
			newBufDesc.Alignment = 0;
			newBufDesc.DepthOrArraySize = 1;
			newBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			newBufDesc.Flags = flags;
			newBufDesc.Format = DXGI_FORMAT_UNKNOWN;
			newBufDesc.Height = 1;
			newBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			newBufDesc.MipLevels = 1;
			newBufDesc.SampleDesc.Count = 1;
			newBufDesc.SampleDesc.Quality = 0;
			newBufDesc.Width = size;

			bufDesc = &newBufDesc;
		}

		ID3D12Resource1* pBuffer = nullptr;
		device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer));
		return pBuffer;
	}

	static void setResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter) {
		D3D12_RESOURCE_BARRIER barrierDesc = {};

		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = resource;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = StateBefore;
		barrierDesc.Transition.StateAfter = StateAfter;

		commandList->ResourceBarrier(1, &barrierDesc);
	}

	static bool xmMatrixEqual(const DirectX::XMMATRIX& mat1, const DirectX::XMMATRIX& mat2) {
		return DirectX::XMVector4Equal(mat1.r[0], mat2.r[0]) && DirectX::XMVector4Equal(mat1.r[1], mat2.r[1]) && DirectX::XMVector4Equal(mat1.r[2], mat2.r[2]) && DirectX::XMVector4Equal(mat1.r[3], mat2.r[3]);
	}

	class PSOBuilder {
	public:
		PSOBuilder();
		~PSOBuilder();

		D3D12_STATE_SUBOBJECT* append(D3D12_STATE_SUBOBJECT_TYPE type, const void* desc);
		void addLibrary(const std::string& shaderPath, const std::vector<LPCWSTR> names);
		void addHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport = nullptr, LPCWSTR intersectionShaderImport = nullptr, D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES);
		void addSignatureToShaders(std::vector<LPCWSTR> shaderNames, ID3D12RootSignature** rootSignature);
		void setGlobalSignature(ID3D12RootSignature** rootSignature);
		void setMaxPayloadSize(UINT size);
		void setMaxAttributeSize(UINT size);
		void setMaxRecursionDepth(UINT depth);

		ID3D12StateObject* build(ID3D12Device5* device);

	private:
		DXILShaderCompiler m_dxilCompiler;

		D3D12_STATE_SUBOBJECT m_start[100];
		UINT m_numSubobjects;

		// Settings
		UINT m_maxPayloadSize;
		UINT m_maxAttributeSize;
		UINT m_maxRecursionDepth;
		ID3D12RootSignature** m_globalRootSignature;

		// Objects to keep in memory until generate() is called
		std::vector<LPCWSTR> m_shaderNames;
		std::vector<std::vector<D3D12_EXPORT_DESC>> m_exportDescs;
		std::vector<D3D12_DXIL_LIBRARY_DESC> m_libraryDescs;
		std::vector<std::vector<LPCWSTR>> m_associationNames;
		std::vector<D3D12_HIT_GROUP_DESC> m_hitGroupDescs;
		std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> m_exportAssociations;
	};


	struct ShaderTableData {
		UINT64 SizeInBytes;
		UINT32 StrideInBytes;
		wComPtr<ID3D12Resource1> Resource = nullptr;
	};

	class ShaderTableBuilder {
	public:
		ShaderTableBuilder(LPCWSTR shaderName, ID3D12StateObject* pso, UINT numInstances = 1, UINT maxBytesPerInstance = 32);
		~ShaderTableBuilder();

		void addDescriptor(UINT64& descriptor, UINT instance = 0);
		void addConstants(UINT numConstants, float* constants, UINT instance = 0);

		ShaderTableData build(ID3D12Device5* device);

	private:
		wComPtr<ID3D12StateObjectProperties> m_soProps;
		LPCWSTR m_shaderName;
		UINT m_numInstances;
		UINT m_maxBytesPerInstance;

		void** m_data;
		UINT* m_dataOffsets;
	};

};