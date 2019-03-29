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
		struct AnimationStack {
			enum Interpolation {LINEAR, QUADRATIC};
			struct PotatoFrame {
				float time = 0.0f;
				XMMATRIX transform;
			};
			std::vector<PotatoFrame> animation;
			Interpolation interpolation = LINEAR;
			float duration = 0.0f;
		};
		struct Cache {
			XMMATRIX transform = XMMatrixIdentity();
			int index = 0;
			float time = 0;
		}cache;
		fbxsdk::FbxUInt64 uniqueID;
		int parentIndex;
		std::vector<int> childIndexes;
		std::vector<AnimationStack> animationStack;
		DirectX::XMMATRIX globalBindposeInverse;
		
		XMMATRIX getTransform(int index, float t) {
			if (index >= animationStack.size() || index < 0)
				return XMMatrixIdentity();
			if (animationStack[index].duration == 0.0f)
				return XMMatrixIdentity();
			if (t >= animationStack[index].duration)
				return XMMatrixIdentity();
			if (animationStack[index].animation.size() == 0)
				return XMMatrixIdentity();
			if (t == cache.time && index == cache.index)
				return cache.transform;
			int frame0 = getFrameFromTime(index, t);
			if (frame0 == -1)
				return XMMatrixIdentity();
			int frame1 = (frame0 + 1) % animationStack[index].animation.size();
			//std::cout << index << " " << t << " " << frame0 << " " << frame1 << std::endl;
			XMMATRIX transform = interpolate(index, frame0, frame1, getWeight(index, frame0, frame1, t));
			cache.transform = globalBindposeInverse*transform;
			cache.index = index;
			cache.time = t;
			return globalBindposeInverse * transform;
		}
	private: 
		int getFrameFromTime(size_t index, float t) {
			for (int i = 0; i < animationStack[index].animation.size(); i++) {
				if (animationStack[index].animation[i].time <= t && 
					animationStack[index].animation[(i + 1)%animationStack[index].animation.size()].time >= t) {
					return i;
				}
			}
			return -1;
		}
		float getWeight(size_t index, int f0, int f1, float time) {
			if (animationStack[index].interpolation == AnimationStack::Interpolation::LINEAR) {
				//float w = ((expiredTime - getTime(0)) / (getTime(1) - getTime(0)));
				return ((time - animationStack[index].animation[f0].time) / (animationStack[index].animation[f1].time - animationStack[index].animation[f0].time));
			}
			return -1.0f;
		}
		XMMATRIX interpolate(size_t index, int f0, int f1, float weight) {
			switch (animationStack[index].interpolation) {
				case AnimationStack::Interpolation::LINEAR: {
					return interpLinear(animationStack[index].animation[f0].transform, animationStack[index].animation[f1].transform, weight);
				}
				/*case AnimationStack::Interpolation::QUADRATIC: {
					return interpQuad(getFrame(-1),getFrame(0), getFrame(1), getFrame(2), getWeight(index, f0, f1));
				}*/
				default: {

					return XMMatrixIdentity();
				}
			}
		}
		XMMATRIX interpLinear(XMMATRIX & m1, XMMATRIX & m2, float t) {
			/*if (t > 1.0f || t < 0.0) {
				std::cout << t << std::endl;
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
		//XMMATRIX interpQuad(XMMATRIX & m1, XMMATRIX & m2, XMMATRIX & m3, XMMATRIX & m4, float t) {
		//	XMVECTOR scal1, scal2, scal3, scal4; //for scaling
		//	XMVECTOR quat1, quat2, quat3, quat4; //for rotation
		//	XMVECTOR tran1, tran2, tran3, tran4; //for translation
		//	XMMatrixDecompose(&scal1, &quat1, &tran1, m1);
		//	XMMatrixDecompose(&scal2, &quat2, &tran2, m2);
		//	XMMatrixDecompose(&scal3, &quat3, &tran3, m3);
		//	XMMatrixDecompose(&scal4, &quat4, &tran4, m4);
		//	return XMMatrixIdentity()*
		//		XMMatrixScalingFromVector(XMVectorCatmullRom(scal1, scal2, scal3, scal3, t))*
		//		XMMatrixRotationQuaternion(XMQuaternionSquad(quat1, quat2, quat3, quat4, t))*
		//		XMMatrixTranslationFromVector(XMVectorCatmullRom(tran1, tran2, tran3, tran4, t));
		//}
	};

	PotatoModel();
	PotatoModel(std::vector<PotatoModel::Vertex> _data);
	~PotatoModel();

	const std::vector<PotatoModel::Vertex>& getMesh(int animation = -1, float t = 0.0f);
	void addVertex(Vertex vertex, int controlPointIndex);
	void addControlPoint(DirectX::XMFLOAT3 position, unsigned int index);
	void addBone(Limb limb);
	void setGlobalBindposeInverse(unsigned int limbIndex, XMMATRIX matrix);
	void addFrame(unsigned int animationIndex, unsigned int limbIndex, float time, XMMATRIX matrix);
	void addConnection(int ControlPointIndex, int boneIndex, float weight);
	void reSizeControlPoints(size_t size);
	void reSizeAnimationStack(size_t size);

	void addSpecularTexture(const std::string& name);
	void addDiffuseTexture(const std::string& name);
	void addNormalTexture(const std::string& name);



	const std::vector<PotatoModel::Vertex>& getModelData();
	const std::vector<PotatoModel::Vertex>& getModelVertices();
	const std::vector<std::string>& getSpecularTextureNames();
	const std::vector<std::string>& getDiffuseTextureNames();
	const std::vector<std::string>& getNormalTextureNames();


	std::vector<PotatoModel::Limb>& getSkeleton();
	const std::vector<unsigned int>& getModelIndices();
	size_t getStackSize();
	float getMaxTime(size_t index);

	//PotatoModel::Limb* findLimb(fbxsdk::FbxUInt64 id);
	int findLimbIndex(fbxsdk::FbxUInt64 id);
	void normalizeWeights();
private:
	std::vector<float> m_maxTime;
	std::vector<PotatoModel::Vertex> m_Data;
	std::vector<PotatoModel::Vertex> m_currentData;
	std::vector<PotatoModel::Limb> m_Skeleton;
	std::vector<PotatoModel::LimbConnection> m_ConnectionData;
	std::vector<std::vector<unsigned int>> m_ControlPointIndexes;
	std::vector<unsigned int> indexes;

	std::vector<std::string> m_specularTextureNames;
	std::vector<std::string> m_diffuseTextureNames;
	std::vector<std::string> m_normalTextureNames;
	struct Cache {
		int index = -1;
		float time = 0.0f;
	}cache;
	
	void updateVertexes(int animation, float t);

	int exists(PotatoModel::Vertex _vert);
};