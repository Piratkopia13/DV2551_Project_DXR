#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(const std::string& name) : m_name(name) { };

/*
	buffer: is a VertexBuffer*
	offset: offset of the first byte in the buffer used when binding
	size: how many elements (how many points, normals, UVs) should be read from this buffer
*/
void Mesh::setIABinding(VertexBuffer* vBuffer, IndexBuffer* iBuffer, size_t offset, size_t numVertices, size_t numIndices, size_t sizeElement) {
	//buffer->incRef();
	geometryBuffer = { sizeElement, numVertices, numIndices, offset, vBuffer, iBuffer };
};

void Mesh::bindIA(unsigned int location) {
	// no checking if the key is valid...TODO
	geometryBuffer.vBuffer->bind(geometryBuffer.offset, geometryBuffer.numVertices * geometryBuffer.sizeElement, location);
	geometryBuffer.iBuffer->bind(geometryBuffer.offset, geometryBuffer.numIndices * geometryBuffer.sizeElement, location);
}

// note, slot is a value set in the shader as well (registry, or binding)
void Mesh::addTexture(Texture2D* texture, unsigned int slot) {
	// would override the slot if there is another pointer here.
	textures[slot] = texture;
}

Transform& Mesh::getTransform() {
	return transform;
}

void Mesh::setTransform(Transform& transform) {
	this->transform = transform;
	transformCB->setData(&transform.getTransformMatrix(), CB_REG_TRANSFORM);
}

ConstantBuffer* Mesh::getTransformCB() {
	return transformCB;
}

Mesh::~Mesh() {
	geometryBuffer.vBuffer->decRef();
	geometryBuffer.iBuffer->decRef();
	delete transformCB;
}

void Mesh::setName(const std::string& name) {
	m_name = name;
}

const std::string& Mesh::getName() const {
	return m_name;
}

void Mesh::updateCameraCB(ConstantBuffer* cb) {
	cameraCB = cb;
}

ConstantBuffer* Mesh::getCameraCB() {
	return cameraCB;
}
