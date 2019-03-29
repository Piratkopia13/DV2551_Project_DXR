#include "pch.h"
#include "Game.h"
#include "DX12/DX12Renderer.h"
#include "DX12/DX12Material.h"
#include "DX12/DX12Mesh.h"
#include "DX12/DX12VertexBuffer.h"
#include "Utils/Input.h"

Game::Game() 
	: Application(1700, 900, "Loading.. DX12 DXR Raytracer with skinned animated models")
{
	m_dxRenderer = static_cast<DX12Renderer*>(&getRenderer());
	m_timerSaver = std::make_unique<TimerSaver>(10000, m_dxRenderer);

	#ifdef PERFORMANCE_TESTING
	m_persCamera = std::make_unique<Camera>(m_dxRenderer->getWindow()->getWindowWidth() / (float)m_dxRenderer->getWindow()->getWindowHeight(), 110.f, 0.1f, 1000.f);
	m_persCamera->setPosition(XMVectorSet(-20.0f, 30.0f, 0.0f, 0.f));
	m_persCamera->setDirection(XMVectorSet(1.0f, -0.4f, 0.0f, 1.0f));
	m_persCameraController = std::make_unique<CameraController>(m_persCamera.get(), m_persCamera->getDirectionVec());

	if (m_dxRenderer->isDXREnabled()) {
		m_dxRenderer->getDXR().getRTFlags() &= ~RT_ENABLE_TA;
		m_dxRenderer->getDXR().getRTFlags() &= ~RT_ENABLE_JITTER_AA;
	}
	#else
	m_persCamera = std::make_unique<Camera>(m_dxRenderer->getWindow()->getWindowWidth() / (float)m_dxRenderer->getWindow()->getWindowHeight(), 110.f, 0.1f, 1000.f);
	m_persCamera->setPosition(XMVectorSet(7.37f, 12.44f, 13.5f, 0.f));
	m_persCamera->setDirection(XMVectorSet(0.17f, -0.2f, -0.96f, 1.0f));
	m_persCameraController = std::make_unique<CameraController>(m_persCamera.get(), m_persCamera->getDirectionVec());
	#endif
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

	m_timerSaver->saveToFile();

	for (int i = 0; i < m_models.size(); i++) {
		if (m_models[i])
			delete m_models[i];
	}
}

