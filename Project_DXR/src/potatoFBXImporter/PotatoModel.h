#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

class PotatoModel {
public:
	struct Vertex {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texCoord;
		bool operator==(Vertex& other) {
			return position.x == other.position.x && position.y == other.position.y && position.z == other.position.z
				&& normal.x == other.normal.x && normal.y == other.normal.y && normal.z == other.normal.z
				&& texCoord.x == other.texCoord.x && texCoord.y == other.texCoord.y;
		}
	};
	struct LimbConnection {
		std::vector<int> indexes;
		std::vector<float> weights;
	};
	struct Limb {
		enum Interpolation {LINEAR, QUADRATIC};
		struct PotatoFrame {
			float time = 0.0;
			XMMATRIX transform;
		};
		fbxsdk::FbxUInt64 uniqueID;
		int parentIndex;
		std::vector<int> childIndexes;
		float expiredTime = 0.0f;
		float animationTime = 1.0f;
		int currentFrame = 0;
		std::vector<std::vector<PotatoFrame>> animationStack = {{}};
		std::vector<float> animationDuration;
		DirectX::XMMATRIX globalBindposeInverse;
		Interpolation interpolation = LINEAR;
		XMMATRIX getTransform(size_t index, float t) {
			if (index < animationStack.size()) {
				if (t <= animationDuration[index]) {

				}
			}
			return XMMatrixIdentity();
		}
		void update(float t) {
			if (t > 10.0f)
				return;
			//t = 0.04f;

			//assert(currentFrame <= 50);

			expiredTime += t;
			if (expiredTime >= animationTime) {
				expiredTime -= animationTime;
				currentFrame = 0;
			}
			if (animationStack[0].size() > 0) {
				if (expiredTime >= getTime(1)) {
					currentFrame++;
				}
			}

		}
		XMMATRIX getCurrentTransform() {
			if (animationStack[0].size() > 0)
				return globalBindposeInverse*interpolate();
			else
				return globalBindposeInverse;
		}

	private: 

		XMMATRIX& getFrame(int step) {
			return animationStack[0][(currentFrame + step) % animationStack[0].size()].transform;
		}
		float getLinearWeight() {
			float w = ((expiredTime - getTime(0)) / (getTime(1) - getTime(0)));
			//assert(w >= 0 && w <= 1);
			return w;
		}
		float getQuadWeight() {
			return ((expiredTime - getTime(0)) / (getTime(1) - getTime(0))*0.5f) + 0.25f;
		}
		float getTime(int step) {
			return animationStack[0][(currentFrame + step) % animationStack[0].size()].time;
		}

