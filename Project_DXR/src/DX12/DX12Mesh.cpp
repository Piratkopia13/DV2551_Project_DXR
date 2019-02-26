#include "pch.h"
#include "DX12Mesh.h"
#include "DX12VertexBuffer.h"
#include "DX12Renderer.h"

DX12Mesh::DX12Mesh(DX12Renderer* renderer) {
	transformCB = renderer->makeConstantBuffer("Transform", sizeof(DirectX::XMMATRIX));
	transformCB->setData(&transform.getTransformMatrix(), CB_REG_TRANSFORM);
}
DX12Mesh::~DX12Mesh() {
}

void DX12Mesh::setIAVertexBufferBinding( VertexBuffer* buffer, size_t offset, size_t numElements, size_t sizeElement) {
	Mesh::setIAVertexBufferBinding(buffer, offset, numElements, sizeElement);
	static_cast<DX12VertexBuffer*>(buffer)->setVertexStride(sizeElement);
};

void DX12Mesh::bindIAVertexBuffer(unsigned int location) {
	throw std::exception("The IA vertex buffer must be bound using the other bind method taking a command list as parameter");
}

void DX12Mesh::bindIAVertexBuffer(ID3D12GraphicsCommandList3* cmdList) {
	// no checking if the key is valid...TODO
	static_cast<DX12VertexBuffer*>(geometryBuffer.buffer)->bind(geometryBuffer.offset, geometryBuffer.numElements * geometryBuffer.sizeElement, 0, cmdList);
}

DX12Texture2DArray * DX12Mesh::getTexture2DArray() const {
	return m_texArr;
}

void DX12Mesh::setTexture2DArray(DX12Texture2DArray * texArr) {
	m_texArr = texArr;
}