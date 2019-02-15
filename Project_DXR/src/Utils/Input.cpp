#include "pch.h"
#include "Input.h"


Input::Input() {

}


Input::~Input() {
}

void Input::Update() {
}

void Input::RegisterKeyDown(const unsigned char keyCode) {
	auto iter = m_keys.find(keyCode);
	if(iter != m_keys.end())
		iter->second = true;
	else {
		m_keys.emplace(keyCode, true);
	}
}

void Input::RegisterKeyUp(const unsigned char keyCode) {
	auto iter = m_keys.find(keyCode);
	if (iter != m_keys.end())
		iter->second = false;
	else {
		m_keys.emplace(keyCode, false);
	}
}

bool Input::IsKeyDown(const unsigned char keyCode) {
	auto iter = m_keys.find(keyCode);
	if (iter != m_keys.end()) {
		return iter->second;
	}
	return false;
}
