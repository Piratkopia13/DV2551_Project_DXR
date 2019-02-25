#include "pch.h"
#include "DX12Skybox.h"
#include "DX12VertexBuffer.h"
#include "DX12Renderer.h"
#include "DX12Technique.h"
#include "../Core/Camera.h"
#include "DX12Texture2D.h"


DX12Skybox::DX12Skybox(DX12Renderer* renderer)
: m_renderer(renderer) {

	m_mat = new DX12Material("Skybox Material", renderer);
	std::string shaderPath = m_renderer->getShaderPath();
	std::string shaderExtension = m_renderer->getShaderExtension();
	m_mat->setShader(shaderPath + "SkyShader" + shaderExtension, Material::ShaderType::VS);
	m_mat->setShader(shaderPath + "SkyShader" + shaderExtension, Material::ShaderType::PS);
	std::string err;
	m_mat->compileMaterial(err);

	ID3DBlob* vertexBlob = m_mat->getShaderBlob(Material::ShaderType::VS);
	ID3DBlob* pixelBlob = m_mat->getShaderBlob(Material::ShaderType::PS);

	////// Pipline State //////
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	//Specify pipeline stages:
	gpsd.pRootSignature = renderer->getRootSignature();
	gpsd.InputLayout = m_mat->getInputLayoutDesc();
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsd.VS.pShaderBytecode = reinterpret_cast<void*>(vertexBlob->GetBufferPointer());
	gpsd.VS.BytecodeLength = vertexBlob->GetBufferSize();
	gpsd.PS.pShaderBytecode = reinterpret_cast<void*>(pixelBlob->GetBufferPointer());
	gpsd.PS.BytecodeLength = pixelBlob->GetBufferSize();

	//Specify render target and depthstencil usage.
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.NumRenderTargets = 1;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleDesc.Quality = 0;
	gpsd.SampleMask = UINT_MAX;

	//Specify rasterizer behaviour.
	gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//Specify blend descriptions.
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	gpsd.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	ThrowIfFailed(renderer->getDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pipelineState)));
	
	const Vertex vertices[] = { // butiful cube
		{XMFLOAT3(-1.0f,-1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f,-1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f,-1.0f,-1.0), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f,-1.0f,-1.0), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f,-1.0f,-1.0), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f,-1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f,-1.0f, 1.0), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f,-1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f,-1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f,-1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f,-1.0), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f,-1.0), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 1.0), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3(1.0f,-1.0f, 1.0), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT2(0.0f, 0.0f)}
	};

	m_vb = new DX12VertexBuffer(sizeof(vertices), VertexBuffer::DATA_USAGE::STATIC, m_renderer);
	m_vb->setData(&vertices, sizeof(vertices), 0U);
	m_vb->setVertexStride(sizeof(Vertex));

	m_texture = new DX12Texture2D(renderer);
	m_texture->loadFromFile("../assets/textures/GCanyon_C_YumaPoint_4k.png");

	DirectX::XMMATRIX identityMat = DirectX::XMMatrixIdentity();
	m_transCB = new DX12ConstantBuffer("Skybox Transform CB", sizeof(DirectX::XMMATRIX), renderer);
	m_transCB->setData(&identityMat, CB_REG_TRANSFORM);

	m_camCB = nullptr;
}

DX12Skybox::~DX12Skybox() {
	delete m_mat;
}

void DX12Skybox::render(ID3D12GraphicsCommandList4 * cmdList) {
	if (!m_renderer->isDXREnabled()) {
		cmdList->SetPipelineState(m_pipelineState.Get());

		// Enable the material
		// This binds constant buffers
		m_mat->enable(cmdList);

		//static_cast<DX12Texture2D*>(t.second)->bind(t.first, list.Get());
		m_texture->bind(0, cmdList);

		// Bind vertices, normals and UVs
		m_vb->bind(0, m_vb->getSize(), 0, cmdList);

		// Bind translation constant buffer
		m_transCB->bind(m_mat, cmdList);

		// Bind camera data constant buffer
		m_camCB->bind(m_mat, cmdList);

		// Draw
		cmdList->DrawInstanced(36, 1, 0, 0);
	}
}

void DX12Skybox::setCamCB(DX12ConstantBuffer * cb) {
	m_camCB = cb;
}

DX12Texture2D * DX12Skybox::getTexture() {
	return m_texture;
}
