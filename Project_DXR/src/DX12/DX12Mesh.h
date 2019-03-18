#pragma once
#include <unordered_map>
#include "DX12.h"
#include "DX12ConstantBuffer.h"
#include "../Core/Mesh.h"

class DX12Renderer;

class DX12Mesh : public Mesh {
public:
	DX12Mesh(DX12Renderer* renderer);
	~DX12Mesh();
	virtual void setIABinding(VertexBuffer* vBuffer, IndexBuffer* iBuffer, size_t offset, size_t numVertices, size_t numIndices, size_t sizeElement) override;
	virtual void bindIA(unsigned int location) override;
	void bindIA(ID3D12GraphicsCommandList3* cmdList);

	const DX12ConstantBuffer* getMaterialCB() const;
	const MaterialProperties& getProperties() const;
	void setProperties(const MaterialProperties& props);

private:
	DX12ConstantBuffer m_materialCB;
	MaterialProperties m_matProps; // MaterialProperties is a shader-common struct

};

