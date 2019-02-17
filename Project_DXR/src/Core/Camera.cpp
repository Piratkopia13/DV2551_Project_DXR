#include "pch.h"
#include "Camera.h"


using namespace DirectX;

Camera::Camera(const float aspectRatio, const float fov, const float nearZ, const float farZ)
	: m_aspectRatio(aspectRatio)
	, m_fov(XMConvertToRadians(fov))
	, m_nearZ(nearZ)
	, m_farZ(farZ) 
{
	m_pos = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	m_dir = XMVectorSet(0.f, 0.f, -1.f, 0.f);
	m_up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	m_VPMatrix = XMMatrixIdentity();
	m_projMatrix = m_VPMatrix;
	m_viewMatrix = m_VPMatrix;

	m_viewMatNeedsUpdate = true;
	m_projMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}


Camera::~Camera() {
}

void Camera::lookAt(const DirectX::XMVECTOR& position) {
	m_dir = position - m_pos;
	m_dir = XMVector3Normalize(m_dir);
	m_viewMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}

void Camera::setPosition(const DirectX::XMVECTOR& position) {
	m_pos = position;
	m_viewMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}

void Camera::move(const DirectX::XMVECTOR& toMove) {
	m_pos += toMove;
	m_viewMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}

void Camera::setDirection(const DirectX::XMVECTOR& direction) {
	m_dir = direction;
	m_viewMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}

void Camera::setFOV(const float fov) {
	m_fov = XMConvertToRadians(fov);
	m_projMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}

void Camera::setAspectRatio(const float aspectRatio) {
	m_aspectRatio = aspectRatio;
	m_projMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}

void Camera::setNearZ(const float nearZ) {
	m_nearZ = nearZ;
	m_projMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}

void Camera::setFarZ(const float farZ) {
	m_farZ = farZ;
	m_projMatNeedsUpdate = true;
	m_VPMatNeedsUpdate = true;
}


const float Camera::getAspectRatio() const {
	return m_aspectRatio;
}

const float Camera::getFOV() const {
	return m_fov;
}

const float Camera::getNearZ() const {
	return m_nearZ;
}

const float Camera::getFarZ() const {
	return m_farZ;
}

const DirectX::XMFLOAT3 Camera::getPosition() {
	XMFLOAT3 toReturn;
	XMStoreFloat3(&toReturn, m_pos);
	return toReturn;
}

const DirectX::XMFLOAT3 Camera::getDirection() {
	XMFLOAT3 toReturn;
	XMStoreFloat3(&toReturn, m_dir);
	return toReturn;
}

const DirectX::XMMATRIX Camera::getViewMatrix()
{
	if (m_viewMatNeedsUpdate) {
		m_viewMatrix = XMMatrixLookToLH(m_pos, m_dir, m_up);
		m_invViewMatrix = XMMatrixInverse(nullptr, m_viewMatrix);
		m_viewMatNeedsUpdate = false;
	}

	return m_viewMatrix;
}

const DirectX::XMMATRIX Camera::getInvViewMatrix()
{
	// Update inverse view matrix if required
	getViewMatrix();
	return m_invViewMatrix;
}

const DirectX::XMMATRIX Camera::getProjMatrix()
{
	if (m_projMatNeedsUpdate) {
		m_projMatrix = XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_nearZ, m_farZ);
		m_invProjMatrix = XMMatrixInverse(nullptr, m_projMatrix);
		m_projMatNeedsUpdate = false;
	}

	return m_projMatrix;
}

const DirectX::XMMATRIX Camera::getInvProjMatrix()
{
	// Update inverse projection matrix if required
	getProjMatrix();
	return m_invProjMatrix;
}

const DirectX::XMMATRIX Camera::getVPMatrix() {

	if (m_VPMatNeedsUpdate) {
		m_VPMatrix = getViewMatrix() * getProjMatrix();
		m_VPMatNeedsUpdate = false;
	}

	return m_VPMatrix;
}
