#include "pch.h"
#include "Game.h"
#include "DX12/DX12Renderer.h"
#include "Utils/Input.h"

Game::Game() 
	: Application(1280, 720, "DX12 DXR Raytracer thing with soon to come skinned animated models")
{
	//static_cast<DX12Renderer&>(getRenderer()).enableDXR(false);
}

Game::~Game() {
}

void Game::init() {
	// triangle geometry
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

	// set material name from text file?
	m_material = std::unique_ptr<Material>(getRenderer().makeMaterial("material_0"));
	m_material->setShader(shaderPath + "VertexShader" + shaderExtension, Material::ShaderType::VS);
	m_material->setShader(shaderPath + "FragmentShader" + shaderExtension, Material::ShaderType::PS);
	std::string err;
	m_material->compileMaterial(err);

	// add a constant buffer to the material, to tint every triangle using this material
	m_material->addConstantBuffer(DIFFUSE_TINT_NAME, DIFFUSE_TINT);
	// no need to update anymore
	// when material is bound, this buffer should be also bound for access.

	m_material->updateConstantBuffer(diffuse, 4 * sizeof(float), DIFFUSE_TINT);

	// basic technique
	m_technique = std::unique_ptr<Technique>(getRenderer().makeTechnique(m_material.get(), getRenderer().makeRenderState()));

	// create texture
	m_texture = std::unique_ptr<Texture2D>(getRenderer().makeTexture2D());
	m_texture->loadFromFile("../assets/textures/fatboy.png");
	m_sampler = std::unique_ptr<Sampler2D>(getRenderer().makeSampler2D());
	m_sampler->setWrap(WRAPPING::REPEAT, WRAPPING::REPEAT);
	m_texture->sampler = m_sampler.get();

	// pre-allocate one single vertex buffer for ALL triangles
	m_vertexBuffer = std::unique_ptr<VertexBuffer>(getRenderer().makeVertexBuffer(sizeof(vertices), VertexBuffer::DATA_USAGE::STATIC));

	// Create a mesh array with 3 basic vertex buffers.
	m_mesh = std::unique_ptr<Mesh>(getRenderer().makeMesh());

	constexpr auto numberOfPosElements = std::extent<decltype(vertices)>::value;
	size_t offset = 0;
	m_vertexBuffer->setData(vertices, sizeof(vertices), offset);
	m_mesh->setIAVertexBufferBinding(m_vertexBuffer.get(), offset, numberOfPosElements, sizeof(float) * 8); // 3 positions, 3 normals and 2 UVs
	
	// we can create a constant buffer outside the material, for example as part of the Mesh.
	m_mesh->txBuffer = getRenderer().makeConstantBuffer(std::string(TRANSLATION_NAME), TRANSLATION);

	m_mesh->technique = m_technique.get();
	m_mesh->addTexture(m_texture.get(), DIFFUSE_SLOT);

}

void Game::update(double dt) {

	static bool pressed = false;
	if (Input::IsKeyDown(' ')) {
		if (!pressed) {
			pressed = true;
			auto& r = static_cast<DX12Renderer&>(getRenderer());
			r.enableDXR(!r.isDXREnabled());
		}
	} else {
		pressed = false;
	}

	/*
//	    For each mesh in scene list, update their position
//	*/
//	{
//		static long long shift = 0;
//		size_t size = scene.size();
//		for (size_t i = 0; i < size; i++)
//		{
//			const float4 trans { 
//				xt[(int)(float)(i + shift) % (TOTAL_PLACES)], 
//				yt[(int)(float)(i + shift) % (TOTAL_PLACES)], 
//				i * (-1.0f / TOTAL_PLACES),
//				0.0f
//			};
//			scene[i]->txBuffer->setData(&trans, sizeof(trans), scene[i]->technique->getMaterial(), TRANSLATION);
//		}
//		// just to make them move...
//		shift += static_cast<long long>(max(TOTAL_TRIS / 1000.0, TOTAL_TRIS / 100.0));
//	}
}

void Game::render(double dt) {
	//getRenderer().clearBuffer(CLEAR_BUFFER_FLAGS::COLOR | CLEAR_BUFFER_FLAGS::DEPTH); // Doesnt do anything

	getRenderer().submit(m_mesh.get());

	getRenderer().frame();
	getRenderer().present();
}
