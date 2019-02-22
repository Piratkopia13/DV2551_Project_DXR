#pragma once
#include <unordered_map>
#include "DX12.h"
#include "../Core/Mesh.h"

class DX12Renderer;

class DX12Mesh : public Mesh {
public:
	DX12Mesh(DX12Renderer* renderer);
	~DX12Mesh();
	virtual void setIAVertexBufferBinding(VertexBuffer* buffer, size_t offset, size_t numElements, size_t sizeElement) override;
	virtual void bindIAVertexBuffer(unsigned int location) override;
	void bindIAVertexBuffer(ID3D12GraphicsCommandList3* cmdList);

};

