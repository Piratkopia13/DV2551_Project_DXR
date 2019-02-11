#pragma once

class Mesh;

class Scene
{
public:
	Scene();
	~Scene();

	// add a mesh to the scene, when the scene is traversed for rendering
	// this mesh will be rendered.
	virtual void addMesh(Mesh* mesh) = 0;

	// let the scene know that a mesh has been updated
	virtual void updateMesh(Mesh* mesh) = 0;

};