void Game::init() {
	m_animationSpeed = 1.0f;
	m_fbxImporter = std::make_unique<PotatoFBXImporter>();
	PotatoModel* roboModel;
	std::cout << "Loading fbx models.." << std::endl;
	#ifdef PERFORMANCE_TESTING
	#ifndef _DEBUG
	roboModel= m_fbxImporter->importStaticModelFromScene("../assets/fbx/ballbot3.fbx");
	#else
	roboModel= m_fbxImporter->importStaticModelFromScene("../assets/fbx/ScuffedSteve_2.fbx");
	#endif

	if (!roboModel) {
		std::cout << "NO ROBO" << std::endl;
	}
	m_models.push_back(roboModel);
	

	std::string shaderPath = getRenderer().getShaderPath();
	std::string shaderExtension = getRenderer().getShaderExtension();
	float diffuse[4] = {
		1.0, 1.0, 1.0, 1.0
	};

	m_material = std::unique_ptr<Material>(getRenderer().makeMaterial("material_0"));
	m_material->setShader(shaderPath + "VertexShader" + shaderExtension, Material::ShaderType::VS);
	m_material->setShader(shaderPath + "FragmentShader" + shaderExtension, Material::ShaderType::PS);
	DX12Material* dxMaterial = ((DX12Material*)m_material.get());
	std::string err;
	dxMaterial->compileMaterial(err);

	// add a constant buffer to the material, to tint every triangle using this material
	m_material->addConstantBuffer("DiffuseTint", CB_REG_DIFFUSE_TINT, 4 * sizeof(float));
	// no need to update anymore
	// when material is bound, this buffer should be also bound for access.
	m_material->updateConstantBuffer(diffuse, CB_REG_DIFFUSE_TINT);

	// basic technique
	m_technique = std::unique_ptr<Technique>(getRenderer().makeTechnique(m_material.get(), getRenderer().makeRenderState()));


	m_ballbotTexArray = std::unique_ptr<DX12Texture2DArray>(new DX12Texture2DArray(static_cast<DX12Renderer*>(&getRenderer())));
	std::vector<std::string> texFiles;
	texFiles.clear();
	texFiles.emplace_back("../assets/textures/ballbot_lowres.png");
	m_ballbotTexArray->loadFromFiles(texFiles);




	m_animatedModelsStartIndex = (unsigned int)m_meshes.size();
	int STARTOBJECTS = 1;
	for (int i = 0; i < STARTOBJECTS; i++) {
		Transform tt = getNextPosition();
#ifndef _DEBUG
		XMFLOAT3 scale = XMFLOAT3(0.03f, 0.03f, 0.03f);
		tt.setScale(XMLoadFloat3(&scale));
#endif
		addObject(m_models[0], 0, tt);
	}



	#else


#ifdef _DEBUG
	roboModel= m_fbxImporter->importStaticModelFromScene("../assets/fbx/ScuffedSteve_2.fbx");
#else
	roboModel= m_fbxImporter->importStaticModelFromScene("../assets/fbx/ballbot3.fbx");
#endif
	
	m_models.push_back(roboModel);
	for (int i = 0; i < roboModel->getStackSize(); i++) {
#ifdef _DEBUG
		XMFLOAT3 rotation{ -XM_PIDIV2, 0, 0 };
		XMFLOAT3 position = { (i)*15.0f, 10.0f, 0.0f};
		XMFLOAT3 scale = { 2.0f, 2.0f, 2.0f };
#else
		XMFLOAT3 rotation{ 0,0,0 };
		XMFLOAT3 position = { (i) * 15.0f - 40.f, 0.0f, 36.0f };
		XMFLOAT3 scale = { 0.03f, 0.03f, 0.03f };
#endif
		m_gameObjects.push_back(GameObject(m_models.back(), i, position, rotation, scale));
	}

	float floorHalfWidth = 50.0f;
	float floorTiling = 5.0f;
	const Vertex floorVertices[] = {
			{XMFLOAT3(-floorHalfWidth, 0.f, -floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, floorTiling)},	// position, normal and UV
			{XMFLOAT3(-floorHalfWidth, 0.f,  floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
			{XMFLOAT3(floorHalfWidth,  0.f,  floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(floorTiling, 0.0f)},
			{XMFLOAT3(floorHalfWidth, 0.f,  -floorHalfWidth), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(floorTiling, floorTiling)},
	};
	const unsigned int floorIndices[] {
		0, 1, 2, 0, 2, 3
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
	DX12Material* dxMaterial = ((DX12Material*)m_material.get());
	std::string err;
	dxMaterial->compileMaterial(err);

	// add a constant buffer to the material, to tint every triangle using this material
	m_material->addConstantBuffer("DiffuseTint", CB_REG_DIFFUSE_TINT, 4 * sizeof(float));
	// no need to update anymore
	// when material is bound, this buffer should be also bound for access.
	m_material->updateConstantBuffer(diffuse, CB_REG_DIFFUSE_TINT);

	// basic technique
	m_technique = std::unique_ptr<Technique>(getRenderer().makeTechnique(m_material.get(), getRenderer().makeRenderState()));
	
	// create textures
	DX12Texture2DArray* floorTexture = new DX12Texture2DArray(static_cast<DX12Renderer*>(&getRenderer()));
	m_floorTexArray = std::unique_ptr<DX12Texture2DArray>(floorTexture);
	std::vector<std::string> texFiles;
	texFiles.emplace_back("../assets/textures/floortilediffuse.png");
	m_floorTexArray->loadFromFiles(texFiles);

	DX12Texture2DArray* reflectTexture = new DX12Texture2DArray(static_cast<DX12Renderer*>(&getRenderer()));
	m_reflectTexArray = std::unique_ptr<DX12Texture2DArray>(reflectTexture);
	texFiles.clear();
	texFiles.emplace_back("../assets/textures/white.png");
	texFiles.emplace_back("../assets/textures/reflect.png");
	m_reflectTexArray->loadFromFiles(texFiles);

	DX12Texture2DArray* refractTexture = new DX12Texture2DArray(static_cast<DX12Renderer*>(&getRenderer()));
	m_refractTexArray = std::unique_ptr<DX12Texture2DArray>(refractTexture);
	texFiles.clear();
	texFiles.emplace_back("../assets/textures/white.png");
	texFiles.emplace_back("../assets/textures/refract.png");
	m_refractTexArray->loadFromFiles(texFiles);

	DX12Texture2DArray* mirrorTextures = new DX12Texture2DArray(static_cast<DX12Renderer*>(&getRenderer()));
	m_mirrorTextures = std::unique_ptr<DX12Texture2DArray>(mirrorTextures);
	texFiles.clear();
	texFiles.emplace_back("../assets/textures/diffuse-mirror.png");
	texFiles.emplace_back("../assets/textures/mat-mirror.png");
	m_mirrorTextures->loadFromFiles(texFiles);

	m_dragonTexArray = std::unique_ptr<DX12Texture2DArray>(new DX12Texture2DArray(static_cast<DX12Renderer*>(&getRenderer())));
	texFiles.clear();
	texFiles.emplace_back("../assets/textures/Dragon_ground_color.png");
	m_dragonTexArray->loadFromFiles(texFiles);

	m_cornellTexArray = std::unique_ptr<DX12Texture2DArray>(new DX12Texture2DArray(static_cast<DX12Renderer*>(&getRenderer())));
	texFiles.clear();
	texFiles.emplace_back("../assets/textures/cornell.png");
	m_cornellTexArray->loadFromFiles(texFiles);

	m_ballbotTexArray = std::unique_ptr<DX12Texture2DArray>(new DX12Texture2DArray(static_cast<DX12Renderer*>(&getRenderer())));
	texFiles.clear();
	texFiles.emplace_back("../assets/textures/ballbot_lowres.png");
	m_ballbotTexArray->loadFromFiles(texFiles);

	MaterialProperties mirrorMatProps;
	mirrorMatProps.maxRecursionDepth = 30;
	mirrorMatProps.reflectionAttenuation = 0.0f;
	XMStoreFloat3(&mirrorMatProps.albedoColor, XMVectorSet(1.f, 1.f, 1.f, 1.f));

	size_t offset = 0;
	PotatoModel* mirrorModel = m_fbxImporter->importStaticModelFromScene("../assets/fbx/mirror.fbx");
	{
		// Set up mirrror 1 mesh
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_meshes.back()->setName("Mirror 1");
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex)* mirrorModel->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(&mirrorModel->getModelData()[0], sizeof(Vertex)* mirrorModel->getModelData().size(), offset);
		m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(unsigned int)* mirrorModel->getModelIndices().size(), IndexBuffer::STATIC));
		m_indexBuffers.back()->setData(&mirrorModel->getModelIndices()[0], sizeof(unsigned int)* mirrorModel->getModelIndices().size(), offset);
		m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, mirrorModel->getModelVertices().size(), mirrorModel->getModelIndices().size(), sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->setTexture2DArray(m_mirrorTextures.get());
		m_meshes.back()->getTransform().translate(XMVectorSet(0.0f, -1000.0f, 0.f, 0.0f));
		m_meshes.back()->setTransform(m_meshes.back()->getTransform()); // To update rasterisation CB
		m_meshes.back()->setProperties(mirrorMatProps);
	}
	{
		// Set up mirrror 2 mesh
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_meshes.back()->setName("Mirror 2");
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex)* mirrorModel->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(&mirrorModel->getModelData()[0], sizeof(Vertex)* mirrorModel->getModelData().size(), offset);
		m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(unsigned int)* mirrorModel->getModelIndices().size(), IndexBuffer::STATIC));
		m_indexBuffers.back()->setData(&mirrorModel->getModelIndices()[0], sizeof(unsigned int)* mirrorModel->getModelIndices().size(), offset);
		m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, mirrorModel->getModelVertices().size(), mirrorModel->getModelIndices().size(), sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->setTexture2DArray(m_mirrorTextures.get());
		m_meshes.back()->getTransform().translate(XMVectorSet(0.0f, -1000.0f, 0.f, 0.0f));
		m_meshes.back()->setTransform(m_meshes.back()->getTransform()); // To update rasterisation CB
		m_meshes.back()->setProperties(mirrorMatProps);
	}
	delete mirrorModel;

	{
		// Set up floor mesh
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_meshes.back()->setName("Floor");
		constexpr auto numVertices = std::extent<decltype(floorVertices)>::value;
		constexpr auto numIndices = std::extent<decltype(floorIndices)>::value;
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(floorVertices), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(floorVertices, sizeof(floorVertices), offset);
		m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(floorIndices), IndexBuffer::STATIC));
		m_indexBuffers.back()->setData(floorIndices, sizeof(floorIndices), offset);
		m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, numVertices, numIndices, sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->setTexture2DArray(m_floorTexArray.get());
	}

