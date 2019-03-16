#include "pch.h"
#include "GameObject.h"


GameObject::GameObject() {
	m_model = nullptr;
}

GameObject::GameObject(PotatoModel * model, size_t _animationIndex, XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale) {
	m_model = model;
	m_animationIndex = _animationIndex;
	m_animationTime = 0.0f;
	m_maxAnimationTime = model->getMaxTime(m_animationIndex);
	m_transform.setTranslation(XMLoadFloat3(&position));
}


GameObject::~GameObject() {
}

void GameObject::update(float d) {
	m_animationTime += d;
	if (d > 1.0) {
		std::cout << d << std::endl;
	}
	if (m_animationTime > m_maxAnimationTime) {
		m_animationTime = m_animationTime - (int(m_animationTime / m_maxAnimationTime)*m_maxAnimationTime);
		//std::cout << "max" << std::endl;
	}

}

PotatoModel * GameObject::getModel() {
	return m_model;
}


int GameObject::getAnimationIndex() {
	return m_animationIndex;
}
float GameObject::getAnimationTime() {
	return m_animationTime;
}
void GameObject::setAnimationIndex(size_t index) {
	m_animationIndex = index;
	if (m_model)
		m_maxAnimationTime = m_model->getMaxTime(index);
}

const XMMATRIX& GameObject::getTransform() {
	return m_transform.getTransformMatrix();
}

