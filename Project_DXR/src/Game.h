#pragma once

//#include <array>

#include "Application.h"

#include "Core/Texture2D.h"
#include "DX12/DX12Texture2DArray.h"
#include "Core/Mesh.h"
#include "Core/Camera.h"
#include "Core/CameraController.h"
#include "potatoFBXImporter/PotatoFBXImporter.h"

class DX12Renderer;
class DX12Mesh;

class Game : public Application {
public:
	Game();
	~Game();

	virtual void init() override;
	virtual void update(double dt) override;
	virtual void render(double dt) override;

private:
	void imguiFunc();
	
private:
	DX12Renderer* m_dxRenderer;

	std::unique_ptr<Camera> m_persCamera;
	std::unique_ptr<CameraController> m_persCameraController;

	std::unique_ptr<Technique> m_technique;
	/*std::unique_ptr<Texture2D> m_texture;
	std::unique_ptr<Texture2D> m_floorTexture;*/
	std::unique_ptr<DX12Texture2DArray> m_testTexArray;
	std::unique_ptr<Sampler2D> m_sampler;
	std::unique_ptr<Material> m_material;
	//std::unique_ptr<VertexBuffer> m_vertexBuffer;
	//std::unique_ptr<Mesh> m_mesh;

	std::vector<std::unique_ptr<VertexBuffer>> m_vertexBuffers;
	std::vector<std::unique_ptr<DX12Mesh>> m_meshes;

	std::unique_ptr<PotatoFBXImporter> m_fbxImporter;

	// ImGui
	std::string m_availableModels;
	std::vector<std::string> m_availableModelsList;

	std::string m_availableTextures;
	std::vector<std::string> m_availableTexturesList;
};