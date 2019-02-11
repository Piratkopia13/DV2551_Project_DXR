#include "DX12RenderState.h"

DX12RenderState::DX12RenderState() {
	m_wireframe = false;
}

DX12RenderState::~DX12RenderState() {
}

void DX12RenderState::set() {
}

/*
	Keep a pointer to the global wireframe state
*/
void DX12RenderState::setGlobalWireFrame(bool* global) {
	this->m_globalWireFrame = global;
}

bool DX12RenderState::wireframeEnabled() {
	return m_wireframe || *m_globalWireFrame;
}

/*
 set wireframe mode for this Render state
*/
void DX12RenderState::setWireFrame(bool enabled) {
	m_wireframe = enabled;

}
