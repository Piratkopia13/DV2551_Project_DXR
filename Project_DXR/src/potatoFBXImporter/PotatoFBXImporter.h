#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
class PotatoModel {
public:
	struct Vertex {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texCoord;
	};

	PotatoModel() {

	};

	PotatoModel(std::vector<PotatoModel::Vertex> _data) {
		data = _data;
	};

	~PotatoModel() {};

	void addVertex(Vertex vertex) {
		data.push_back(vertex);
	};


	const std::vector<PotatoModel::Vertex> getModelData() {
		return data;
	}

private:

	std::vector<PotatoModel::Vertex> data;

};


class PotatoFBXImporter{
public:
	PotatoFBXImporter();
	~PotatoFBXImporter();

	PotatoModel* importModelFromScene(std::string fileName);
	PotatoModel* importStaticModelFromScene(std::string fileName);

	PotatoModel* createDefaultPyramid(float width = 1.0f, float height = 1.0f);
	PotatoModel* createDefaultCube(float width = 1.0f, float height = 1.0f);



private:

	FbxManager* m_Manager;
	FbxIOSettings* m_IOSettings;


	void traverse( );
	std::string GetAttributeTypeName(FbxNodeAttribute::EType type) {
		switch (type) {
		case FbxNodeAttribute::eUnknown: return "unidentified";
		case FbxNodeAttribute::eNull: return "null";
		case FbxNodeAttribute::eMarker: return "marker";
		case FbxNodeAttribute::eSkeleton: return "skeleton";
		case FbxNodeAttribute::eMesh: return "mesh";
		case FbxNodeAttribute::eNurbs: return "nurbs";
		case FbxNodeAttribute::ePatch: return "patch";
		case FbxNodeAttribute::eCamera: return "camera";
		case FbxNodeAttribute::eCameraStereo: return "stereo";
		case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
		case FbxNodeAttribute::eLight: return "light";
		case FbxNodeAttribute::eOpticalReference: return "optical reference";
		case FbxNodeAttribute::eOpticalMarker: return "marker";
		case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
		case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
		case FbxNodeAttribute::eBoundary: return "boundary";
		case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
		case FbxNodeAttribute::eShape: return "shape";
		case FbxNodeAttribute::eLODGroup: return "lodgroup";
		case FbxNodeAttribute::eSubDiv: return "subdiv";
		default: return "unknown";
		}
	}
	std::string PrintAttribute(FbxNodeAttribute* pAttribute) {
		if (!pAttribute) return "";

		std::string typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
		std::string attrName = pAttribute->GetName();

		// Note: to retrieve the character array of a FbxString, use its Buffer() method.
		return typeName + " " + attrName;
	}
};

