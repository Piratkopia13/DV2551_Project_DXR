#pragma once

#include <iostream>
#include <map>
#include <vector>

class Input {
public:
	enum MouseButton {
		LEFT,
		RIGHT,
		NUM_MOUSE_BUTTONS
	};

	// Used by Win32Window to register keystrokes
	static void RegisterKeyDown(const UINT keyCode);
	// Used by Win32Window to register keystrokes
	static void RegisterKeyUp(const UINT keyCode);
	
	// Call at the start of a new frame
	static void NewFrame();
	// Call at the end of frame
	static void EndFrame();

	static void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	static float GetMouseDX();
	static float GetMouseDY();
	static bool IsMouseButtonDown(MouseButton button);
	static bool IsMouseButtonPressed(MouseButton button);
	
	// Check if key is down
	static bool IsKeyDown(const UINT keyCode);
	static bool IsKeyPressed(const UINT keyCode);

	static void showCursor(bool show);
	static bool IsCursorHidden();

	static void setActive(bool active);

private:
	// Disallow instances
	Input() {}
	~Input() {}

private:
	static bool m_isActive;

	static std::map<unsigned int, bool> m_keysDown;
	static std::vector<unsigned int> m_keysPressedPreviousFrame;
	// TODO: Add system keys

	static bool m_mouseButtonsDown[2];
	static bool m_mouseButtonsPressed[2];

	static float m_mouseDeltaX;
	static float m_mouseDeltaY;

	static float m_mouseDeltaXSinceLastFrame;
	static float m_mouseDeltaYSinceLastFrame;

	static bool m_cursorHidden;

};

