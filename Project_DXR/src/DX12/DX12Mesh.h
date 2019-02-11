#pragma once
#include <unordered_map>
#include "DX12.h"
#include "../Core/Mesh.h"

class DX12Mesh : public Mesh {
public:
	DX12Mesh();
	~DX12Mesh();
	virtual void addIAVertexBufferBinding(VertexBuffer* buffer, size_t offset, size_t numElements, size_t sizeElement, unsigned int inputStream) override;
	virtual void bindIAVertexBuffer(unsigned int location) override;
	void bindIAVertexBuffer(unsigned int location, ID3D12GraphicsCommandList3* cmdList);
};

