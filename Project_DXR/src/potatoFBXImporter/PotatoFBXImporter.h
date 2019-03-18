#pragma once

#include "PotatoModel.h"


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
	void fetchSkeletonRecursive(FbxNode* inNode, PotatoModel* model, int inDepth, int myIndex, int inParentIndex);
	void fetchTextures(FbxNode* node, PotatoModel* model);

	static XMMATRIX convertToXMMatrix(const FbxAMatrix& pSrc)
	{
		return XMMatrixSet(
			static_cast<FLOAT>(pSrc.Get(0, 0)), static_cast<FLOAT>(pSrc.Get(0, 1)), static_cast<FLOAT>(pSrc.Get(0, 2)), static_cast<FLOAT>(pSrc.Get(0, 3)),
			static_cast<FLOAT>(pSrc.Get(1, 0)), static_cast<FLOAT>(pSrc.Get(1, 1)), static_cast<FLOAT>(pSrc.Get(1, 2)), static_cast<FLOAT>(pSrc.Get(1, 3)),
			static_cast<FLOAT>(pSrc.Get(2, 0)), static_cast<FLOAT>(pSrc.Get(2, 1)), static_cast<FLOAT>(pSrc.Get(2, 2)), static_cast<FLOAT>(pSrc.Get(2, 3)),
			static_cast<FLOAT>(pSrc.Get(3, 0)), static_cast<FLOAT>(pSrc.Get(3, 1)), static_cast<FLOAT>(pSrc.Get(3, 2)), static_cast<FLOAT>(pSrc.Get(3, 3)));
	}



	void printAnimationStack(FbxNode* node);






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

