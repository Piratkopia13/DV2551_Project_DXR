#include "pch.h"
#include "DX12Mesh.h"
#include "DX12VertexBuffer.h"

DX12Mesh::DX12Mesh() {}
DX12Mesh::~DX12Mesh() {}

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
