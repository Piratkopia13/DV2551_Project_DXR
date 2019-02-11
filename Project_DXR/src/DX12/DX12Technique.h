#pragma once
#include <vector>
#include "DX12.h"
#include "../Core/Technique.h"
#include "DX12Material.h"
#include "DX12RenderState.h"

class DX12Renderer;

class DX12Technique : public Technique {
public:
	DX12Technique(DX12Material* m, DX12RenderState* r, DX12Renderer* renderer);
	~DX12Technique();
	virtual void enable(Renderer* renderer) override;
	void enable(Renderer* renderer, ID3D12GraphicsCommandList3* cmdList);

	ID3D12PipelineState* getPipelineState() const;

private:
	wComPtr<ID3D12PipelineState> m_pipelineState;

};

