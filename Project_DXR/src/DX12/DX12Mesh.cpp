#include "pch.h"
#include "DX12Mesh.h"
#include "DX12VertexBuffer.h"
#include "DX12IndexBuffer.h"
#include "DX12Renderer.h"

DX12Mesh::DX12Mesh(DX12Renderer* renderer) {
	transformCB = renderer->makeConstantBuffer("Transform", sizeof(DirectX::XMMATRIX));
	transformCB->setData(&transform.getTransformMatrix(), CB_REG_TRANSFORM);
}
DX12Mesh::~DX12Mesh() {
}

void DX12Mesh::setIABinding(VertexBuffer* vBuffer, IndexBuffer* iBuffer, size_t offset, size_t numVertices, size_t numIndices, size_t sizeElement) {
	Mesh::setIABinding(vBuffer, iBuffer, offset, numVertices, numIndices, sizeElement);
	static_cast<DX12VertexBuffer*>(vBuffer)->setVertexStride(sizeElement);
};

void DX12Mesh::bindIA(unsigned int location) {
	throw std::exception("The IA inputs must be bound using the other bind method taking a command list as parameter");
}

void DX12Mesh::bindIA(ID3D12GraphicsCommandList3* cmdList) {
	// no checking if the key is valid...TODO
	static_cast<DX12VertexBuffer*>(geometryBuffer.vBuffer)->bind(geometryBuffer.offset, geometryBuffer.numVertices * geometryBuffer.sizeElement, 0, cmdList);
	static_cast<DX12IndexBuffer*>(geometryBuffer.iBuffer)->bind(geometryBuffer.offset, geometryBuffer.numIndices * sizeof(unsigned int), 0, cmdList);
}
