#include <pch.h>
#include "PotatoModel.h"

PotatoModel::PotatoModel() {
	m_FrameTime = 0.0f;
}

PotatoModel::PotatoModel(std::vector<PotatoModel::Vertex> _data) {

}

PotatoModel::~PotatoModel() {

}

void PotatoModel::update(float d) {
	m_FrameTime += d;
	updateLimb(0, XMMatrixIdentity(), m_FrameTime);


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

void PotatoModel::setGlobalBindposeInverse(unsigned int index, XMMATRIX matrix) {
	m_Skeleton[index].globalBindposeInverse = matrix;
}

void PotatoModel::addFrame(unsigned int index, XMMATRIX matrix) {
	m_Skeleton[index].animation.push_back(matrix);
}

void PotatoModel::addConnection(int ControlPointIndex, int boneIndex, float weight) {
	m_ConnectionData[ControlPointIndex].indexes.push_back(boneIndex);
	m_ConnectionData[ControlPointIndex].weights.push_back(weight);
}

void PotatoModel::reSizeControlPoints(int size) {
	m_ControlPointIndexes.resize(size);
	m_ConnectionData.resize(size);
}

const std::vector<PotatoModel::Vertex>& PotatoModel::getModelData() {
	return m_currentData;
}

const std::vector<PotatoModel::Vertex>& PotatoModel::getModelVertices() {
	return m_Data;
}

std::vector<PotatoModel::Limb>& PotatoModel::getSkeleton() {
	return m_Skeleton;
}

const std::vector<unsigned int>& PotatoModel::getModelIndices() {
	return indexes;
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

void PotatoModel::updateLimb(int index, XMMATRIX & matrix, unsigned int frame) {
	m_Skeleton[index].animation[frame];

	for (int i = 0; i < m_Skeleton[index].childIndexes.size(); i++) {
		updateLimb(i, matrix, frame);
	}
}

void PotatoModel::updateVertexes() {

	for (unsigned int cpi = 0; cpi < m_ControlPointIndexes.size(); cpi++) {
		
		XMMATRIX sum = XMMatrixIdentity();
		for (int i = 0; i < m_ConnectionData[cpi].indexes.size(); i++) {
			Limb&limb = m_Skeleton[m_ConnectionData[cpi].indexes[i]];
			sum += limb.getAnimation()*m_ConnectionData[cpi].weights[i];
		}

		for (unsigned int index = 0; index < m_ControlPointIndexes[cpi].size(); index++) {
			int dataIndex = m_ControlPointIndexes[cpi][index];

			XMStoreFloat3(&m_currentData[dataIndex].position, XMVector3Transform(XMLoadFloat3(&m_Data[dataIndex].position),sum));
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
