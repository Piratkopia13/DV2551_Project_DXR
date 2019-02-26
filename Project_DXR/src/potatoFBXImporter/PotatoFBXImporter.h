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
			return position.x == other.position.x && position.y == other.position.y && position.z == other.position.z;
		}
	};
	struct LimbConnection {
		std::vector<int> indexes;
		std::vector<float> weights;
	};
	struct Limb {
		Limb*parent;
		std::vector<Limb*> children;
		FbxMatrix mat;
	};

	PotatoModel() {

	};

	PotatoModel(std::vector<PotatoModel::Vertex> _data) {
		data = _data;
	};

	~PotatoModel() {};

	void addVertex(Vertex vertex) {
		int index = exists(vertex);
		if (index == -1) {
			data.push_back(vertex);
			indexes.push_back(data.size() - 1);
		}
		else {
			indexes.push_back(index);
		}
	};


	const std::vector<PotatoModel::Vertex>& getModelData() {
		return data;
	}

private:

	std::vector<PotatoModel::Vertex> data;
	std::vector<PotatoModel::LimbConnection> connectionData;
	std::vector<PotatoModel::Limb> limbData;
	std::vector<PotatoModel::Vertex*> controlPoints;
	std::vector<int> indexes;

	int exists(PotatoModel::Vertex _vert) {
		for (int i = data.size() - 1; i >= 0; i--) {
			if (_vert == data[i])
				return i;
		}
		return -1;
	}

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


	bool validFile(std::string fileName);
	FbxScene* makeScene(std::string fileName, std::string sceneName);
	std::string getObjName(std::string fileName);
	

	void traverse(FbxNode* node, PotatoModel* model);
	FbxVector2 getTexCoord(int cpIndex, FbxGeometryElementUV* geUV, FbxMesh* mesh, int polyIndex, int vertIndex) const;
	void fetchGeometry(FbxNode* mesh, PotatoModel* model, const std::string& filename);
	void fetchSkeleton(FbxNode* mesh, PotatoModel* model, const std::string& filename);
















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

		return typeName + " " + attrName;
	}




};