#ifndef _DEBUG

	// Reflection and refraction models
	{
		// Set up mesh from FBX file
		PotatoModel* model = m_fbxImporter->importStaticModelFromScene("../assets/fbx/sphere.fbx");
		
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_meshes.back()->setName("Reflection ball");
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex) * model->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(&model->getModelData()[0], sizeof(Vertex) * model->getModelData().size(), offset);
		m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(unsigned int) * model->getModelIndices().size(), IndexBuffer::STATIC));
		m_indexBuffers.back()->setData(&model->getModelIndices()[0], sizeof(unsigned int) * model->getModelIndices().size(), offset);
		m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, model->getModelVertices().size(), model->getModelIndices().size(), sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->setTexture2DArray(m_reflectTexArray.get());
		Transform t = m_meshes.back()->getTransform();
		t.setTranslation(DirectX::XMVectorSet(15.0f, 16.f, 0.f, 1.0f));
		m_meshes.back()->setTransform(t);

		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_meshes.back()->setName("Refraction ball");
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex) * model->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(&model->getModelData()[0], sizeof(Vertex) * model->getModelData().size(), offset);
		m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(unsigned int) * model->getModelIndices().size(), IndexBuffer::STATIC));
		m_indexBuffers.back()->setData(&model->getModelIndices()[0], sizeof(unsigned int) * model->getModelIndices().size(), offset);
		m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, model->getModelVertices().size(), model->getModelIndices().size(), sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->setTexture2DArray(m_refractTexArray.get());
		t = m_meshes.back()->getTransform();
		t.setTranslation(DirectX::XMVectorSet(-15.0f, 16.f, 0.f, 1.0f));
		m_meshes.back()->setTransform(t);

		delete model;
	}

	{
		// Dragon mesh
		PotatoModel* model = m_fbxImporter->importStaticModelFromScene("../assets/fbx/Dragon.fbx");
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_meshes.back()->setName("Dragon");
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex) * model->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(&model->getModelData()[0], sizeof(Vertex) * model->getModelData().size(), offset);
		m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(unsigned int) * model->getModelIndices().size(), IndexBuffer::STATIC));
		m_indexBuffers.back()->setData(&model->getModelIndices()[0], sizeof(unsigned int) * model->getModelIndices().size(), offset);
		m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, model->getModelVertices().size(), model->getModelIndices().size(), sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->setTexture2DArray(m_dragonTexArray.get());
		m_meshes.back()->getTransform().scaleUniformly(0.3f);
		m_meshes.back()->setTransform(m_meshes.back()->getTransform()); // To update rasterisation CB
		auto props = m_meshes.back()->getProperties();
		props.reflectionAttenuation = 0.99f;
		props.maxRecursionDepth = 1;
		m_meshes.back()->setProperties(props);
		delete model;
	}
	{
		// Cornell box
		PotatoModel* model = m_fbxImporter->importStaticModelFromScene("../assets/fbx/cornell_box.fbx");
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_meshes.back()->setName("Cornell box");
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex) * model->getModelData().size(), VertexBuffer::DATA_USAGE::STATIC));
		m_vertexBuffers.back()->setData(&model->getModelData()[0], sizeof(Vertex) * model->getModelData().size(), offset);
		m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(unsigned int) * model->getModelIndices().size(), IndexBuffer::STATIC));
		m_indexBuffers.back()->setData(&model->getModelIndices()[0], sizeof(unsigned int) * model->getModelIndices().size(), offset);
		m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, model->getModelVertices().size(), model->getModelIndices().size(), sizeof(Vertex));
		m_meshes.back()->technique = m_technique.get();
		m_meshes.back()->setTexture2DArray(m_cornellTexArray.get());
		m_meshes.back()->getTransform().translate(XMVectorSet(0.0f, 0.01f, 0.f, 0.0f));
		m_meshes.back()->getTransform().setScale(XMVectorSet(1.7f, 1.f, 1.f, 1.f));
		m_meshes.back()->setTransform(m_meshes.back()->getTransform()); // To update rasterisation CB
		delete model;
	}
