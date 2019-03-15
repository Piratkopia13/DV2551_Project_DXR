#pragma once

#include <DirectXMath.h>

#include "Camera.h"

class CameraController {
public:

	CameraController(Camera* cam, const DirectX::XMVECTOR& startDirection);

	void update(float dt);
	float& getMovementSpeed();

protected:
	void setCameraLookAt(const DirectX::XMVECTOR& pos) {
		m_cam->lookAt(pos);
	}
	void setCameraDirection(const DirectX::XMVECTOR& direction) {
		m_cam->setDirection(direction);
	}
	void setCameraPosition(const DirectX::XMVECTOR& pos) {
		m_cam->setPosition(pos);
	}
	const DirectX::XMVECTOR& getCameraDirection() {
		return m_cam->getDirectionVec();
	}
	const DirectX::XMVECTOR& getCameraPosition() {
		return m_cam->getPositionVec();
	}
	const DirectX::XMVECTOR& getCameraUp() {
		return m_cam->getUpVec();
	}
	const Camera* getCamera() {
		return m_cam;
	}

private:
	Camera* m_cam;
	float m_pitch, m_yaw, m_roll;
	float m_speed;

};