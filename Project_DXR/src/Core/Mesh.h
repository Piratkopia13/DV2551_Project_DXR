#pragma once
#include <unordered_map>
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Technique.h"
#include "../Geometry/Transform.h"
#include "ConstantBuffer.h"
#include "Texture2D.h"
#include "../Core/Camera.h"

class Mesh
{
public:
	Mesh(const std::string& name = "Unnamed");
	~Mesh();

	void setName(const std::string& name);
	const std::string& getName() const;

	// technique has: Material, RenderState, Attachments (color, depth, etc)
	Technique* technique; 

	// Not used since vertices are interleaved now
	// TODO: remove?
	struct VertexBufferBind {
		size_t sizeElement, numVertices, numIndices, offset;
		VertexBuffer* vBuffer;
		IndexBuffer* iBuffer;
	};
	
	void addTexture(Texture2D* texture, unsigned int slot);

	Transform& getTransform();
	void setTransform(Transform& transform);
	ConstantBuffer* getTransformCB();

	void updateCameraCB(ConstantBuffer* cb);
	ConstantBuffer* getCameraCB();

	// array of buffers with locations (binding points in shaders)
	virtual void setIABinding(VertexBuffer* vBuffer, IndexBuffer* iBuffer, size_t offset, size_t numVertices, size_t numIndices, size_t sizeElement);

	virtual void bindIA(unsigned int location);
	//std::unordered_map<unsigned int, VertexBufferBind> geometryBuffers;
	VertexBufferBind geometryBuffer;
	std::unordered_map<unsigned int, Texture2D*> textures;

protected:
	// Transform buffer
	ConstantBuffer* transformCB;
	// Camera buffer
	ConstantBuffer* cameraCB;
	// local copy of the transform
	Transform transform;

private:
	std::string m_name;


};
