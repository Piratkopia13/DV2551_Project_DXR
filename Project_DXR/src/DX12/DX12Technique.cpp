#include "pch.h"
#include "DX12Technique.h"
#include "DX12Renderer.h"

DX12Technique::DX12Technique(DX12Material* m, DX12RenderState* r, DX12Renderer* renderer, bool hasDSV)
	: Technique(m, r) {

	ID3DBlob* vertexBlob = m->getShaderBlob(Material::ShaderType::VS);
	ID3DBlob* pixelBlob = m->getShaderBlob(Material::ShaderType::PS);

	////// Pipline State //////
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	//Specify pipeline stages:
	gpsd.pRootSignature = renderer->getRootSignature();
	gpsd.InputLayout = m->getInputLayoutDesc();
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
	gpsd.RasterizerState.FillMode = (r->wireframeEnabled()) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
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

	if (hasDSV) {
		// Specify depth stencil state descriptor.
		gpsd.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	}

	ThrowIfFailed(renderer->getDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pipelineState)));

}

DX12Technique::~DX12Technique() {

}

void DX12Technique::enable(Renderer* renderer) {
	throw std::exception("The technique must be enabled using the other enable method taking two parameters");
}

void DX12Technique::enable(Renderer* renderer, ID3D12GraphicsCommandList3* cmdList) {
	static_cast<DX12Material*>(material)->enable(cmdList);
}

ID3D12PipelineState* DX12Technique::getPipelineState() const {
	return m_pipelineState.Get();
}