#endif

	m_animatedModelsStartIndex = (unsigned int)m_meshes.size();

	for (int i = 0; i < m_gameObjects.size(); i++) {
		// Set up multiple meshes from a single fbx model
		m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
		m_meshes.back()->setName("Animated " + std::to_string(i));

		// Vertex buffer
		m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex) * m_gameObjects[i].getModel()->getModelVertices().size(), VertexBuffer::STATIC));
		m_vertexBuffers.back()->setData(m_gameObjects[i].getModel()->getModelVertices().data(), sizeof(Vertex) * m_gameObjects[i].getModel()->getModelVertices().size(), offset);
		// Index buffer
		m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(unsigned int) * m_gameObjects[i].getModel()->getModelIndices().size(), IndexBuffer::STATIC));
		m_indexBuffers.back()->setData(m_gameObjects[i].getModel()->getModelIndices().data(), sizeof(unsigned int) * m_gameObjects[i].getModel()->getModelIndices().size(), offset);
		// Binding
		m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, m_gameObjects[i].getModel()->getModelVertices().size(), m_gameObjects[i].getModel()->getModelIndices().size(), sizeof(Vertex));

		// Technique
		m_meshes.back()->technique = m_technique.get();
		// Texture
		//m_meshes.back()->addTexture(m_ballBotTexture.get(), TEX_REG_DIFFUSE_SLOT);
		m_meshes.back()->setTexture2DArray(m_ballbotTexArray.get());
		//delete roboModel;
	}

	#endif

	if (m_dxRenderer->isDXREnabled()) {
		// Update raytracing acceleration structures
		m_dxRenderer->getDXR().setMeshes(m_meshes);
		m_dxRenderer->getDXR().useCamera(m_persCamera.get());
	}

	static_cast<DX12Renderer*>(&getRenderer())->useCamera(m_persCamera.get());

	m_persCamera->updateConstantBuffer();

}

