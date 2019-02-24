#pragma once

#include <DirectXMath.h>
#include <DirectXMathVector.inl>

class Transform {
public:
	Transform();
	~Transform();

	void translate(const DirectX::XMVECTOR translation);
	void setTranslation(const DirectX::XMVECTOR translation);
	
	void scale(const DirectX::XMVECTOR scale);
	void scaleUniformly(const float scale);
	void setScale(const DirectX::XMVECTOR scale);

	void rotateAroundX(const float radians);
	void rotateAroundY(const float radians);
	void rotateAroundZ(const float radians);
	void setRotation(float x, float y, float z);
	void setTransformMatrix(DirectX::XMMATRIX matrix);

	const DirectX::XMFLOAT3 getTranslation() const;
	const DirectX::XMFLOAT3 getRotation() const;
	const DirectX::XMFLOAT3 getScale() const;
	const DirectX::XMMATRIX& getTransformMatrix();

private:
	bool m_needsUpdate;

	DirectX::XMVECTOR m_translation;
	DirectX::XMVECTOR m_rotation;
	DirectX::XMVECTOR m_scale;

	DirectX::XMMATRIX m_transformMatrix;
};