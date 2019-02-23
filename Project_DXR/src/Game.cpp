#include "pch.h"
#include "Game.h"
#include "DX12/DX12Renderer.h"
#include "DX12/DX12Mesh.h"
#include "Utils/Input.h"

Game::Game() 
	: Application(1280, 720, "DX12 DXR Raytracer thing with soon to come skinned animated models")
{
	m_dxRenderer = static_cast<DX12Renderer*>(&getRenderer());

	m_persCamera = std::make_unique<Camera>(m_dxRenderer->getWindow()->getWindowWidth() / (float)m_dxRenderer->getWindow()->getWindowHeight(), 110.f, 0.1f, 1000.f);
	m_persCamera->setPosition(XMVectorSet(0.f, 0.f, -2.f, 0.f));
	m_persCamera->setDirection(XMVectorSet(0.f, 0.f, 1.0f, 1.0f));
	m_persCameraController = std::make_unique<CameraController>(m_persCamera.get());

	getRenderer().setClearColor(0.2f, 0.4f, 0.2f, 1.0f);

	{
		std::string path = "../assets/fbx/";
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (!entry.is_regular_file()) continue;
			char buffer[50];
			wcstombs_s(nullptr, buffer, 50, entry.path().filename().c_str(), 50);
			m_availableModels += buffer;
			m_availableModels += '\0';
			m_availableModelsList.emplace_back(buffer);
		}
	}
	{
		std::string path = "../assets/textures/";
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (!entry.is_regular_file()) continue;
			char buffer[50];
			wcstombs_s(nullptr, buffer, 50, entry.path().filename().c_str(), 50);
			m_availableTextures += buffer;
			m_availableTextures += '\0';
			m_availableTexturesList.emplace_back(buffer);
		}
	}

}

Game::~Game() {
}

