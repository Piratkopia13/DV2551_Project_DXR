#pragma once

#include "DX12.h"
#include <d3d12.h>

class D3DUtils {
public:

	static void UpdateDefaultBufferData(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* data,
		UINT64 byteSize,
		UINT64 offset,
		ID3D12Resource1* defaultBuffer,
		ID3D12Resource1** uploadBuffer
	);

	static ID3D12Resource1* createBuffer(ID3D12Device5* device, UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps) {
		D3D12_RESOURCE_DESC bufDesc = {};
		bufDesc.Alignment = 0;
		bufDesc.DepthOrArraySize = 1;
		bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufDesc.Flags = flags;
		bufDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufDesc.Height = 1;
		bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufDesc.MipLevels = 1;
		bufDesc.SampleDesc.Count = 1;
		bufDesc.SampleDesc.Quality = 0;
		bufDesc.Width = size;

		ID3D12Resource1* pBuffer = nullptr;
		device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer));
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

};

