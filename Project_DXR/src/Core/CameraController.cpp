#include "pch.h"
#include "CameraController.h"
#include "../Utils/Input.h"

using namespace DirectX;

CameraController::CameraController(Camera * cam, const DirectX::XMVECTOR& startDirection)
	: m_cam(cam) 
{

	XMFLOAT3 d;
	XMStoreFloat3(&d, XMVector3Normalize(startDirection));
	m_yaw = XMConvertToDegrees(atan2f(d.x, d.z)) + 90.0f;
	m_pitch = -XMConvertToDegrees(asinf(-d.y));
	m_roll = 0.f;

	m_speed = 10.0f;
};

void CameraController::update(float dt) {

	float lookSensitivity = 0.1f;
	float movementSpeed = m_speed * dt;

	XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMVECTOR right = XMVector3Cross(getCameraDirection(), worldUp);
	right = XMVector3Normalize(right);
	

	// Camera movement
	if (Input::IsKeyDown('W'))
		setCameraPosition(getCameraPosition() + getCameraDirection() * movementSpeed);
	if (Input::IsKeyDown('S'))
		setCameraPosition(getCameraPosition() - getCameraDirection() * movementSpeed);

	if (Input::IsKeyDown('A'))
		setCameraPosition(getCameraPosition() + right * movementSpeed);
	if (Input::IsKeyDown('D'))
		setCameraPosition(getCameraPosition() - right * movementSpeed);

	if (Input::IsKeyDown(' '))
		setCameraPosition(getCameraPosition() + worldUp * movementSpeed);
	if (Input::IsKeyDown('C'))
		setCameraPosition(getCameraPosition() - worldUp * movementSpeed);


	// Camera rotation
	if (Input::IsCursorHidden()) {
		m_pitch -= (float)(Input::GetMouseDY()) * lookSensitivity;
		m_yaw -= (float)(Input::GetMouseDX()) * lookSensitivity;
	}

	// Lock pitch to the range -89 - 89
	if (m_pitch >= 89.f)
		m_pitch = 89.f;
	else if (m_pitch <= -89.f)
		m_pitch = -89.f;

	// Lock yaw to the range 0 - 360
	if (m_yaw >= 360.f)
		m_yaw -= 360.f;
	else if (m_yaw <= 0.f)
		m_yaw += 360.f;

	XMVECTOR forwards = XMVectorSet(
		std::cos(XMConvertToRadians(m_pitch)) * std::cos(XMConvertToRadians(m_yaw)),
		std::sin(XMConvertToRadians(m_pitch)),
		std::cos(XMConvertToRadians(m_pitch)) * std::sin(XMConvertToRadians(m_yaw)),
		0.f
	);
	forwards = XMVector3Normalize(forwards);

	setCameraDirection(forwards);
}

float& CameraController::getMovementSpeed() {
	return m_speed;
}
