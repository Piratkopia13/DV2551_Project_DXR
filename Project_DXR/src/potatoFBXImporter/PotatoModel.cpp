#include <pch.h>
#include "PotatoModel.h"

PotatoModel::PotatoModel() {
}
PotatoModel::PotatoModel(std::vector<PotatoModel::Vertex> _data) {

}
PotatoModel::~PotatoModel() {

}

const std::vector<PotatoModel::Vertex>& PotatoModel::getMesh(int animation, float t) {
	if (!(cache.index == animation && cache.time == t)) {
		updateVertexes(animation, t);
		cache.index = animation;
		cache.time = t;
	}
	return m_currentData;
}

void PotatoModel::addVertex(Vertex vertex, int controlPointIndex) {
	int index = exists(vertex);

	if (index == -1) {
		m_Data.push_back(vertex);
		m_currentData.push_back(vertex);
		indexes.push_back(static_cast<unsigned int>(m_Data.size()) - 1);
		index = m_Data.size() - 1;
	}
	else {
		indexes.push_back(static_cast<unsigned int>(index));
	}
}
void PotatoModel::addControlPoint(DirectX::XMFLOAT3 position, unsigned int index) {
	for (int i = 0; i < m_Data.size(); i++) {
		if (position.x == m_Data[i].position.x && position.y == m_Data[i].position.y && position.z == m_Data[i].position.z) {
			m_ControlPointIndexes[index].push_back(i);
		}
	}
}
void PotatoModel::addBone(Limb limb) {
	if (m_Skeleton.size() > 0)
		m_Skeleton[limb.parentIndex].childIndexes.push_back(m_Skeleton.size());
	m_Skeleton.push_back(limb);

}
void PotatoModel::addFrame(unsigned int animationIndex, unsigned int limbIndex, float time, XMMATRIX matrix) {
	m_Skeleton[limbIndex].animationStack[animationIndex].animation.push_back({time, matrix});
	m_Skeleton[limbIndex].animationStack[animationIndex].duration = time;
	if (m_maxTime[animationIndex] < time) {
		m_maxTime[animationIndex] = time;
	}
}
void PotatoModel::addConnection(int ControlPointIndex, int boneIndex, float weight) {
	m_ConnectionData[ControlPointIndex].indexes.push_back(boneIndex);
	m_ConnectionData[ControlPointIndex].weights.push_back(weight);
}

void PotatoModel::reSizeControlPoints(size_t size) {
	m_ControlPointIndexes.resize(size);
	m_ConnectionData.resize(size);
}
void PotatoModel::reSizeAnimationStack(size_t size) {
	for (Limb&limb : m_Skeleton) {
		limb.animationStack.resize(size);
	}
	m_maxTime.resize(size);
}
void PotatoModel::setGlobalBindposeInverse(unsigned int index, XMMATRIX matrix) {
	m_Skeleton[index].globalBindposeInverse = matrix;
}



const std::vector<PotatoModel::Vertex>& PotatoModel::getModelData() {
	return m_Data;
}
const std::vector<PotatoModel::Vertex>& PotatoModel::getModelVertices() {
	return m_currentData;
}
const std::vector<unsigned int>& PotatoModel::getModelIndices() {
	return indexes;
}

size_t PotatoModel::getStackSize() {
	return m_maxTime.size();
}

float PotatoModel::getMaxTime(size_t index) {
	return m_maxTime[index];
}

std::vector<PotatoModel::Limb>& PotatoModel::getSkeleton() {
	return m_Skeleton;
}

PotatoModel::Limb * PotatoModel::findLimb(fbxsdk::FbxUInt64 id) {
	for (Limb limb : m_Skeleton) {
		if (limb.uniqueID == id)
			return &limb;
	}
	return nullptr;
}
int PotatoModel::findLimbIndex(fbxsdk::FbxUInt64 id) {
	int size = m_Skeleton.size();
	for (int i = 0; i < size; i++) {
		if (m_Skeleton[i].uniqueID == id)
			return i;
	}
	return -1;
}

void PotatoModel::normalizeWeights() {
	for (int i = 0; i < m_ConnectionData.size(); i++) {
		float sum = 0.0f;
		
		for (int j = 0; j < m_ConnectionData[i].weights.size();j++) {
			sum += m_ConnectionData[i].weights[j];
		}
		for (int j = 0; j < m_ConnectionData[i].weights.size(); j++) {
			m_ConnectionData[i].weights[j] /= sum;
		}
	}
}

void PotatoModel::updateVertexes(int animation, float t) {
	for (unsigned int cpi = 0; cpi < m_ControlPointIndexes.size(); cpi++) {
		XMMATRIX sum = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		float weight = 0.0f;
		for (int i = 0; i < m_ConnectionData[cpi].indexes.size(); i++) {
			Limb&limb = m_Skeleton[m_ConnectionData[cpi].indexes[i]];
			sum += limb.getTransform(animation, t) * m_ConnectionData[cpi].weights[i];
			weight += m_ConnectionData[cpi].weights[i];
		}
		XMMATRIX invSum = XMMatrixInverse(nullptr, XMMatrixTranspose(sum));
		for (unsigned int index = 0; index < m_ControlPointIndexes[cpi].size(); index++) {
			int dataIndex = m_ControlPointIndexes[cpi][index];
			
			XMStoreFloat3(&m_currentData[dataIndex].position, XMVector3Transform(XMLoadFloat3(&m_Data[dataIndex].position), sum));
			XMStoreFloat3(&m_currentData[dataIndex].normal, XMVector3Transform(XMLoadFloat3(&m_Data[dataIndex].normal), invSum));
		}
	}




}

int PotatoModel::exists(PotatoModel::Vertex _vert) {
	for (int i = m_Data.size() - 1; i >= 0; i--) {
		if (_vert == m_Data[i])
			return i;
	}
	return -1;
}
