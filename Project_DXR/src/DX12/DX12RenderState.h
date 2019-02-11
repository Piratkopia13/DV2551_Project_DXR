#pragma once
#include <vector>
#include "../Core/RenderState.h"

class DX12RenderState : public RenderState {
public:
	DX12RenderState();
	~DX12RenderState();
	void setWireFrame(bool enabled);
	void set();
	void setGlobalWireFrame(bool* global);
	bool wireframeEnabled();

private:
	bool m_wireframe;
	bool* m_globalWireFrame;
};

