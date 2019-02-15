#pragma once

#include <iostream>
#include <map>
#include <vector>

class Input {
public:
	enum MouseButton {
		LEFT,
		RIGHT
	};

	// Used by Win32Window to register keystrokes
	static void RegisterKeyDown(const UINT keyCode);
	// Used by Win32Window to register keystrokes
	static void RegisterKeyUp(const UINT keyCode);
	
	// Call at the start of a new frame
	static void NewFrame();
	// Call at the end of frame
	static void EndFrame();

	// Check if key is down
	static bool IsKeyDown(const UINT keyCode);

	static void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	static float GetMouseDX();
	static float GetMouseDY();
	static bool GetMouseButtonDown(MouseButton button);

	static void showCursor(bool show);

private:
	static std::map<unsigned char, bool> m_keys;

	static bool m_mouseButtons[2];

	static float m_mouseDeltaX;
	static float m_mouseDeltaY;

	static float m_mouseDeltaXSinceLastFrame;
	static float m_mouseDeltaYSinceLastFrame;

private:
	// Disallow instances
	Input() {}
	~Input() {}

};

