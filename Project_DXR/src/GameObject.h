#pragma once
#include "Geometry/Transform.h"
#include "potatoFBXImporter/PotatoModel.h"

class GameObject
{
public:
	GameObject();
	GameObject(PotatoModel*model, size_t _animationIndex = 0, XMFLOAT3 position = {0, 0, 0}, XMFLOAT3 rotation = {0, 0, 0}, XMFLOAT3 scale = {1, 1, 1});
	GameObject(PotatoModel*model, size_t _animationIndex, Transform & transform);
	~GameObject();

	void update(float d);

	PotatoModel* getModel();
	int& getAnimationIndex();
	int& getMaxAnimationIndex();
	bool& getAnimationUpdate();
	float& getAnimationTime();
	float& getMaxAnimationTime();
	
	void setAnimationIndex(size_t index);
	void setAnimationUpdate(const bool state);
	void setAnimationTime(const float time);


	Transform& getTransform();
	const XMMATRIX& getTransformMatrix();
private:

	Transform m_transform;
	int m_animationIndex;
	float m_animationTime;
	float m_maxAnimationTime;
	bool m_update;
	
	PotatoModel* m_model;


};

