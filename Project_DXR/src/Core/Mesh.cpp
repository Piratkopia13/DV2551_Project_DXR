#include "pch.h"
#include "Mesh.h"

Mesh::Mesh() { };

/*
	buffer: is a VertexBuffer*
	offset: offset of the first byte in the buffer used when binding
	size: how many elements (how many points, normals, UVs) should be read from this buffer
*/
void Mesh::setIAVertexBufferBinding(VertexBuffer* buffer, size_t offset, size_t numElements, size_t sizeElement) {
	// inputStream is unique (has to be!) for this Mesh
	buffer->incRef();
	geometryBuffer = { sizeElement, numElements, offset, buffer };
};

void Mesh::bindIAVertexBuffer(unsigned int location) {
	// no checking if the key is valid...TODO
	geometryBuffer.buffer->bind(geometryBuffer.offset, geometryBuffer.numElements * geometryBuffer.sizeElement, location);
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
	transformCB->setData(&transform.getTransformMatrix(), sizeof(transform.getTransformMatrix()), CB_REG_TRANSFORM);
}

ConstantBuffer* Mesh::getTransformCB() {
	return transformCB;
}

Mesh::~Mesh() {
	geometryBuffer.buffer->decRef();
	delete transformCB;
}

void Mesh::updateCameraCB(ConstantBuffer* cb) {
	cameraCB = cb;
}

ConstantBuffer* Mesh::getCameraCB() {
	return cameraCB;
}
