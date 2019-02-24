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
	m_fbxImporter = std::make_unique<PotatoFBXImporter>();
	PotatoModel* dino;
	dino = m_fbxImporter->importStaticModelFromScene("../assets/fbx/Dragon_Baked_Actions.fbx");
	

	float floorHalfWidth = 50.0f;
	float floorTiling = 5.0f;
	const Vertex floorVertices[] = {
		{XMFLOAT3(-floorHalfWidth, 0.f, -floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, floorTiling)},	// position, normal and UV
		{XMFLOAT3(floorHalfWidth,  0.f,  floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(floorTiling, 0.0f)},
		{XMFLOAT3(-floorHalfWidth, 0.f,  floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},

		{XMFLOAT3(-floorHalfWidth, 0.f, -floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, floorTiling)},
		{XMFLOAT3(floorHalfWidth, 0.f,  -floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(floorTiling, floorTiling)},
		{XMFLOAT3(floorHalfWidth,  0.f,  floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(floorTiling, 0.0f)},
	};
	float mirrorHalfWidth = 5.0f;
	float mirrorHalfHeight = 8.0f;
	const Vertex mirrorVertices[] = {
		{XMFLOAT3(-mirrorHalfWidth, -mirrorHalfHeight, 0.f ), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.0f, 1.0f)},	// position, normal and UV
		{XMFLOAT3(mirrorHalfWidth,   mirrorHalfHeight, 0.f ), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.0f, 0.0f)},
		{XMFLOAT3(-mirrorHalfWidth,  mirrorHalfHeight, 0.f ), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.0f, 0.0f)},

		{XMFLOAT3(-mirrorHalfWidth, -mirrorHalfHeight, 0.f ), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.0f, 1.0f)},
		{XMFLOAT3(mirrorHalfWidth,  -mirrorHalfHeight, 0.f ), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.0f, 1.0f)},
		{XMFLOAT3(mirrorHalfWidth,   mirrorHalfHeight, 0.f ), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.0f, 0.0f)},
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

	// create textures
	m_texture = std::unique_ptr<Texture2D>(getRenderer().makeTexture2D());
	m_texture->loadFromFile("../assets/textures/Dragon_ground_color.png");
	m_sampler = std::unique_ptr<Sampler2D>(getRenderer().makeSampler2D()); // Sampler does not work in RT mode
	m_sampler->setWrap(WRAPPING::REPEAT, WRAPPING::REPEAT);
	m_texture->sampler = m_sampler.get();

	m_floorTexture = std::unique_ptr<Texture2D>(getRenderer().makeTexture2D());
	m_floorTexture->loadFromFile("../assets/textures/floortilediffuse.png");
	m_floorTexture->sampler = m_sampler.get();

	size_t offset = 0;

	{
		// Set up mesh from FBX file
		// This is the first mesh and vertex buffer in the lists and therefor the ones modifiable via imgui
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex) * dino->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(&dino->getModelData()[0], sizeof(Vertex) * dino->getModelData().size(), offset);
		m_meshes.back()->setIAVertexBufferBinding(m_vertexBuffers.back().get(), offset, dino->getModelData().size(), sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->addTexture(m_texture.get(), TEX_REG_DIFFUSE_SLOT);
		delete dino;
	}

	{
		// Set up mirrror 1 mesh
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		constexpr auto numVertices = std::extent<decltype(mirrorVertices)>::value;
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(mirrorVertices), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(mirrorVertices, sizeof(mirrorVertices), offset);
		m_meshes.back()->setIAVertexBufferBinding(m_vertexBuffers.back().get(), offset, numVertices, sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->addTexture(m_floorTexture.get(), TEX_REG_DIFFUSE_SLOT);
	}
	{
		// Set up mirrror 2 mesh
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		constexpr auto numVertices = std::extent<decltype(mirrorVertices)>::value;
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(mirrorVertices), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(mirrorVertices, sizeof(mirrorVertices), offset);
		m_meshes.back()->setIAVertexBufferBinding(m_vertexBuffers.back().get(), offset, numVertices, sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->addTexture(m_floorTexture.get(), TEX_REG_DIFFUSE_SLOT);
	}

	{
		// Set up floor mesh
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		constexpr auto numVertices = std::extent<decltype(floorVertices)>::value;
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(floorVertices), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(floorVertices, sizeof(floorVertices), offset);
		m_meshes.back()->setIAVertexBufferBinding(m_vertexBuffers.back().get(), offset, numVertices, sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->addTexture(m_floorTexture.get(), TEX_REG_DIFFUSE_SLOT);
	}


	if (m_dxRenderer->isDXREnabled()) {
		// Update raytracing acceleration structures
		//m_dxRenderer->getDXR().setMeshes(static_cast<DX12Mesh*>(m_mesh.get()));
		m_dxRenderer->getDXR().setMeshes(m_meshes);
		m_dxRenderer->getDXR().useCamera(m_persCamera.get());
	}

}

void Game::update(double dt) {

	// Camera movement
	m_persCameraController->update(dt);

	if (Input::IsKeyPressed(VK_RETURN)) {
		auto& r = static_cast<DX12Renderer&>(getRenderer());
		r.enableDXR(!r.isDXREnabled());
	}

	if (Input::IsMouseButtonPressed(Input::MouseButton::RIGHT)) {
		Input::showCursor(Input::IsCursorHidden());
	}

	if (Input::IsKeyDown('F')) {
		Transform& mirrorTransform = m_meshes[1]->getTransform();

		XMVECTOR camPos = m_persCamera->getPositionVec();
		XMVECTOR pos = m_persCamera->getPositionVec() + m_persCamera->getDirectionVec() * 10.f;

		XMVECTOR d = XMVector3Normalize(pos - camPos);
		float dx = XMVectorGetX(d);
		float dy = XMVectorGetY(d);
		float dz = XMVectorGetZ(d);

		float pitch = asin(-dy);
		float yaw = atan2(dx, dz);

		XMMATRIX rotMat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(pitch, yaw, 0.f, 0.f)));
		XMMATRIX mat = rotMat * XMMatrixTranslationFromVector(pos);

		mirrorTransform.setTransformMatrix(mat);
		m_meshes[1]->setTransform(mirrorTransform);
	}
	if (Input::IsKeyDown('G')) {
		Transform& mirrorTransform = m_meshes[2]->getTransform();

		XMVECTOR camPos = m_persCamera->getPositionVec();
		XMVECTOR pos = m_persCamera->getPositionVec() + m_persCamera->getDirectionVec() * 10.f;

		XMVECTOR d = XMVector3Normalize(pos - camPos);
		float dx = XMVectorGetX(d);
		float dy = XMVectorGetY(d);
		float dz = XMVectorGetZ(d);

		float pitch = asin(-dy);
		float yaw = atan2(dx, dz);

		XMMATRIX rotMat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(pitch, yaw, 0.f, 0.f)));
		XMMATRIX mat = rotMat * XMMatrixTranslationFromVector(pos);

		mirrorTransform.setTransformMatrix(mat);
		m_meshes[2]->setTransform(mirrorTransform);
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
		shift += dt * 1.f;
	}
	if (shift >= XM_PI * 2.0f) shift -= XM_PI * 2.0f;

	XMVECTOR translation = XMVectorSet(cosf(shift), sinf(shift), 0.0f, 0.0f);
	Transform& t = m_meshes[0]->getTransform();
	t.setTranslation(translation);

	m_meshes[0]->setTransform(t); // Updates transform matrix for rasterisation
	// Update camera constant buffer for rasterisation
	for (auto& mesh : m_meshes)
		mesh->updateCamera(*m_persCamera);

	if (m_dxRenderer->isDXREnabled()) {
		auto instanceTransform = [&](int instanceID) {
			XMFLOAT3X4 m;
			XMStoreFloat3x4(&m, m_meshes[instanceID]->getTransform().getTransformMatrix());
			return m;
		};
		m_dxRenderer->getDXR().updateTLASnextFrame(instanceTransform); // Updates transform matrix for raytracing
	}


}