		XMMATRIX interpolate() {
			switch (interpolation) {
				case LINEAR: {
					return interpLinear(getFrame(0), getFrame(1), getLinearWeight());
				}
				case QUADRATIC: {
					return interpQuad(getFrame(-1),getFrame(0),getFrame(1),getFrame(2),getQuadWeight());
				}
				default: {

					return XMMatrixIdentity();
				}
			}
		}
		XMMATRIX interpLinear(XMMATRIX & m1, XMMATRIX & m2, float t) {
			/*if (uniqueID == 70) {
				std::cout << t << std::endl;
				XMFLOAT4X4 asd;
				XMStoreFloat4x4(&asd, m1);
				std::cout << "m1: " << asd._11 << ", " << asd._12 << ", " << asd._13 << ", " << asd._14 << ", " << asd._21 << ", " << asd._22 << ", " << asd._23 << ", " << asd._24 << ", " << asd._31 << ", " << asd._32 << ", " << asd._33 << ", " << asd._34 << ", " << asd._41 << ", " << asd._42 << ", " << asd._43 << ", " << asd._44 << ", " << std::endl;
				XMStoreFloat4x4(&asd, m2);
				std::cout << "m2: " << asd._11 << ", " << asd._12 << ", " << asd._13 << ", " << asd._14 << ", " << asd._21 << ", " << asd._22 << ", " << asd._23 << ", " << asd._24 << ", " << asd._31 << ", " << asd._32 << ", " << asd._33 << ", " << asd._34 << ", " << asd._41 << ", " << asd._42 << ", " << asd._43 << ", " << asd._44 << ", " << std::endl;
			}*/

			XMVECTOR scal1, scal2; //for scaling
			XMVECTOR quat1, quat2; //for rotation
			XMVECTOR tran1, tran2; //for translation
			XMMatrixDecompose(&scal1, &quat1, &tran1, m1);
			XMMatrixDecompose(&scal2, &quat2, &tran2, m2);
			return XMMatrixIdentity()*
				XMMatrixScalingFromVector(XMVectorLerp(scal1, scal2, t))*
				XMMatrixRotationQuaternion(XMQuaternionSlerp(quat1, quat2, t))*
				XMMatrixTranslationFromVector(XMVectorLerp(tran1, tran2, t));
		}
		XMMATRIX interpQuad(XMMATRIX & m1, XMMATRIX & m2, XMMATRIX & m3, XMMATRIX & m4, float t)
		{
			XMVECTOR scal1, scal2, scal3, scal4; //for scaling
			XMVECTOR quat1, quat2, quat3, quat4; //for rotation
			XMVECTOR tran1, tran2, tran3, tran4; //for translation
			XMMatrixDecompose(&scal1, &quat1, &tran1, m1);
			XMMatrixDecompose(&scal2, &quat2, &tran2, m2);
			XMMatrixDecompose(&scal3, &quat3, &tran3, m3);
			XMMatrixDecompose(&scal4, &quat4, &tran4, m4);
			return XMMatrixIdentity()*
				XMMatrixScalingFromVector(XMVectorCatmullRom(scal1, scal2, scal3, scal3, t))*
				XMMatrixRotationQuaternion(XMQuaternionSquad(quat1, quat2, quat3, quat4, t))*
				XMMatrixTranslationFromVector(XMVectorCatmullRom(tran1, tran2, tran3, tran4, t));
		}
	};

	PotatoModel();
	PotatoModel(std::vector<PotatoModel::Vertex> _data);
	~PotatoModel();

	void update(float d);

	void addVertex(Vertex vertex, int controlPointIndex);
	void addControlPoint(DirectX::XMFLOAT3 position, unsigned int index);
	void addBone(Limb limb);
	void setGlobalBindposeInverse(unsigned int index, XMMATRIX matrix);
	void addFrame(unsigned int index, float time, XMMATRIX matrix);
	void addConnection(int ControlPointIndex, int boneIndex, float weight);
	void reSizeControlPoints(int size);



	const std::vector<PotatoModel::Vertex>& getModelData();
	const std::vector<PotatoModel::Vertex>& getModelVertices();
	std::vector<PotatoModel::Limb>& getSkeleton();
	const std::vector<unsigned int>& getModelIndices();


	PotatoModel::Limb* findLimb(fbxsdk::FbxUInt64 id);;
	int findLimbIndex(fbxsdk::FbxUInt64 id);
	void normalizeWeights();
private:

	std::vector<PotatoModel::Vertex> m_Data;
	std::vector<PotatoModel::Vertex> m_currentData;
	std::vector<PotatoModel::Limb> m_Skeleton;
	std::vector<PotatoModel::LimbConnection> m_ConnectionData;
	std::vector<std::vector<unsigned int>> m_ControlPointIndexes;
	std::vector<unsigned int> indexes;

	float m_FrameTime;
	
	void updateLimb(int index, XMMATRIX& matrix, float t);
	void updateVertexes();

	XMMATRIX interpM(XMMATRIX& m1, XMMATRIX& m2, float t);
	XMMATRIX interpQuad(XMMATRIX& m1, XMMATRIX& m2, XMMATRIX& m3, XMMATRIX& m4, float t);
	int exists(PotatoModel::Vertex _vert);



public:

	class Animation {
	public:


	private:



	};











};