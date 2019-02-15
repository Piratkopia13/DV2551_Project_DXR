#pragma once

#include <iostream>
#include <map>
#include <vector>

class Input {
public:
	
	static void Update();

	static void RegisterKeyDown(const unsigned char keyCode);
	static void RegisterKeyUp(const unsigned char keyCode);

	static bool IsKeyDown(const unsigned char keyCode);
	//void MouseDown();
	//void MouseUp();

	//bool IsKeyDown();

private:
	static std::map<unsigned char, bool> m_keys;

private:
	// Disallow instances
	Input() {}
	~Input() {}

};

