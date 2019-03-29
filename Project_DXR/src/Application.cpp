#include "pch.h"
#include "Application.h"
#include "Utils/Input.h"

Application::Application(int windowWidth, int windowHeight, const char* windowTitle, Renderer::BACKEND api) {
	m_renderer = std::unique_ptr<Renderer>(Renderer::makeRenderer(api));
	m_renderer->initialize(windowWidth, windowHeight);
	m_renderer->setWinTitle(windowTitle);

	ZeroMemory(frameTimeHistory, FRAME_HISTORY_COUNT * sizeof(float));

	m_timeSinceLastUpdate = 0;
}

Application::~Application() {
}

int Application::startGameLoop() {
	std::cout << "Initializing.." << std::endl;
	init();
	std::cout << "Initialization done!" << std::endl;

	MSG msg = { 0 };
	// Main message loop
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			// do the stuff
			Input::NewFrame();
			double dtSeconds = m_lastDelta / 1000.0;
			if (dtSeconds > 1.0) // If frame-time is greater than 1 second, cap it to 1 second.
				dtSeconds = 1.0;
			m_timeSinceLastUpdate += dtSeconds;
			while (m_timeSinceLastUpdate > 0.01667) { // Update 60 times per second
				fixedUpdate(0.01667);
				m_timeSinceLastUpdate -= 0.01667;
			}
			update(dtSeconds);
			render(dtSeconds);
			Input::EndFrame();
			updateDelta();

		}
	}

	return 0;
}

Renderer& Application::getRenderer() {
	return *m_renderer;
}

const UINT Application::getFPS() const {
	return static_cast<UINT>(1000.0 / m_lastDelta);
}

void Application::updateDelta() {
	static const int WINDOW_SIZE = 10;
	static LARGE_INTEGER lStart;
	static LARGE_INTEGER lFreq;
	static UINT64 start = 0;
	static UINT64 last = 0;
	static double avg[WINDOW_SIZE] = { 0.0 };
	static double lastSum = 10.0;
	static int loop = 0;
	static double timeAccumulator = 0.f;

	last = start;
	QueryPerformanceCounter(&lStart);
	QueryPerformanceFrequency(&lFreq);
	start = lStart.QuadPart;
	double deltaTime = (double)((start - last) * 1000.0 / lFreq.QuadPart);
	// moving average window of WINDOWS_SIZE
	lastSum -= avg[loop];
	lastSum += deltaTime;
	avg[loop] = deltaTime;
	loop = (loop + 1) % WINDOW_SIZE;
	m_lastDelta = (lastSum / WINDOW_SIZE);

	if (timeAccumulator >= 500.0) {
		timeAccumulator = 0.0;
		sprintf_s(m_titleBuff, "DX12 DXR - %3.0lfms, %3.0u fps", m_lastDelta, getFPS());
		m_renderer->setWinTitle(m_titleBuff);
	}
	timeAccumulator += deltaTime;

	if (loop == 0) {
		// Shift elements to the left
		float sum = 0;
		for (int i = 0; i < FRAME_HISTORY_COUNT - 1; i++) {
			frameTimeHistory[i] = frameTimeHistory[i + 1];
			sum += frameTimeHistory[i];
		}
		// Insert the new delta
		frameTimeHistory[FRAME_HISTORY_COUNT - 1] = static_cast<float>(m_lastDelta);
		frameTimeAvg = (sum + frameTimeHistory[FRAME_HISTORY_COUNT - 1]) / FRAME_HISTORY_COUNT;
	}
}
