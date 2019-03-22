#pragma once

//#include <array>

#include "Application.h"

#include "Core/Texture2D.h"
#include "DX12/DX12Texture2DArray.h"
#include "Core/Mesh.h"
#include "Core/Camera.h"
#include "Core/CameraController.h"
#include "potatoFBXImporter/PotatoFBXImporter.h"
#include "GameObject.h"
#include "TimerSaver.h"

class DX12Renderer;
class DX12Mesh;

class Game : public Application {
public:
	Game();
	~Game();

	virtual void init() override;
	//virtual void init() override;
	// Updates every frame
	virtual void update(double dt) override;
	// Tries to update 60 times per second
	virtual void fixedUpdate(double dt) override;
	// Renders every frame
	virtual void render(double dt) override;

private:
	void imguiFunc();
	Transform getNextPosition();
	bool addObject(PotatoModel* model, int animationIndex, Transform& transform);
	
private:
	DX12Renderer* m_dxRenderer;

	std::unique_ptr<Camera> m_persCamera;
	std::unique_ptr<CameraController> m_persCameraController;

	std::unique_ptr<Technique> m_technique;
	/*std::unique_ptr<Texture2D> m_texture;
	std::unique_ptr<Texture2D> m_floorTexture;*/
	std::unique_ptr<DX12Texture2DArray> m_ballbotTexArray;
	std::unique_ptr<DX12Texture2DArray> m_cornellTexArray;
	std::unique_ptr<DX12Texture2DArray> m_dragonTexArray;
	std::unique_ptr<DX12Texture2DArray> m_floorTexArray;
	std::unique_ptr<DX12Texture2DArray> m_reflectTexArray;
	std::unique_ptr<DX12Texture2DArray> m_refractTexArray;
	std::unique_ptr<DX12Texture2DArray> m_diffuseTexArray;
	std::unique_ptr<DX12Texture2DArray> m_mirrorTextures;
	std::unique_ptr<Sampler2D> m_sampler;
	std::unique_ptr<Material> m_material;
	//std::unique_ptr<VertexBuffer> m_vertexBuffer;
	//std::unique_ptr<Mesh> m_mesh;

	std::vector<std::unique_ptr<VertexBuffer>> m_vertexBuffers;
	std::vector<std::unique_ptr<IndexBuffer>> m_indexBuffers;
	std::vector<std::unique_ptr<DX12Mesh>> m_meshes;

	std::unique_ptr<PotatoFBXImporter> m_fbxImporter;
	std::vector<PotatoModel*> m_models;
	float m_animationSpeed;
	size_t m_animatedModelsStartIndex;
	std::vector<GameObject> m_gameObjects;


	// ImGui
	std::string m_availableModels;
	std::vector<std::string> m_availableModelsList;

	std::string m_availableTextures;
	std::vector<std::string> m_availableTexturesList;

	std::unique_ptr<TimerSaver> m_timerSaver;
};