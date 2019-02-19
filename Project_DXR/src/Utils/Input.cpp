#include "pch.h"
#include "Input.h"


float Input::m_mouseDeltaX = 0.f;
float Input::m_mouseDeltaY = 0.f;
float Input::m_mouseDeltaXSinceLastFrame = 0.f;
float Input::m_mouseDeltaYSinceLastFrame = 0.f;
bool Input::m_mouseButtonsDown[2] = { false, false };
bool Input::m_mouseButtonsPressed[2] = { false, false };
bool Input::m_cursorHidden = false;
std::map<unsigned int, bool> Input::m_keysDown = {};
std::vector<unsigned int> Input::m_keysPressed = {};

void Input::RegisterKeyDown(const UINT keyCode) {
	auto iter = m_keysDown.find(keyCode);
	if (iter != m_keysDown.end()) {
		iter->second = true;
	}
	else {
		m_keysDown.emplace(keyCode, true);
	}
	m_keysPressed.emplace_back(keyCode);
}

void Input::RegisterKeyUp(const UINT keyCode) {
	auto iter = m_keysDown.find(keyCode);
	if (iter != m_keysDown.end())
		iter->second = false;
	else {
		m_keysDown.emplace(keyCode, false);
	}
}

void Input::NewFrame() {
	m_mouseDeltaXSinceLastFrame = m_mouseDeltaX;
	m_mouseDeltaYSinceLastFrame = m_mouseDeltaY;

	m_mouseDeltaX = 0.f;
	m_mouseDeltaY = 0.f;
}

void Input::EndFrame() {
	m_keysPressed.clear();

	for (unsigned int i = 0; i < MouseButton::NUM_MOUSE_BUTTONS; i++)
		m_mouseButtonsPressed[i] = false;
}

bool Input::IsKeyDown(const UINT keyCode) {
	auto iter = m_keysDown.find(keyCode);
	if (iter != m_keysDown.end()) {
		return iter->second;
	}
	return false;
}

bool Input::IsKeyPressed(const UINT keyCode)
{
	return std::find(m_keysPressed.begin(), m_keysPressed.end(), keyCode) != m_keysPressed.end();
}

void Input::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
	//std::cout << "Processing messages..." << std::endl;

	UINT dwSize;

	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	LPBYTE lpb = new BYTE[dwSize];
	if (lpb == NULL) {
		return;
	}

	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
		OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

	RAWINPUT* raw = (RAWINPUT*)lpb;
	if (raw->header.dwType == RIM_TYPEMOUSE) {

		m_mouseDeltaX += raw->data.mouse.lLastX;
		m_mouseDeltaY += raw->data.mouse.lLastY;

		if ((RI_MOUSE_LEFT_BUTTON_DOWN & raw->data.mouse.usButtonFlags) == RI_MOUSE_LEFT_BUTTON_DOWN) {
			m_mouseButtonsDown[MouseButton::LEFT] = true;
			m_mouseButtonsPressed[MouseButton::LEFT] = true;
		}
		else if ((RI_MOUSE_LEFT_BUTTON_UP & raw->data.mouse.usButtonFlags) == RI_MOUSE_LEFT_BUTTON_UP) {
			m_mouseButtonsDown[MouseButton::LEFT] = false;
		}

		if ((RI_MOUSE_RIGHT_BUTTON_DOWN & raw->data.mouse.usButtonFlags) == RI_MOUSE_RIGHT_BUTTON_DOWN) {
			m_mouseButtonsDown[MouseButton::RIGHT] = true;
			m_mouseButtonsPressed[MouseButton::RIGHT] = true;
		}
		else if ((RI_MOUSE_RIGHT_BUTTON_UP & raw->data.mouse.usButtonFlags) == RI_MOUSE_RIGHT_BUTTON_UP) {
			m_mouseButtonsDown[MouseButton::RIGHT] = false;
		}
	}

	//std::cout << "---------------------" << std::endl;

	delete[] lpb;
}

float Input::GetMouseDX() {
	return m_mouseDeltaXSinceLastFrame;
}

float Input::GetMouseDY() {
	return m_mouseDeltaYSinceLastFrame;
}

bool Input::IsMouseButtonDown(MouseButton button) {
	assert(button >= 0 && button < MouseButton::NUM_MOUSE_BUTTONS);
	return m_mouseButtonsDown[button];
}

bool Input::IsMouseButtonPressed(MouseButton button) {
	assert(button >= 0 && button < MouseButton::NUM_MOUSE_BUTTONS);
	return m_mouseButtonsPressed[button];
}

void Input::showCursor(bool show) {
	m_cursorHidden = !show;
	ShowCursor(show);
}

bool Input::IsCursorHidden()
{
	return m_cursorHidden;
}