void Game::init() {
	// triangle geometry

	m_fbxImporter = std::make_unique<PotatoFBXImporter>();
	PotatoModel* dino;
	dino = m_fbxImporter->importStaticModelFromScene("../assets/fbx/Dragon_Baked_Actions.fbx");
	
	/*if(dino)
		delete dino;*/

	const Vertex vertices[] = {
		{XMFLOAT3(0,		1,	  0), XMFLOAT3(0, 0, -1), XMFLOAT2(0.5f, 0.0f)},	// Vertex, normal and UV
		{XMFLOAT3(0.866f,  -0.5f, 0), XMFLOAT3(0, 0, -1), XMFLOAT2(1.0f, 1.0f)},
		{XMFLOAT3(-0.866f, -0.5f, 0), XMFLOAT3(0, 0, -1), XMFLOAT2(0.0f, 1.0f)},

		{XMFLOAT3(0,       -0.5f, 1), XMFLOAT3(0, 0, -1), XMFLOAT2(0.5f, 0.0f)},
		{XMFLOAT3(0.866f,  -1.5f, 1), XMFLOAT3(0, 0, -1), XMFLOAT2(1.0f, 1.0f)},
		{XMFLOAT3(-0.866f, -1.5f, 1), XMFLOAT3(0, 0, -1), XMFLOAT2(0.0f, 1.0f)}
	};

	// load Materials.
	std::string shaderPath = getRenderer().getShaderPath();
	std::string shaderExtension = getRenderer().getShaderExtension();
	float diffuse[4] = {
		1.0, 1.0, 1.0, 1.0
	};

	m_material = std::unique_ptr<Material>(getRenderer().makeMaterial("material_0"));
	m_material->setShader(shaderPath + "VertexShader" + shaderExtension, Material::ShaderType::VS);
	m_material->setShader(shaderPath + "FragmentShader" + shaderExtension, Material::ShaderType::PS);
	std::string err;
	m_material->compileMaterial(err);

	// add a constant buffer to the material, to tint every triangle using this material
	m_material->addConstantBuffer("DiffuseTint", CB_REG_DIFFUSE_TINT, 4 * sizeof(float));
	// no need to update anymore
	// when material is bound, this buffer should be also bound for access.
	m_material->updateConstantBuffer(diffuse, CB_REG_DIFFUSE_TINT);

	// basic technique
	m_technique = std::unique_ptr<Technique>(getRenderer().makeTechnique(m_material.get(), getRenderer().makeRenderState()));

	// create texture
	m_texture = std::unique_ptr<Texture2D>(getRenderer().makeTexture2D());
	m_texture->loadFromFile("../assets/textures/Dragon_ground_color.png");
	m_sampler = std::unique_ptr<Sampler2D>(getRenderer().makeSampler2D()); // Sampler does not work in RT mode
	m_sampler->setWrap(WRAPPING::REPEAT, WRAPPING::REPEAT);
	m_texture->sampler = m_sampler.get();

	m_vertexBuffer = std::unique_ptr<VertexBuffer>(getRenderer().makeVertexBuffer(sizeof(Vertex) * dino->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
	//m_vertexBuffer = std::unique_ptr<VertexBuffer>(getRenderer().makeVertexBuffer(sizeof(vertices), VertexBuffer::DATA_USAGE::STATIC));

	m_mesh = std::unique_ptr<Mesh>(getRenderer().makeMesh());

	constexpr auto numberOfPosElements = std::extent<decltype(vertices)>::value;
	size_t offset = 0;
	m_vertexBuffer->setData(&dino->getModelData()[0], sizeof(Vertex) * dino->getModelData().size(), offset);
	//m_vertexBuffer->setData(vertices, sizeof(vertices), offset);
	m_mesh->setIAVertexBufferBinding(m_vertexBuffer.get(), offset, dino->getModelData().size(), sizeof(float) * 8); // 3 positions, 3 normals and 2 UVs
	
	// we can create a constant buffer outside the material, for example as part of the Mesh.

	m_mesh->technique = m_technique.get();
	m_mesh->addTexture(m_texture.get(), TEX_REG_DIFFUSE_SLOT);

	delete dino;

	if (m_dxRenderer->isDXREnabled()) {
		// Update raytracing acceleration structures
		m_dxRenderer->getDXR().setMesh(static_cast<DX12Mesh*>(m_mesh.get()));
		m_dxRenderer->getDXR().useCamera(m_persCamera.get());
	}

}

void Game::update(double dt) {

	/* Camera movement, Move this to main */
	m_persCameraController->update(dt);

	if (Input::IsKeyPressed(VK_RETURN)) {
		auto& r = static_cast<DX12Renderer&>(getRenderer());
		r.enableDXR(!r.isDXREnabled());
	}

	if (Input::IsMouseButtonPressed(Input::MouseButton::RIGHT)) {
		Input::showCursor(Input::IsCursorHidden());
	}

	// Lock mouse
	if (Input::IsCursorHidden()) {
		POINT p;
		p.x = reinterpret_cast<DX12Renderer*>(&getRenderer())->getWindow()->getWindowWidth() / 2;
		p.y = reinterpret_cast<DX12Renderer*>(&getRenderer())->getWindow()->getWindowHeight() / 2;
		ClientToScreen(*reinterpret_cast<DX12Renderer*>(&getRenderer())->getWindow()->getHwnd(), &p);
		SetCursorPos(p.x, p.y);
	}

	static float shift = 0.0f;
	if (dt < 10.0) {
		shift += dt * 0.001f;
	}
	if (shift >= XM_PI * 2.0f) shift -= XM_PI * 2.0f;


	XMVECTOR translation = XMVectorSet(cosf(shift), sinf(shift), 0.0f, 0.0f);
	Transform& t = m_mesh->getTransform();
	t.setTranslation(translation);
	//std::cout << t.getTranslation().x << std::endl;
	m_mesh->setTransform(t); // Updates transform matrix for rasterisation
	m_mesh->updateCamera(*m_persCamera); // Update camera constant buffer for rasterisation


	if (m_dxRenderer->isDXREnabled()) {
		auto instanceTransform = [&t](int instanceID) {
			XMFLOAT3X4 m;
			XMStoreFloat3x4(&m, t.getTransformMatrix());
			return m;
			/*XMFLOAT3X4 m;
			XMStoreFloat3x4(&m, XMMatrixIdentity());
			if (instanceID == 1) {
				XMStoreFloat3x4(&m, XMMatrixRotationY(instanceID * 0.25f) * XMMatrixTranslation(-1.0f + instanceID, 0, 0));
			} else {
				XMStoreFloat3x4(&m, XMMatrixRotationY(instanceID * 0.25f) * XMMatrixTranslation(-1.0f + instanceID, 0, 0));
			}
			return m;*/
		};
		m_dxRenderer->getDXR().updateTLASnextFrame(instanceTransform, 1); // Updates transform matrix for raytracing
	}


}

void Game::render(double dt) {
	//getRenderer().clearBuffer(CLEAR_BUFFER_FLAGS::COLOR | CLEAR_BUFFER_FLAGS::DEPTH); // Doesnt do anything

	getRenderer().submit(m_mesh.get());

	std::function<void()> imgui = std::bind(&Game::imguiFunc, this);
	m_dxRenderer->frame(imgui);

	getRenderer().present();
}

void Game::imguiFunc() {
	// Only display options if the window isn't collapsed
	if (ImGui::Begin("Options")) {
		std::string build = "Release";
#ifdef _DEBUG
		build = "Debug";
#endif
#ifdef _WIN64
		build += " x64";
#else
		build += " x86";
#endif
		ImGui::LabelText(build.c_str(), "Build");
		
		if (ImGui::TreeNode("Camera")) {

			ImGui::InputFloat3("Position", (float*)&m_persCamera->getPositionVec(), 2);
			ImGui::InputFloat3("Direction", (float*)&m_persCamera->getDirectionVec(), 2);
			ImGui::SliderFloat("Speed", &m_persCameraController->getMovementSpeed(), 1.f, 500.f);

			ImGui::TreePop();
			ImGui::Separator();
		}
		if (ImGui::TreeNode("Renderer")) {
			if (!m_availableModels.empty()) {
				static int currentModelIndex = -1; // If the selection isn't within 0..count, Combo won't display a preview
				if (ImGui::Combo("Model", &currentModelIndex, m_availableModels.c_str())) {
					std::cout << "selected: " << m_availableModelsList[currentModelIndex] << std::endl;

					auto updateVB = [&]() {
						try {
							PotatoModel* model = m_fbxImporter->importStaticModelFromScene("../assets/fbx/" + m_availableModelsList[currentModelIndex]);
							m_vertexBuffer = std::unique_ptr<VertexBuffer>(getRenderer().makeVertexBuffer(sizeof(Vertex) * model->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
							m_vertexBuffer->setData(&model->getModelData()[0], sizeof(Vertex) * model->getModelData().size(), 0);
							m_mesh->setIAVertexBufferBinding(m_vertexBuffer.get(), 0, model->getModelData().size(), sizeof(float) * 8); // 3 positions, 3 normals and 2 UVs

							if (m_dxRenderer->isDXRSupported())
								m_dxRenderer->getDXR().updateBLASnextFrame(false);

							delete model;
						} catch (...) {
							std::cout << "Error importing model" << std::endl;
						}
					};
					m_dxRenderer->executeNextOpenPreCommand(updateVB);
					
				}
				static int currentTextureIndex = -1; // If the selection isn't within 0..count, Combo won't display a preview
				if (ImGui::Combo("Texture", &currentTextureIndex, m_availableTextures.c_str())) {
					std::cout << "selected: " << m_availableTexturesList[currentTextureIndex] << std::endl;

					auto updateTexture = [&]() {
						try {
							m_texture = std::unique_ptr<Texture2D>(getRenderer().makeTexture2D());
							m_texture->loadFromFile("../assets/textures/" + m_availableTexturesList[currentTextureIndex]);
							m_texture->sampler = m_sampler.get();
							m_mesh->addTexture(m_texture.get(), TEX_REG_DIFFUSE_SLOT);
							if (m_dxRenderer->isDXRSupported())
								m_dxRenderer->getDXR().updateBLASnextFrame(true);
						} catch (...) {
							std::cout << "Error loading texture" << std::endl;
						}
					};
					m_dxRenderer->executeNextOpenPreCommand(updateTexture);

				}
			}

			if (m_dxRenderer->isDXRSupported()) {
				ImGui::Checkbox("DXR Enabled", &m_dxRenderer->isDXREnabled());
			}
			ImGui::TreePop();
			ImGui::Separator();
		}
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
		if (ImGui::TreeNode("Statistics")) {

			static bool paused = false;
			static float pausedCopy[FRAME_HISTORY_COUNT];
			static float pausedAvg;
			static float pausedLast;
			char buffer[100];
			sprintf_s(buffer, 100, "Frame Times\n\nLast:    %.2f ms\nAverage: %.2f ms", (paused) ? pausedLast : frameTimeHistory[FRAME_HISTORY_COUNT-1], (paused) ? pausedAvg : frameTimeAvg);
			ImGui::PlotLines(buffer, (paused) ? pausedCopy : frameTimeHistory, FRAME_HISTORY_COUNT, 0, "", 0.f, 10.f, ImVec2(0, 100));
			if (ImGui::Button((paused) ? "Resume" : "Pause")) {
				paused = !paused;
				if (paused) {
					memcpy(pausedCopy, frameTimeHistory, FRAME_HISTORY_COUNT * sizeof(float));
					pausedAvg = frameTimeAvg;
					pausedLast = frameTimeHistory[FRAME_HISTORY_COUNT - 1];
				}
			}

			ImGui::TreePop();
			ImGui::Separator();
		}
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
	}

	ImGui::ShowDemoWindow();
}