void Game::update(double dt) {

	#ifdef PERFORMANCE_TESTING
	const int step = 10;
	const unsigned int framesToSave = 500;
	static double acc = 0.0;
	acc += dt;
#ifdef _DEBUG
	if (Input::IsKeyPressed('L')) {
#else
	if (m_timerSaver->getSizeOf("BLAS", (int)m_gameObjects.size()) >= framesToSave) {
#endif
		acc = 0.0;
		
		m_dxRenderer->executeNextOpenPreCommand([&, step]() {
			m_dxRenderer->waitForGPU();
			
			int toAdd = step;
			if (m_gameObjects.size() < 10) {
				toAdd = 1;
			} else {
				toAdd = step;
			}

			for (int i = 0; i < toAdd; i++) {
				Transform tt = getNextPosition();
#ifndef _DEBUG
				XMFLOAT3 scale = XMFLOAT3(0.03f, 0.03f, 0.03f);
				tt.setScale(XMLoadFloat3(&scale));
#endif
				addObject(m_models[0], 0, tt);
			}
			std::cout << "Num objects: " << m_gameObjects.size() << std::endl;
		});
			
	}


	#else

	if (Input::IsMouseButtonPressed(Input::MouseButton::RIGHT)) {
		Input::showCursor(Input::IsCursorHidden());
	}
	// Lock mouseaaw
	if (Input::IsCursorHidden()) {
		POINT p;
		p.x = reinterpret_cast<DX12Renderer*>(&getRenderer())->getWindow()->getWindowWidth() / 2;
		p.y = reinterpret_cast<DX12Renderer*>(&getRenderer())->getWindow()->getWindowHeight() / 2;
		ClientToScreen(*reinterpret_cast<DX12Renderer*>(&getRenderer())->getWindow()->getHwnd(), &p);
		SetCursorPos(p.x, p.y);
	}


	if (Input::IsKeyDown('F')) {
		Transform& mirrorTransform = m_meshes[0]->getTransform();

		XMVECTOR camPos = m_persCamera->getPositionVec();
		XMVECTOR pos = m_persCamera->getPositionVec() + m_persCamera->getDirectionVec() * 10.f;

		XMVECTOR d = XMVector3Normalize(pos - camPos);
		float dx = XMVectorGetX(d);
		float dy = XMVectorGetY(d);
		float dz = XMVectorGetZ(d);

		float pitch = asin(-dy);
		float yaw = atan2(dx, dz);
		XMMATRIX scale = XMMatrixScalingFromVector(XMLoadFloat3(&mirrorTransform.getScale()));
		XMMATRIX rotMat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(pitch, yaw, 0.f, 0.f)));
		XMMATRIX mat = scale * rotMat * XMMatrixTranslationFromVector(pos);

		mirrorTransform.setTransformMatrix(mat);
		m_meshes[0]->setTransform(mirrorTransform);
	}
	if (Input::IsKeyDown('G')) {
		Transform& mirrorTransform = m_meshes[1]->getTransform();

		XMVECTOR camPos = m_persCamera->getPositionVec();
		XMVECTOR pos = m_persCamera->getPositionVec() + m_persCamera->getDirectionVec() * 10.f;

		XMVECTOR d = XMVector3Normalize(pos - camPos);
		float dx = XMVectorGetX(d);
		float dy = XMVectorGetY(d);
		float dz = XMVectorGetZ(d);

		float pitch = asin(-dy);
		float yaw = atan2(dx, dz);

		XMMATRIX scale = XMMatrixScalingFromVector(XMLoadFloat3(&mirrorTransform.getScale()));
		XMMATRIX rotMat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(pitch, yaw, 0.f, 0.f)));
		XMMATRIX mat = scale * rotMat * XMMatrixTranslationFromVector(pos);

		mirrorTransform.setTransformMatrix(mat);
		m_meshes[1]->setTransform(mirrorTransform);
	}

	// Camera movement
	m_persCameraController->update(float(dt));

	m_persCamera->updateConstantBuffer();

	// Update camera constant buffer for rasterisation
	#endif
	for (auto& mesh : m_meshes)
		mesh->updateCameraCB((ConstantBuffer*)(m_persCamera->getConstantBuffer())); // Update camera constant buffer for rasterisation

	if (m_dxRenderer->isDXREnabled()) {
		auto instanceTransform = [&](int instanceID) {
			XMFLOAT3X4 m;
			XMStoreFloat3x4(&m, m_meshes[instanceID]->getTransform().getTransformMatrix());
			return m;
		};
		// Update transform matrices for raytracing
		m_dxRenderer->getDXR().updateTLASnextFrame(instanceTransform);

		auto io = ImGui::GetIO();
		if (io.MouseDown[0] || std::any_of(std::begin(io.KeysDown), std::end(io.KeysDown), [](bool k) { return k; })) {
			// Reset temporal accumulation count when states affecting how the rendered image looks may have changed
			m_dxRenderer->getDXR().getTemporalAccumulationCount() = 0;
		}
	}
	
	// CPU Skinning - this is SLOW!
	if (m_animationSpeed > 0.0f) {
		for (unsigned int i = 0; i < m_gameObjects.size(); i++) {
			PotatoModel* pModel = m_gameObjects[i].getModel();
			m_gameObjects[i].update(float(dt) * m_animationSpeed);
			m_meshes[m_animatedModelsStartIndex + i]->setTransform(m_gameObjects[i].getTransform());

			if (m_dxRenderer->isDXRSupported())
				m_dxRenderer->getDXR().updateBLASnextFrame(true);

			m_dxRenderer->executeNextOpenCopyCommand([&, pModel, i] {
				auto cmdList = m_dxRenderer->getCopyList();
				auto* vb = static_cast<DX12VertexBuffer*>(m_vertexBuffers[m_animatedModelsStartIndex + i].get());
				vb->updateData(pModel->getMesh((int)m_gameObjects[i].getAnimationIndex(), m_gameObjects[i].getAnimationTime()).data(), sizeof(Vertex) * pModel->getModelVertices().size());
			});
		}
	}

}

