#pragma once

#include "Core/Renderer.h"

class Application {

public:
	Application(int windowWidth, int windowHeight, const char* windowTitle, Renderer::BACKEND api = Renderer::BACKEND::DX12);
	virtual ~Application();

	int startGameLoop();

	// Required methods
	virtual void init() = 0;
	virtual void update(double dt) = 0;
	virtual void render(double dt) = 0;

	Renderer& getRenderer();
	const UINT getFPS() const;

private:
	void updateDelta();
private:
	std::unique_ptr<Renderer> m_renderer;
	double m_lastDelta;
	char m_titleBuff[256];

};