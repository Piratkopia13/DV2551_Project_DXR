#include "D3DUtils.h"

#include "DX12.h"

void D3DUtils::UpdateDefaultBufferData(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* data,
	UINT64 byteSize,
	UINT64 offset,
	ID3D12Resource* defaultBuffer,
	ID3D12Resource** uploadBuffer) 
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
