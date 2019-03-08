#include "pch.h"
#include "Transform.h"

using namespace DirectX;
Transform::Transform() {
	m_needsUpdate = false;
	m_transformMatrix = XMMatrixIdentity();
	m_translation = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	m_rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	m_scale = XMVectorSet(1.f, 1.f, 1.f, 0.f);
}
Transform::~Transform() {}

void Transform::translate(const DirectX::XMVECTOR translation) {
	m_translation += translation;
	m_needsUpdate = true;
}

void Transform::setTranslation(const DirectX::XMVECTOR translation) {
	m_translation = translation;
	m_needsUpdate = true;
}

void Transform::scale(const DirectX::XMVECTOR scale) {
	m_scale *= scale;
	m_needsUpdate = true;
}

void Transform::scaleUniformly(const float scale) {
	m_scale *= scale;
	m_needsUpdate = true;
}

void Transform::setScale(const DirectX::XMVECTOR scale) {
	m_scale = scale;
	m_needsUpdate = true;
}

void Transform::setRotation(float x, float y, float z) {
	m_rotation = XMVectorSet(x, y, z, 1.0f);
	m_needsUpdate = true;
}

void Transform::setRotation(const DirectX::XMVECTOR& rotVec) {
	m_rotation = rotVec;
	m_needsUpdate = true;
}

void Transform::rotateAroundX(const float radians) {
	XMVectorSetX(m_rotation, XMVectorGetX(m_rotation) + radians);
	m_needsUpdate = true;
}

void Transform::rotateAroundY(const float radians) {
	XMVectorSetY(m_rotation, XMVectorGetY(m_rotation) + radians);
	m_needsUpdate = true;
}

void Transform::rotateAroundZ(const float radians) {
	XMVectorSetZ(m_rotation, XMVectorGetZ(m_rotation) + radians);
	m_needsUpdate = true;
}

void Transform::setTransformMatrix(DirectX::XMMATRIX matrix) {
	XMVECTOR quat;
	XMFLOAT4 q;
	XMMatrixDecompose(&m_scale, &quat, &m_translation, matrix);
	XMStoreFloat4(&q, quat);
	XMVectorSetX(m_rotation, atan2f(2.0f * q.y*q.w - 2.0f * q.x*q.z, 1.0f - 2.0f * q.y * q.y - 2.0 * q.z * q.z));
	XMVectorSetY(m_rotation, asinf(2.0f * q.x*q.y + 2.0f * q.z*q.w));
	XMVectorSetZ(m_rotation, atan2f(2.0f * q.x*q.w - 2.0f * q.y*q.z, 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z));
	m_transformMatrix = matrix;
}

const DirectX::XMFLOAT3 Transform::getTranslation() const {
	XMFLOAT3 toReturn;
	XMStoreFloat3(&toReturn, m_translation);
	return toReturn;
}

const DirectX::XMVECTOR& Transform::getTranslationVec() const {
	return m_translation;
}

const DirectX::XMFLOAT3 Transform::getRotation() const {
	XMFLOAT3 toReturn;
	XMStoreFloat3(&toReturn, m_rotation);
	return toReturn;
}

const DirectX::XMVECTOR& Transform::getRotationVec() const {
	return m_rotation;
}

const DirectX::XMFLOAT3 Transform::getScale() const {
	XMFLOAT3 toReturn;
	XMStoreFloat3(&toReturn, m_scale);
	return toReturn;
}

const DirectX::XMVECTOR& Transform::getScaleVec() const {
	return m_scale;
}

const DirectX::XMMATRIX & Transform::getTransformMatrix() {
	// The matrix needs to be updated, hence recalculate it
	if (m_needsUpdate) {
		m_transformMatrix =
			// Scale
			XMMatrixScalingFromVector(m_scale) *
			// Rotation
			XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYawFromVector(m_rotation)) *
			// Translation
			XMMatrixTranslationFromVector(m_translation);

		m_needsUpdate = false;
	}

	return m_transformMatrix;
}
