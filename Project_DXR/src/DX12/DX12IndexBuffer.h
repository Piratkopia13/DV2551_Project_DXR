#pragma once

#include <vector>

#include "../Core/IndexBuffer.h"
#include "DX12.h"

class DX12Renderer;

class DX12IndexBuffer : public IndexBuffer {
public:
	DX12IndexBuffer(size_t byteSize, IndexBuffer::DATA_USAGE usage, DX12Renderer* renderer);
	virtual ~DX12IndexBuffer();
	virtual void setData(const void* data, size_t size, size_t offset) override;
	virtual void bind(size_t offset, size_t size, unsigned int location) override;
	void bind(size_t offset, size_t size, unsigned int location, ID3D12GraphicsCommandList3* cmdList);
	virtual void unbind() override;
	virtual size_t getSize() override;

	size_t getNumIndices();
	ID3D12Resource1* getBuffer() const;
	void releaseBufferedObjects();

private:
	IndexBuffer::DATA_USAGE m_usage;

	wComPtr<ID3D12Resource1> m_indexBuffer;

	// uploadBuffers to be released
	std::vector<ID3D12Resource1*> m_uploadBuffers;

	size_t m_byteSize;
	DX12Renderer* m_renderer;

};