void Game::fixedUpdate(double dt) {
	// Runs at 60 ticks/sec regardless of fps
}


void Game::render(double dt) {
	// Submit rasterization meshes
	for (auto& mesh : m_meshes)
		getRenderer().submit(mesh.get());

	// Render frame with gui
	std::function<void()> imgui = std::bind(&Game::imguiFunc, this);
	m_dxRenderer->frame(imgui);

	getRenderer().present();

#ifdef PERFORMANCE_TESTING
	UINT64 queueFreq;
	m_dxRenderer->getCmdQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)->GetTimestampFrequency(&queueFreq);
	double timestampToMs = (1.0 / queueFreq) * 1000.0;

	auto getMsTime = [&](Timers::Gpu timer) {
		D3D12::GPUTimestampPair updateTime = m_dxRenderer->getTimer().getTimestampPair(timer);
		UINT64 timerDt = updateTime.Stop - updateTime.Start;
		return timerDt * timestampToMs;
	};
#ifndef _DEBUG

	m_timerSaver->addResult("BLAS", int(m_gameObjects.size()), getMsTime(Timers::BLAS));
	m_timerSaver->addResult("TLAS", int(m_gameObjects.size()), getMsTime(Timers::TLAS));
	m_timerSaver->addResult("RAYS", int(m_gameObjects.size()), getMsTime(Timers::DISPATCHRAYS));
	m_timerSaver->addResult("DXRCOPY", int(m_gameObjects.size()), getMsTime(Timers::DXRCOPY));
	m_timerSaver->addResult("VRAM", int(m_gameObjects.size()), m_dxRenderer->getGPUInfo().usedVideoMemory);
#endif
#endif

}

