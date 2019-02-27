#include "pch.h"
#include "DX12IndexBuffer.h"

#include "DX12Renderer.h"
#include "D3DUtils.h"

DX12IndexBuffer::DX12IndexBuffer(size_t byteSize, IndexBuffer::DATA_USAGE usage, DX12Renderer* renderer)
	: m_renderer(renderer)
	, m_byteSize(byteSize)
	, m_usage(usage) {

	ThrowIfFailed(m_renderer->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_indexBuffer.GetAddressOf())));

	m_indexBuffer->SetName(L"index buffer");

	//m_lastBoundVBSlot = -1;
}

DX12IndexBuffer::~DX12IndexBuffer() {
	releaseBufferedObjects();
	m_renderer = nullptr;
}

void DX12IndexBuffer::setData(const void* data, size_t size, size_t offset) {
	assert(size + offset <= m_byteSize);

	ID3D12Resource1* uploadBuffer = nullptr;
	D3DUtils::UpdateDefaultBufferData(m_renderer->getDevice(), m_renderer->getCmdList(),
		data, size, offset, m_indexBuffer.Get(), &uploadBuffer);
	// To be released, let renderer handle later
	m_uploadBuffers.emplace_back(uploadBuffer);
}

void DX12IndexBuffer::bind(size_t offset, size_t size, unsigned int location) {
	throw std::exception("The index buffer must be bound using the other bind method taking four parameters");
}

void DX12IndexBuffer::bind(size_t offset, size_t size, unsigned int location, ID3D12GraphicsCommandList3* cmdList) {
	assert(size + offset <= m_byteSize);
	assert(location == 0); // This parameter is not used!

	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress() + offset;
	ibView.SizeInBytes = static_cast<UINT>(size);
	ibView.Format = DXGI_FORMAT_R32_UINT;
	// Later update to just put in a buffer on the renderer to set multiple vertex buffers at once
	cmdList->IASetIndexBuffer(&ibView);
}

void DX12IndexBuffer::unbind() {
	m_renderer->getCmdList()->IASetIndexBuffer(nullptr);
}

size_t DX12IndexBuffer::getSize() {
	return m_byteSize;
}

size_t DX12IndexBuffer::getNumIndices() {
	return m_byteSize / sizeof(unsigned int);
}

ID3D12Resource1* DX12IndexBuffer::getBuffer() const {
	return m_indexBuffer.Get();
}

void DX12IndexBuffer::releaseBufferedObjects() {
	for (auto uBuffer : m_uploadBuffers) {
		SafeRelease(&uBuffer);
	}
	m_uploadBuffers.clear();
}
