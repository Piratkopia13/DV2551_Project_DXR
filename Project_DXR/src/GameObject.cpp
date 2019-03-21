#include "pch.h"
#include "GameObject.h"


GameObject::GameObject() {
	m_model = nullptr;
}

GameObject::GameObject(PotatoModel * model, size_t _animationIndex, XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale) {
	m_model = model;
	m_update = true;
	m_animationIndex = _animationIndex;
	m_animationTime = 0.0f;
	m_maxAnimationTime = model->getMaxTime(m_animationIndex);
	m_transform.setTranslation(XMLoadFloat3(&position));
	m_transform.setScale(XMLoadFloat3(&scale));
}

GameObject::GameObject(PotatoModel * model, size_t _animationIndex, Transform & transform) {
	m_model = model;
	m_update = true;
	m_animationIndex = _animationIndex;
	m_animationTime = 0.0f;
	m_maxAnimationTime = model->getMaxTime(m_animationIndex);
	m_transform = Transform(transform);
}


GameObject::~GameObject() {
}

void GameObject::update(float d) {
	if (m_update) {
		m_animationTime += d;
		if (d > 1.0) {
			std::cout << d << std::endl;
		}
		if (m_animationTime > m_maxAnimationTime) {
			m_animationTime = m_animationTime - (int(m_animationTime / m_maxAnimationTime)*m_maxAnimationTime);
			//std::cout << "max" << std::endl;
		}

	}

}

PotatoModel * GameObject::getModel() {
	return m_model;
}


int& GameObject::getAnimationIndex() {
	return m_animationIndex;
}
bool & GameObject::getAnimationUpdate()
{
	return m_update;
	// TODO: insert return statement here
}
float& GameObject::getAnimationTime() {
	return m_animationTime;
}

float & GameObject::getMaxAnimationTime()
{
	return m_maxAnimationTime;
	// TODO: insert return statement here
}

void GameObject::setAnimationIndex(size_t index) {
	m_animationIndex = index;
	if (m_model)
		m_maxAnimationTime = m_model->getMaxTime(index);
	}

void GameObject::setAnimationUpdate(const bool state) {
	m_update = state;
}

void GameObject::setAnimationTime(const float time) {
	m_animationTime = time - (int(time / m_maxAnimationTime)*m_maxAnimationTime);
}

Transform& GameObject::getTransform() {
	return m_transform;
}

const XMMATRIX & GameObject::getTransformMatrix() {
	return m_transform.getTransformMatrix();
}