void Game::imguiFunc() {

	// Dont show gui while performance testing
	#ifdef PERFORMANCE_TESTING
	return;
	#endif

	if (ImGui::Begin("Animation")) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
		ImGui::TextWrapped("NOTE: Animations are skinned on the CPU. This is slow. Turn down animation speed to 0 to see pure raytracing performance.");
		ImGui::PopStyleColor();
		ImGui::SliderFloat("Speed", &m_animationSpeed, 0.0f, 1.0f);
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
		if (ImGui::CollapsingHeader("Objects")) {
			for (int i = 0; i < m_gameObjects.size(); i++) {
				if (ImGui::TreeNode(std::string("Animation " + std::to_string(i)).c_str())) {
					ImGui::Checkbox("Updating", &m_gameObjects[i].getAnimationUpdate());

					int index = (int)m_gameObjects[i].getAnimationIndex();
					if (ImGui::SliderInt("Animation", &index, 0, (int)m_gameObjects[i].getModel()->getStackSize() - 1)) {
						m_gameObjects[i].setAnimationIndex(index);
					}

					if (ImGui::SliderFloat("Time", &m_gameObjects[i].getAnimationTime(), 0.0f, m_gameObjects[i].getMaxAnimationTime())) {

					}

					ImGui::TreePop();
				}
			}
		}
	}
	ImGui::End();

	if (ImGui::Begin("Scene")) {
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
		if (ImGui::CollapsingHeader("Models")) {
			unsigned int i = 0;
			for (auto& mesh : m_meshes) {
				if (ImGui::TreeNode(std::string("Mesh " + std::to_string(i) + " - " + mesh->getName()).c_str())) {
					XMVECTOR vec;
					vec = mesh->getTransform().getTranslationVec();
					Transform& transform = mesh->getTransform();
					if (i >= UINT(m_animatedModelsStartIndex)) {
						transform = m_gameObjects[i - m_animatedModelsStartIndex].getTransform();
					}
					if (ImGui::DragFloat3("Position", (float*)&vec, 0.1f)) {
						transform.setTranslation(vec);
						mesh->setTransform(transform); // To update rasterisation CB
					}
					vec = transform.getRotationVec();
					if (ImGui::DragFloat3("Rotation", (float*)&vec, 0.01f)) {
						transform.setRotation(vec);
						mesh->setTransform(transform); // To update rasterisation CB
					}
					vec = transform.getScaleVec();
					if (ImGui::DragFloat3("Scale", (float*)&vec, 0.01f)) {
						transform.setScale(vec);
						mesh->setTransform(transform); // To update rasterisation CB
					}

					ImGui::Text("Material");
					MaterialProperties matProps = mesh->getProperties();
					float fuzziness = matProps.fuzziness;
					if (ImGui::SliderFloat("Fuzziness", &fuzziness, 0.0f, 1.0f)) {
						matProps.fuzziness = fuzziness;
						mesh->setProperties(matProps);
					}
					float reflectionAttenuation = matProps.reflectionAttenuation;
					if (ImGui::SliderFloat("ReflectionAttenuation", &reflectionAttenuation, 0.0f, 1.0f)) {
						matProps.reflectionAttenuation = reflectionAttenuation;
						mesh->setProperties(matProps);
					}
					int recursionDepth = matProps.maxRecursionDepth;
					if (ImGui::SliderInt("Recursion", &recursionDepth, 0, MAX_RAY_RECURSION_DEPTH)) {
						matProps.maxRecursionDepth = recursionDepth;
						mesh->setProperties(matProps);
					}

					ImVec4 color = ImVec4(matProps.albedoColor.x, matProps.albedoColor.y, matProps.albedoColor.z, 1.0f);
					if (ImGui::ColorEdit4("Albedo color##3", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha)) {
						matProps.albedoColor = XMFLOAT3(color.x, color.y, color.z);
						mesh->setProperties(matProps);
					}
					ImGui::SameLine();
					ImGui::Text("Albedo color");

					ImGui::TreePop();
				}
				i++;
			}
		}

	}
	ImGui::End();

	if (m_dxRenderer->isDXRSupported()) {
		if (ImGui::Begin("Raytracer")) {
			auto& dxr = m_dxRenderer->getDXR();

			ImGui::Checkbox("DXR Enabled", &m_dxRenderer->isDXREnabled());

			//ImGui::SameLine();
			//ImGui::PushID(0);
			//ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1 / 7.0f, 0.6f, 0.6f));
			//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1 / 7.0f, 0.7f, 0.7f));
			//ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1 / 7.0f, 0.8f, 0.8f));
			//if (ImGui::Button("Reload DXR shaders")) {
			//	std::cout << "Reloading DXR shaders.." << std::endl;
			//	m_dxRenderer->executeNextOpenPreCommand([&] {
			//		m_dxRenderer->waitForGPU();
			//		m_dxRenderer->getDXR().reloadShaders();
			//	});
			//}
			//ImGui::PopStyleColor(3);
			//ImGui::PopID();
			static bool drawNormals = dxr.getRTFlags() & RT_DRAW_NORMALS;
			if (ImGui::Checkbox("Draw normals only", &drawNormals)) {
				if (drawNormals)
					dxr.getRTFlags() |= RT_DRAW_NORMALS;
				else
					dxr.getRTFlags() &= ~RT_DRAW_NORMALS;
			}
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			static bool enableAO = dxr.getRTFlags() & RT_ENABLE_AO;
			if (ImGui::Checkbox("Enable AO", &enableAO)) {
				if (enableAO)
					dxr.getRTFlags() |= RT_ENABLE_AO;
				else
					dxr.getRTFlags() &= ~RT_ENABLE_AO;
			}
			ImGui::SliderFloat("AO radius", &dxr.getAORadius(), 0.1f, 10.0f);
			ImGui::SliderInt("AO rays", (int*)&dxr.getNumAORays(), 1, 100);
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			static bool enableGI = dxr.getRTFlags() & RT_ENABLE_GI;
			if (ImGui::Checkbox("Enable GI", &enableGI)) {
				if (enableGI)
					dxr.getRTFlags() |= RT_ENABLE_GI;
				else
					dxr.getRTFlags() &= ~RT_ENABLE_GI;
			}
			ImGui::SliderInt("GI samples", (int*)&dxr.getNumGISamples(), 1, 10);
			ImGui::SliderInt("GI bounces", (int*)&dxr.getNumGIBounces(), 1, 5);
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			static bool enableTA = dxr.getRTFlags() & RT_ENABLE_TA;
			static bool enableAA = dxr.getRTFlags() & RT_ENABLE_JITTER_AA;
			if (ImGui::Checkbox("Enable TA", &enableTA)) {
				if (enableTA) {
					dxr.getRTFlags() |= RT_ENABLE_TA;
					dxr.getRTFlags() |= RT_ENABLE_JITTER_AA;
					enableAA = true;
				} else {
					dxr.getRTFlags() &= ~RT_ENABLE_TA;
					dxr.getRTFlags() &= ~RT_ENABLE_JITTER_AA;
					enableAA = false;
				}
			}
			ImGui::Text("Accumulation count: %i", dxr.getTemporalAccumulationCount());
			if (ImGui::Checkbox("Enable camera jitter AA", &enableAA)) {
				if (enableAA)
					dxr.getRTFlags() |= RT_ENABLE_JITTER_AA;
				else
					dxr.getRTFlags() &= ~RT_ENABLE_JITTER_AA;
			}
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Materials override");
			static float fuzziness = 0.0f;
			bool fuzzChanged = ImGui::SliderFloat("Fuzziness", &fuzziness, 0.0f, 1.0f);
			static float reflectionAttenuation = 0.8f;
			bool refAttChanged = ImGui::SliderFloat("ReflectionAttenuation", &reflectionAttenuation, 0.0f, 1.0f);
			static int recursionDepth = 3;
			bool recursionChanged = ImGui::SliderInt("Recursion", &recursionDepth, 0, MAX_RAY_RECURSION_DEPTH);
			static ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			bool colorChanged = ImGui::ColorEdit4("Albedo color##3", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha);
			if (fuzzChanged || refAttChanged || recursionChanged || colorChanged) {
				MaterialProperties props;
				props.fuzziness = fuzziness;
				props.reflectionAttenuation = reflectionAttenuation;
				props.maxRecursionDepth = recursionDepth;
				props.albedoColor = XMFLOAT3(color.x, color.y, color.z);
				for (auto& mesh : m_meshes) {
					mesh->setProperties(props);
				}
			}
			ImGui::SameLine();
			ImGui::Text("Albedo color");

		}
		ImGui::End();
	}

	// Only display options if the window isn't collapsed
	if (ImGui::Begin("Info")) {
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

		static bool vsync = m_dxRenderer->getVsync();
		ImGui::Checkbox("V-sync", &m_dxRenderer->getVsync());
		
		if (ImGui::TreeNode("Camera")) {

			ImGui::InputFloat3("Position", (float*)&m_persCamera->getPositionVec(), 2);
			ImGui::InputFloat3("Direction", (float*)&m_persCamera->getDirectionVec(), 2);
			ImGui::SliderFloat("Speed", &m_persCameraController->getMovementSpeed(), 1.f, 500.f);

			ImGui::TreePop();
			ImGui::Separator();
		}
		
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
		if (ImGui::TreeNode("Statistics")) {

			static bool paused = false;
			static float pausedCopy[FRAME_HISTORY_COUNT];
			static float pausedAvg;
			static float pausedLast;
			static float min = 0.f;
			static float max = 35.f;
			char buffer[100];
			sprintf_s(buffer, 100, "Frame Times\n\nLast:    %.2f ms\nAverage: %.2f ms", (paused) ? pausedLast : frameTimeHistory[FRAME_HISTORY_COUNT-1], (paused) ? pausedAvg : frameTimeAvg);
			ImGui::PlotLines(buffer, (paused) ? pausedCopy : frameTimeHistory, FRAME_HISTORY_COUNT, 0, "", min, max, ImVec2(0, 100));
			ImGui::DragFloatRange2("Range", &min, &max, 0.05f, 0.f, 0.f, "Min: %.1f", "Max: %.1f");
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
		ImGui::Text("Controls\nRight click to lock/unlock mouse\nWASD, Space and C to move camera\nHold F to move mirror 1\nHold G to move mirror 2");

	}

	ImGui::End();
}

