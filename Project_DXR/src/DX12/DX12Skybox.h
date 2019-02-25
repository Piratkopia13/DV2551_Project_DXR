#pragma once

#include "DX12.h"
class DX12Renderer;
class DX12Material;
class DX12VertexBuffer;
class DX12ConstantBuffer;
class DX12Texture2D;

class DX12Skybox {
public:
	DX12Skybox(DX12Renderer* renderer);
	~DX12Skybox();

	void render(ID3D12GraphicsCommandList4 * cmdList);

	void setCamCB(DX12ConstantBuffer* cb);

private:
	wComPtr<ID3D12PipelineState> m_pipelineState;
	DX12Renderer* m_renderer;
	DX12Material* m_mat;
	DX12Texture2D* m_texture;
	DX12VertexBuffer* m_vb;
	DX12ConstantBuffer* m_camCB;
	DX12ConstantBuffer* m_transCB;
};

