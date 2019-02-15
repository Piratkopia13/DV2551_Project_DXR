#include "pch.h"
#include "Input.h"


float Input::m_mouseDeltaX = 0.f;
float Input::m_mouseDeltaY = 0.f;
float Input::m_mouseDeltaXSinceLastFrame = 0.f;
float Input::m_mouseDeltaYSinceLastFrame = 0.f;
bool Input::m_mouseButtons[2] = { false, false };
std::map<unsigned char, bool> Input::m_keys = {};

void Input::RegisterKeyDown(const UINT keyCode) {
	auto iter = m_keys.find(keyCode);
	if(iter != m_keys.end())
		iter->second = true;
	else {
		m_keys.emplace(keyCode, true);
	}
}

void Input::RegisterKeyUp(const UINT keyCode) {
	auto iter = m_keys.find(keyCode);
	if (iter != m_keys.end())
		iter->second = false;
	else {
		m_keys.emplace(keyCode, false);
	}
}

void Input::NewFrame() {
	m_mouseDeltaXSinceLastFrame = m_mouseDeltaX;
	m_mouseDeltaYSinceLastFrame = m_mouseDeltaY;

	m_mouseDeltaX = 0.f;
	m_mouseDeltaY = 0.f;
}

void Input::EndFrame() {
}

bool Input::IsKeyDown(const UINT keyCode) {
	auto iter = m_keys.find(keyCode);
	if (iter != m_keys.end()) {
		return iter->second;
	}
	return false;
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
			m_mouseButtons[MouseButton::LEFT] = true;
		}
		else if ((RI_MOUSE_LEFT_BUTTON_UP & raw->data.mouse.usButtonFlags) == RI_MOUSE_LEFT_BUTTON_UP) {
			m_mouseButtons[MouseButton::LEFT] = false;
		}

		if ((RI_MOUSE_RIGHT_BUTTON_DOWN & raw->data.mouse.usButtonFlags) == RI_MOUSE_RIGHT_BUTTON_DOWN) {
			m_mouseButtons[MouseButton::RIGHT] = true;
		}
		else if ((RI_MOUSE_RIGHT_BUTTON_UP & raw->data.mouse.usButtonFlags) == RI_MOUSE_RIGHT_BUTTON_UP) {
			m_mouseButtons[MouseButton::RIGHT] = false;
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

bool Input::GetMouseButtonDown(MouseButton button) {
	return m_mouseButtons[button];
}

void Input::showCursor(bool show) {
	ShowCursor(show);
}
