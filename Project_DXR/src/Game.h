#pragma once

#include "Application.h"

#include "Core/Texture2D.h"
#include "Core/Mesh.h"
#include "potatoFBXImporter/PotatoFBXImporter.h"

class Game : public Application {
public:
	Game();
	~Game();

	virtual void init() override;
	virtual void update(double dt) override;
	virtual void render(double dt) override;
	
private:
	std::unique_ptr<Technique> m_technique;
	std::unique_ptr<Texture2D> m_texture;
	std::unique_ptr<Sampler2D> m_sampler;
	std::unique_ptr<Material> m_material;
	std::unique_ptr<VertexBuffer> m_vertexBuffer;
	std::unique_ptr<Mesh> m_mesh;

	std::unique_ptr<PotatoFBXImporter> m_fbxImporter;

};