#ifdef PERFORMANCE_TESTING
Transform Game::getNextPosition() {
	int n = (int)m_gameObjects.size(); 
	Transform potato;
	potato.setTranslation(XMLoadFloat3(&XMFLOAT3(float((n*5)%100), n*0.00f, -10+n*0.10f))); 
	return potato;
}

bool Game::addObject(PotatoModel * model, int animationIndex, Transform & transform) {
	size_t offset = 0;
	bool resetAnimation = true;
	m_gameObjects.emplace_back(model, animationIndex, transform);

	m_meshes.emplace_back(static_cast<DX12Mesh*>(getRenderer().makeMesh()));
	m_meshes.back()->setName("Animated " + std::to_string(m_meshes.size()));

	// Vertex buffer
	m_vertexBuffers.emplace_back(getRenderer().makeVertexBuffer(sizeof(Vertex) * m_gameObjects.back().getModel()->getModelData().size(), VertexBuffer::STATIC));
	m_vertexBuffers.back()->setData(m_gameObjects.back().getModel()->getModelData().data(), sizeof(Vertex) * m_gameObjects.back().getModel()->getModelVertices().size(), offset);
	// Index buffer
	m_indexBuffers.emplace_back(getRenderer().makeIndexBuffer(sizeof(unsigned int) * m_gameObjects.back().getModel()->getModelIndices().size(), IndexBuffer::STATIC));
	m_indexBuffers.back()->setData(m_gameObjects.back().getModel()->getModelIndices().data(), sizeof(unsigned int) * m_gameObjects.back().getModel()->getModelIndices().size(), offset);
	// Binding
	m_meshes.back()->setIABinding(m_vertexBuffers.back().get(), m_indexBuffers.back().get(), offset, m_gameObjects.back().getModel()->getModelVertices().size(), m_gameObjects.back().getModel()->getModelIndices().size(), sizeof(Vertex));

	// Technique
	m_meshes.back()->technique = m_technique.get();
	// Texture
	//m_meshes.back()->addTexture(m_ballBotTexture.get(), TEX_REG_DIFFUSE_SLOT);
	m_meshes.back()->setTexture2DArray(m_ballbotTexArray.get());
	
	if (m_dxRenderer->isDXRSupported())
		m_dxRenderer->getDXR().setMeshes(m_meshes);

	if(resetAnimation)
		for (GameObject& object : m_gameObjects) {
			object.setAnimationTime(0.0f);
		}

	return true;
}
#endif