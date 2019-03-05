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
		fbxsdk::FbxUInt64 uniqueID;
		int parentIndex;
		unsigned int currentFrame = 0;
		std::vector<int> childIndexes;
		std::vector<DirectX::XMMATRIX> animation;
		DirectX::XMMATRIX globalBindposeInverse;
		DirectX::XMMATRIX& getAnimation() {
			return animation[currentFrame];
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
	void addFrame(unsigned int index, XMMATRIX matrix);
	void addConnection(int ControlPointIndex, int boneIndex, float weight);
	void reSizeControlPoints(int size);



	const std::vector<PotatoModel::Vertex>& getModelData();
	const std::vector<PotatoModel::Vertex>& getModelVertices();
	std::vector<PotatoModel::Limb>& getSkeleton();
	const std::vector<unsigned int>& getModelIndices();


	PotatoModel::Limb* findLimb(fbxsdk::FbxUInt64 id);;
	int findLimbIndex(fbxsdk::FbxUInt64 id);

private:

	std::vector<PotatoModel::Vertex> m_Data;
	std::vector<PotatoModel::Vertex> m_currentData;
	std::vector<PotatoModel::Limb> m_Skeleton;
	std::vector<PotatoModel::LimbConnection> m_ConnectionData;
	std::vector<std::vector<unsigned int>> m_ControlPointIndexes;
	std::vector<unsigned int> indexes;

	float m_FrameTime;
	
	void updateLimb(int index, XMMATRIX& matrix, unsigned int d);
	void updateVertexes();


	int exists(PotatoModel::Vertex _vert);
};