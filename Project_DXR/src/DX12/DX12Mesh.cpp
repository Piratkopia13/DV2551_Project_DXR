#include "DX12Mesh.h"
#include "DX12VertexBuffer.h"

DX12Mesh::DX12Mesh() {}
DX12Mesh::~DX12Mesh() {}

void DX12Mesh::addIAVertexBufferBinding( VertexBuffer* buffer, size_t offset, size_t numElements, size_t sizeElement, unsigned int inputStream) {
	Mesh::addIAVertexBufferBinding(buffer, offset, numElements, sizeElement, inputStream);
	static_cast<DX12VertexBuffer*>(buffer)->setVertexSize(sizeElement);
};

void DX12Mesh::bindIAVertexBuffer(unsigned int location) {
	throw std::exception("The IA vertex buffer must be bound using the other bind method taking two parameters");
}

void DX12Mesh::bindIAVertexBuffer(unsigned int location, ID3D12GraphicsCommandList3* cmdList) {
	// no checking if the key is valid...TODO
	const VertexBufferBind& vb = geometryBuffers[location];
	static_cast<DX12VertexBuffer*>(vb.buffer)->bind(vb.offset, vb.numElements*vb.sizeElement, location, cmdList);
}
