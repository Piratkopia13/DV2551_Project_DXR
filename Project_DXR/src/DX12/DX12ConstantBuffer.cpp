#include "pch.h"
#include "DX12ConstantBuffer.h"
#include "DX12Renderer.h"
#include "DX12Material.h"

DX12ConstantBuffer::DX12ConstantBuffer(std::string name, size_t size, DX12Renderer* renderer)
	: ConstantBuffer(name, size)
	, m_name(name)
	, m_renderer(renderer)
{

	m_constantBufferUploadHeap = new wComPtr<ID3D12Resource1>[renderer->getNumSwapBuffers()];
	m_cbGPUAddress = new UINT8*[renderer->getNumSwapBuffers()];
	m_needsUpdate = new bool[renderer->getNumSwapBuffers()];
	for (UINT i = 0; i < m_renderer->getNumSwapBuffers(); i++)
		m_needsUpdate[i] = false;
	
	// Create an upload heap to hold the constant buffer
	// create a resource heap, descriptor heap, and pointer to cbv for each frame
	for (UINT i = 0; i < renderer->getNumSwapBuffers(); ++i) {
		ThrowIfFailed(renderer->getDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&m_constantBufferUploadHeap[i])));
		m_constantBufferUploadHeap[i]->SetName(L"Constant Buffer Upload Resource Heap");

		// Map the constant buffer and keep it mapped for the duration of its lifetime
		CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
		ThrowIfFailed(m_constantBufferUploadHeap[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_cbGPUAddress[i])));

		// TODO: copy upload heap to a default heap when the data is not often changed
	}

	// Allocate cpu memory for the buffer
	// Memory leak
	m_newData = malloc(size);

}

DX12ConstantBuffer::~DX12ConstantBuffer() {

	delete[] m_constantBufferUploadHeap;
	delete[] m_cbGPUAddress;
	delete[] m_needsUpdate;
	free(m_newData);

}

void DX12ConstantBuffer::setData(const void* data, unsigned int location) {
	m_location = location;
	memcpy(m_newData, data, bufferSize);
	for (UINT i = 0; i < m_renderer->getNumSwapBuffers(); i++)
		m_needsUpdate[i] = true;
}

void DX12ConstantBuffer::bind(Material* m) {

	throw std::exception("The constant buffer must be bound using the other bind method taking two parameters");

}

void DX12ConstantBuffer::bind(Material* material, ID3D12GraphicsCommandList3* cmdList) {

	UINT rootIndex;
	if (m_location == CB_REG_TRANSFORM) rootIndex = GlobalRootParam::CBV_TRANSFORM;
	else if (m_location == CB_REG_DIFFUSE_TINT) rootIndex = GlobalRootParam::CBV_DIFFUSE_TINT;
	else if (m_location == CB_REG_CAMERA) rootIndex = GlobalRootParam::CBV_CAMERA;
	else assert(false); // TODO: Map these values in a better way

	UINT frameIndex = m_renderer->getFrameIndex();
	if (m_needsUpdate[frameIndex]) {
		updateBuffer(frameIndex);
		m_needsUpdate[frameIndex] = false;
	}

	cmdList->SetGraphicsRootConstantBufferView(rootIndex, m_constantBufferUploadHeap[frameIndex]->GetGPUVirtualAddress());

}

ID3D12Resource1* DX12ConstantBuffer::getBuffer(unsigned int frameIndex) const {
	if (m_needsUpdate[frameIndex]) {
		updateBuffer(frameIndex);
		m_needsUpdate[frameIndex] = false;
	}
	return m_constantBufferUploadHeap[frameIndex].Get();
}

void DX12ConstantBuffer::forceUpdate(unsigned int frameIndex) {
	updateBuffer(frameIndex);
}

void DX12ConstantBuffer::updateBuffer(UINT frameIndex) const {

	memcpy(m_cbGPUAddress[frameIndex], m_newData, bufferSize);

}