void Game::render(double dt) {
	//getRenderer().clearBuffer(CLEAR_BUFFER_FLAGS::COLOR | CLEAR_BUFFER_FLAGS::DEPTH); // Doesnt do anything

	for (auto& mesh : m_meshes)
		getRenderer().submit(mesh.get());

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
							m_vertexBuffers[0] = std::unique_ptr<VertexBuffer>(getRenderer().makeVertexBuffer(sizeof(Vertex) * model->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
							m_vertexBuffers[0]->setData(&model->getModelData()[0], sizeof(Vertex) * model->getModelData().size(), 0);
							m_meshes[0]->setIAVertexBufferBinding(m_vertexBuffers[0].get(), 0, model->getModelData().size(), sizeof(float) * 8); // 3 positions, 3 normals and 2 UVs

							if (m_dxRenderer->isDXRSupported()) {
								m_dxRenderer->getDXR().setMeshes(m_meshes);
							}

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
							// Set all meshes to this texture
							// Not possible to only set one since we dont know which ones are referencing the old, now deleted texture
							for (auto& mesh : m_meshes)
								mesh->addTexture(m_texture.get(), TEX_REG_DIFFUSE_SLOT);
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
		ImGui::Text("Controls\nEnter to toggle DXR on/off\nRight click to lock/unlock mouse\nWASD Space and C to move camera\nHold F to move mirror 1\nHold G to move mirror 2");
	}

	//ImGui::ShowDemoWindow();
}
