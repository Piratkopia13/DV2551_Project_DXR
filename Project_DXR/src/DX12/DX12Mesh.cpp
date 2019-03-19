#include "pch.h"
#include "DX12Mesh.h"
#include "DX12VertexBuffer.h"
#include "DX12IndexBuffer.h"
#include "DX12Renderer.h"

DX12Mesh::DX12Mesh(DX12Renderer* renderer) 
	: m_materialCB("mesh_material_cb", sizeof(MaterialProperties), renderer)
{
	transformCB = renderer->makeConstantBuffer("Transform", sizeof(DirectX::XMMATRIX));
	transformCB->setData(&transform.getTransformMatrix(), CB_REG_TRANSFORM);

	// Material defaults
	m_matProps.maxRecursionDepth = 3;
	m_matProps.reflectionAttenuation = 0.6f;
	m_matProps.fuzziness = 0.03f;
	m_matProps.albedoColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
	setProperties(m_matProps);
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

const DX12ConstantBuffer* DX12Mesh::getMaterialCB() const {
	return &m_materialCB;
}

const MaterialProperties& DX12Mesh::getProperties() const {
	return m_matProps;
}

void DX12Mesh::setProperties(const MaterialProperties& props) {
	m_matProps = props;
	m_materialCB.setData(&m_matProps, 0);
	m_materialCB.forceUpdate(0);
}

DX12Texture2DArray * DX12Mesh::getTexture2DArray() const {
	return m_texArr;
}

void DX12Mesh::setTexture2DArray(DX12Texture2DArray * texArr) {
	m_texArr = texArr;
}