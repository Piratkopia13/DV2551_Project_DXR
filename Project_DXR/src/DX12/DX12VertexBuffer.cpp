#include "DX12VertexBuffer.h"

#include <cassert>

#include "DX12Renderer.h"
#include "D3DUtils.h"

DX12VertexBuffer::DX12VertexBuffer(size_t byteSize, VertexBuffer::DATA_USAGE usage, DX12Renderer* renderer)
	: m_renderer(renderer)
	, m_byteSize(byteSize)
	, m_usage(usage)
{

	ThrowIfFailed(m_renderer->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_vertexBuffer.GetAddressOf())));

	m_vertexBuffer->SetName(L"vertex buffer");

	m_lastBoundVBSlot = -1;
}

DX12VertexBuffer::~DX12VertexBuffer() {
	releaseBufferedObjects();
	m_renderer = nullptr;
}

void DX12VertexBuffer::setData(const void * data, size_t size, size_t offset) {
	assert(size + offset <= m_byteSize);

	ID3D12Resource* uploadBuffer = nullptr;
	D3DUtils::UpdateDefaultBufferData(m_renderer->getDevice(), m_renderer->getCmdList(),
		data, size, offset, m_vertexBuffer.Get(), &uploadBuffer);
	// To be released, let renderer handle later
	m_uploadBuffers.emplace_back(uploadBuffer);
}

void DX12VertexBuffer::bind(size_t offset, size_t size, unsigned int location) {
	throw std::exception("The vertex buffer must be bound using the other bind method taking four parameters");
}

void DX12VertexBuffer::bind(size_t offset, size_t size, unsigned int location, ID3D12GraphicsCommandList3* cmdList) {
	assert(size + offset <= m_byteSize);
	assert(m_vertexSize > 0);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress() + offset;
	vbView.SizeInBytes = static_cast<UINT>(size);
	vbView.StrideInBytes = static_cast<UINT>(m_vertexSize);
	m_lastBoundVBSlot = location;
	// Later update to just put in a buffer on the renderer to set multiple vertex buffers at once
	cmdList->IASetVertexBuffers(m_lastBoundVBSlot, 1, &vbView);
}

void DX12VertexBuffer::unbind() {
	m_renderer->getCmdList()->IASetVertexBuffers(m_lastBoundVBSlot, 1, nullptr);
}

size_t DX12VertexBuffer::getSize() {
	return m_byteSize;
}

void DX12VertexBuffer::setVertexSize(size_t size) {
	m_vertexSize = size;
}

void DX12VertexBuffer::releaseBufferedObjects() {
	for (auto uBuffer : m_uploadBuffers) {
		SafeRelease(&uBuffer);
	}
	m_uploadBuffers.clear();
}
