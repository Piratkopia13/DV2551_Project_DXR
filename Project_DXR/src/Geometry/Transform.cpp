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
}

void Transform::setTranslation(const DirectX::XMVECTOR translation) {
	m_translation = translation;
}

void Transform::scale(const DirectX::XMVECTOR scale) {
	m_scale *= scale;
}

void Transform::scaleUniformly(const float scale) {
	m_scale *= scale;
}

void Transform::setScale(const DirectX::XMVECTOR scale) {
	m_scale = scale;
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
	m_transformMatrix = matrix;
}

const DirectX::XMFLOAT3 Transform::getTranslation() const {
	XMFLOAT3 toReturn;
	XMStoreFloat3(&toReturn, m_translation);
	return toReturn;
}

const DirectX::XMFLOAT3 Transform::getRotation() const {
	XMFLOAT3 toReturn;
	XMStoreFloat3(&toReturn, m_rotation);
	return toReturn;
}

const DirectX::XMFLOAT3 Transform::getScale() const {
	XMFLOAT3 toReturn;
	XMStoreFloat3(&toReturn, m_scale);
	return toReturn;
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
