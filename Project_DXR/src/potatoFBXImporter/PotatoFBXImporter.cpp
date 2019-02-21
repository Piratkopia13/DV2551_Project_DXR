#include <pch.h>

#include "PotatoFBXImporter.h"
//#include <Windows.h>

using namespace std;
PotatoFBXImporter::PotatoFBXImporter() {
	m_Manager = FbxManager::Create();
	m_IOSettings = FbxIOSettings::Create(m_Manager, IOSROOT);
	

}

PotatoFBXImporter::~PotatoFBXImporter() {
	if(m_Manager)
		m_Manager->Destroy();
}

PotatoModel* PotatoFBXImporter::importModelFromScene(std::string fileName) {
	if (fileName.find(".fbx") != fileName.size()-4) {
		#if _DEBUG
			cout << "Not an Fbx File: " << fileName << endl;
		#endif
		return nullptr;
	}



	PotatoModel* model = new PotatoModel();
	FbxImporter* importer = FbxImporter::Create(m_Manager, "");
	if (!importer->Initialize(fileName.c_str(), -1, m_Manager->GetIOSettings())) {
		importer->Destroy();

		#if _DEBUG
			std::cout << "Could not Load '" + fileName + "'" << std::endl;
		#endif

		return nullptr;
	}
	FbxScene* lScene = FbxScene::Create(m_Manager, "myScene");
	importer->Import(lScene);
	importer->Destroy();

	#if _DEBUG
		std::cout << "Loaded '" + fileName + "'" << std::endl;
	#endif
	FbxNode * root = lScene->GetRootNode();
	FbxMesh* lMesh = FbxMesh::Create(lScene, "mesh");
	
	#if _DEBUG

	cout << "Children: " << to_string(root->GetChildCount());
	string attributes = "";
	for (int i = 0; i < root->GetNodeAttributeCount(); i++)
		attributes += " " + PrintAttribute(root->GetNodeAttributeByIndex(i));
		
		
	cout << attributes << endl;
	for (int i = 0; i < root->GetChildCount(); i++) {
		FbxNode* child = root->GetChild(i);
		cout << "\tChildren: " << to_string(child->GetChildCount());
		string attributesC = "";
		for (int i = 0; i < child->GetNodeAttributeCount(); i++)
			attributesC += " " + PrintAttribute(child->GetNodeAttributeByIndex(i));
		cout << attributesC << " ";
		
		cout << to_string(child->GetMesh()->GetControlPointsCount());




		cout << endl;
	}

	std::cout << " ControlPoints: " << std::to_string(lMesh->GetControlPointsCount()) << std::endl;
	#endif

	return model;
}

PotatoModel * PotatoFBXImporter::importStaticModelFromScene(std::string fileName) {
	
	LARGE_INTEGER frequency;
	LARGE_INTEGER start, end;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);
	
	if (!validFile(fileName)) {
		return nullptr;
	}

	std::string objName = getObjName(fileName);
	FbxScene* scene = makeScene(fileName, objName+"Scene");
	if (!scene) {
		return nullptr;
	}
	else {
		#if _DEBUG
		std::cout << "Loaded '" + objName + "'" << std::endl;
		#endif
	}

	PotatoModel* model = new PotatoModel();
	
	FbxNode * root = scene->GetRootNode();

	traverse(root);

	//for (int i = 0; i < root->GetChildCount(); i++) {
	//	FbxNode * child = root->GetChild(i);
	//	FbxMesh * mesh = child->GetMesh();
	//	FbxVector4* points = mesh->GetControlPoints();
	//	//#if _DEBUG
	//	//	cout << "\tC:" + to_string(i) << " ";
	//	//	if (child) {
	//	//		cout << "Children: " << to_string(child->GetChildCount());
	//	//		if (mesh) {
	//	//			cout << " Points: " << to_string(mesh->GetControlPointsCount()) << " ";
	//	//		
	//	//		}
	//	//		else {
	//	//			cout << "mesh not found" << endl;
	//	//		}
	//	//	}
	//	//	else {
	//	//		cout << " not found " << endl;
	//	//	}
	//	//	cout << endl;
	//	//#endif
	//	for (int v = 0; v < mesh->GetControlPointsCount(); v++) {
	//		//mesh->getNormal
	//		model->addVertex({ DirectX::XMFLOAT3(points[v].mData[0],points[v].mData[1],points[v].mData[2]),DirectX::XMFLOAT3(0.0f,0.0f,0.0f),DirectX::XMFLOAT2(0,0) });
	//	}
	/*	for (int m = 0; m < child->GetMaterialCount(); m++) {
		FbxSurfaceMaterial * mat = child->GetMaterial(m);
		mat->get

	}*/
	//}


	QueryPerformanceCounter(&end);
	float loadTime = (float)((end.QuadPart - start.QuadPart) * 1.0 / frequency.QuadPart);

	cout << "\t LOAD TIME OF " << fileName << ": " << to_string(loadTime) << "s" << " Size: " << to_string(model->getModelData().size()) << " Vertices" << endl;



	return model;
}

PotatoModel * PotatoFBXImporter::createDefaultPyramid(float width, float height) {
	/****************************************************************************************

	Copyright (C) 2015 Autodesk, Inc.
	All rights reserved.

	Use of this software is subject to the terms of the Autodesk license agreement
	provided at the time of installation or download, or which otherwise accompanies
	this software in either electronic or hard copy form.

	****************************************************************************************/

	FbxScene* lScene = FbxScene::Create(m_Manager, "myScene");
	FbxMesh * lPyramid = FbxMesh::Create(lScene, "pyramid");

	//// Calculate the vertices of the pyramid
	const double lBottomWidthHalf = width / 2;
	const FbxVector4 PyramidControlPointArray[] =
	{
		FbxVector4(0, height, 0),
		FbxVector4(lBottomWidthHalf, 0, lBottomWidthHalf),
		FbxVector4(lBottomWidthHalf, 0, -lBottomWidthHalf),
		FbxVector4(-lBottomWidthHalf, 0, -lBottomWidthHalf),
		FbxVector4(-lBottomWidthHalf, 0, lBottomWidthHalf)
	};

	// Initialize and set the control points of the mesh
	const int lControlPointCount = sizeof(PyramidControlPointArray) / sizeof(FbxVector4);
	lPyramid->InitControlPoints(lControlPointCount);
	for (int lIndex = 0; lIndex < lControlPointCount; ++lIndex)
	{
		lPyramid->SetControlPointAt(PyramidControlPointArray[lIndex], lIndex);
	}

	// Set the control point indices of the bottom side of the pyramid
	lPyramid->BeginPolygon();
	lPyramid->AddPolygon(1);
	lPyramid->AddPolygon(4);
	lPyramid->AddPolygon(3);
	lPyramid->AddPolygon(2);
	lPyramid->EndPolygon();

	// Set the control point indices of the front side of the pyramid
	lPyramid->BeginPolygon();
	lPyramid->AddPolygon(0);
	lPyramid->AddPolygon(1);
	lPyramid->AddPolygon(2);
	lPyramid->EndPolygon();

	// Set the control point indices of the left side of the pyramid
	lPyramid->BeginPolygon();
	lPyramid->AddPolygon(0);
	lPyramid->AddPolygon(2);
	lPyramid->AddPolygon(3);
	lPyramid->EndPolygon();

	// Set the control point indices of the back side of the pyramid
	lPyramid->BeginPolygon();
	lPyramid->AddPolygon(0);
	lPyramid->AddPolygon(3);
	lPyramid->AddPolygon(4);
	lPyramid->EndPolygon();

	// Set the control point indices of the right side of the pyramid
	lPyramid->BeginPolygon();
	lPyramid->AddPolygon(0);
	lPyramid->AddPolygon(4);
	lPyramid->AddPolygon(1);
	lPyramid->EndPolygon();

	// Attach the mesh to a node
	FbxNode * lPyramidNode = FbxNode::Create(lScene, "pyramid");
	lPyramidNode->SetNodeAttribute(lPyramid);

	//// Set this node as a child of the root node
	//pScene->GetRootNode()->AddChild(lPyramidNode);

	//return lPyramidNode;












	return nullptr;
}

PotatoModel * PotatoFBXImporter::createDefaultCube(float width, float height) {

	/****************************************************************************************

	Copyright (C) 2015 Autodesk, Inc.
	All rights reserved.

	Use of this software is subject to the terms of the Autodesk license agreement
	provided at the time of installation or download, or which otherwise accompanies
	this software in either electronic or hard copy form.

	****************************************************************************************/



	//// indices of the vertices per each polygon
	//static int vtxId[24] = {
	//	0,1,2,3, // front  face  (Z+)
	//	1,5,6,2, // right  side  (X+)
	//	5,4,7,6, // back   face  (Z-)
	//	4,0,3,7, // left   side  (X-)
	//	0,4,5,1, // bottom face  (Y-)
	//	3,2,6,7  // top    face  (Y+)
	//};

	//// control points
	//static Vector4 lControlPoints[8] = {
	//	{ -5.0,  0.0,  5.0, 1.0}, {  5.0,  0.0,  5.0, 1.0}, {  5.0,10.0,  5.0, 1.0},    { -5.0,10.0,  5.0, 1.0},
	//	{ -5.0,  0.0, -5.0, 1.0}, {  5.0,  0.0, -5.0, 1.0}, {  5.0,10.0, -5.0, 1.0},    { -5.0,10.0, -5.0, 1.0}
	//};

	//// normals
	//static Vector4 lNormals[8] = {
	//	{-0.577350258827209,-0.577350258827209, 0.577350258827209, 1.0},
	//	{ 0.577350258827209,-0.577350258827209, 0.577350258827209, 1.0},
	//	{ 0.577350258827209, 0.577350258827209, 0.577350258827209, 1.0},
	//	{-0.577350258827209, 0.577350258827209, 0.577350258827209, 1.0},
	//	{-0.577350258827209,-0.577350258827209,-0.577350258827209, 1.0},
	//	{ 0.577350258827209,-0.577350258827209,-0.577350258827209, 1.0},
	//	{ 0.577350258827209, 0.577350258827209,-0.577350258827209, 1.0},
	//	{-0.577350258827209, 0.577350258827209,-0.577350258827209, 1.0}
	//};

	//// uvs
	//static Vector2 lUVs[14] = {
	//	{ 0.0, 1.0},
	//	{ 1.0, 0.0},
	//	{ 0.0, 0.0},
	//	{ 1.0, 1.0}
	//};

	//// indices of the uvs per each polygon
	//static int uvsId[24] = {
	//	0,1,3,2,2,3,5,4,4,5,7,6,6,7,9,8,1,10,11,3,12,0,2,13
	//};

	//// create the main structure.
	//FbxMesh* lMesh = FbxMesh::Create(pScene, "");

	//// Create control points.
	//lMesh->InitControlPoints(8);
	//FbxVector4* vertex = lMesh->GetControlPoints();
	//memcpy((void*)vertex, (void*)lControlPoints, 8 * sizeof(FbxVector4));

	//// create the materials.
	///* Each polygon face will be assigned a unique material.
	//*/
	//FbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
	//lMaterialElement->SetMappingMode(FbxGeometryElement::eAllSame);
	//lMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	//lMaterialElement->GetIndexArray().Add(0);

	//// Create polygons later after FbxGeometryElementMaterial is created. Assign material indices.
	//int vId = 0;
	//for (int f = 0; f < 6; f++)
	//{
	//	lMesh->BeginPolygon();
	//	for (int v = 0; v < 4; v++)
	//		lMesh->AddPolygon(vtxId[vId++]);
	//	lMesh->EndPolygon();
	//}

	//// specify normals per control point.
	//FbxGeometryElementNormal* lNormalElement = lMesh->CreateElementNormal();
	//lNormalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
	//lNormalElement->SetReferenceMode(FbxGeometryElement::eDirect);

	//for (int n = 0; n < 8; n++)
	//	lNormalElement->GetDirectArray().Add(FbxVector4(lNormals[n][0], lNormals[n][1], lNormals[n][2]));


	//// Create the node containing the mesh
	//FbxNode* lNode = FbxNode::Create(pScene, pName);
	//lNode->LclTranslation.Set(pLclTranslation);

	//lNode->SetNodeAttribute(lMesh);
	//lNode->SetShadingMode(FbxNode::eTextureShading);

	//// create UVset
	//FbxGeometryElementUV* lUVElement1 = lMesh->CreateElementUV("UVSet1");
	//FBX_ASSERT(lUVElement1 != NULL);
	//lUVElement1->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	//lUVElement1->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
	//for (int i = 0; i < 4; i++)
	//	lUVElement1->GetDirectArray().Add(FbxVector2(lUVs[i][0], lUVs[i][1]));

	//for (int i = 0; i < 24; i++)
	//	lUVElement1->GetIndexArray().Add(uvsId[i % 4]);

	//return lNode;


	return nullptr;
}

bool PotatoFBXImporter::validFile(std::string fileName) {

	if (fileName.find(".fbx") != fileName.size() - 4) {
		#if _DEBUG
		cout << "Not an Fbx File: " << fileName << endl;
		#endif
		return false;
	}

	return true;
}
FbxScene* PotatoFBXImporter::makeScene(std::string fileName, std::string sceneName) {
	FbxImporter* importer = FbxImporter::Create(m_Manager, "");
	if (!importer->Initialize(fileName.c_str(), -1, m_Manager->GetIOSettings())) {
		importer->Destroy();
		#if _DEBUG
		std::cout << "Could not Load '" + fileName + "'" << std::endl;
		#endif
		return nullptr;
	}
	FbxScene* lScene = FbxScene::Create(m_Manager, sceneName.c_str());
	importer->Import(lScene);
	importer->Destroy();
	return lScene;
}
std::string PotatoFBXImporter::getObjName(std::string fileName) {
	if (fileName.find("/") != string::npos) {
		return fileName.substr(fileName.rfind("/") + 1, (fileName.size() - fileName.rfind("/")) - 5);

	}
	else if (fileName.find("\\") != string::npos) {
		return fileName.substr(fileName.rfind("\\") + 1, (fileName.size() - fileName.rfind("\\")) - 5);
	}
	else 
		return fileName.substr(0, fileName.size() - 4);
}

void PotatoFBXImporter::traverse(FbxNode*node) {



	int numAttributes = node->GetNodeAttributeCount();
	for (int j = 0; j < numAttributes; j++) {
		FbxNodeAttribute *nodeAttributeFbx = node->GetNodeAttributeByIndex(j);
		FbxNodeAttribute::EType attributeType = nodeAttributeFbx->GetAttributeType();

		switch (attributeType) {

		case FbxNodeAttribute::eMesh:
			fetchGeometry(node, "potato");


			break;
		case FbxNodeAttribute::eSkeleton:
			

			break;

		default:


			break;
		}
	}

	for (int i = 0; i < node->GetChildCount(); i++) {
		traverse(node->GetChild(i));
	}




}
























void PotatoFBXImporter::fetchGeometry(FbxNode* node, const std::string& filename) {

	// Number of polygon vertices 

	FbxMesh* mesh = node->GetMesh();
	int amount = mesh->GetPolygonVertexCount();
	int* indices = mesh->GetPolygonVertices();
	if (int(amount / 3) != mesh->GetPolygonCount()) {
		cout << "The mesh in '" << filename << "' has to be triangulated.";
		return;
	}


	int vertexIndex = 0;
	FbxVector4* cp = mesh->GetControlPoints();
	if (cp == nullptr) {
		cout << "Couldn't find any vertices in the mesh in the file " << filename;
		return;
	}
	//for (int polyIndex = 0; polyIndex < mesh->GetPolygonCount(); polyIndex++) {
	//	int numVertices = mesh->GetPolygonSize(polyIndex);

	//	for (int vertIndex = 0; vertIndex < numVertices; vertIndex += 3) {

	//		/*
	//		--	Positions
	//		*/
	//		m_buildData.positions[vertexIndex].x = -(float)cp[indices[vertexIndex]][0];
	//		m_buildData.positions[vertexIndex].y = (float)cp[indices[vertexIndex]][1];
	//		m_buildData.positions[vertexIndex].z = (float)cp[indices[vertexIndex]][2];

	//		m_buildData.positions[vertexIndex + 1].x = -(float)cp[indices[vertexIndex + 2]][0];
	//		m_buildData.positions[vertexIndex + 1].y = (float)cp[indices[vertexIndex + 2]][1];
	//		m_buildData.positions[vertexIndex + 1].z = (float)cp[indices[vertexIndex + 2]][2];

	//		m_buildData.positions[vertexIndex + 2].x = -(float)cp[indices[vertexIndex + 1]][0];
	//		m_buildData.positions[vertexIndex + 2].y = (float)cp[indices[vertexIndex + 1]][1];
	//		m_buildData.positions[vertexIndex + 2].z = (float)cp[indices[vertexIndex + 1]][2];












	{
	// Number of polygon vertices 
	//m_buildData.numVertices = mesh->GetPolygonVertexCount();
	//int* indices = mesh->GetPolygonVertices();

	//if (int(m_buildData.numVertices / 3) != mesh->GetPolygonCount()) {
	//	Logger::Error("The mesh in '" + filename + "' has to be triangulated.");
	//	return;
	//}

	//m_buildData.positions = new DirectX::SimpleMath::Vector3[m_buildData.numVertices];
	//m_buildData.normals = new DirectX::SimpleMath::Vector3[m_buildData.numVertices];
	//m_buildData.texCoords = new DirectX::SimpleMath::Vector2[m_buildData.numVertices];
	//m_buildData.tangents = new DirectX::SimpleMath::Vector3[m_buildData.numVertices];
	//m_buildData.bitangents = new DirectX::SimpleMath::Vector3[m_buildData.numVertices];

	//bool norms = true, uvs = true, tangs = true, bitangs = true;

	//int vertexIndex = 0;
	//FbxVector4* cp = mesh->GetControlPoints();
	//if (cp == nullptr) {
	//	Logger::Error("Couldn't find any vertices in the mesh in the file " + filename);
	//	return;
	//}
	//for (int polyIndex = 0; polyIndex < mesh->GetPolygonCount(); polyIndex++) {
	//	int numVertices = mesh->GetPolygonSize(polyIndex);

	//	for (int vertIndex = 0; vertIndex < numVertices; vertIndex += 3) {

	//		/*
	//		--	Positions
	//		*/
	//		m_buildData.positions[vertexIndex].x = -(float)cp[indices[vertexIndex]][0];
	//		m_buildData.positions[vertexIndex].y = (float)cp[indices[vertexIndex]][1];
	//		m_buildData.positions[vertexIndex].z = (float)cp[indices[vertexIndex]][2];

	//		m_buildData.positions[vertexIndex + 1].x = -(float)cp[indices[vertexIndex + 2]][0];
	//		m_buildData.positions[vertexIndex + 1].y = (float)cp[indices[vertexIndex + 2]][1];
	//		m_buildData.positions[vertexIndex + 1].z = (float)cp[indices[vertexIndex + 2]][2];

	//		m_buildData.positions[vertexIndex + 2].x = -(float)cp[indices[vertexIndex + 1]][0];
	//		m_buildData.positions[vertexIndex + 2].y = (float)cp[indices[vertexIndex + 1]][1];
	//		m_buildData.positions[vertexIndex + 2].z = (float)cp[indices[vertexIndex + 1]][2];


	//		/*
	//		--	Normals
	//		*/
	//		FbxLayerElementNormal* leNormal = mesh->GetLayer(0)->GetNormals();
	//		if (leNormal == nullptr && norms) {
	//			Logger::Warning("Couldn't find any normals in the mesh in the file " + filename);
	//			norms = false;
	//		}
	//		else if (norms) {
	//			if (leNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
	//				int normIndex = 0;

	//				if (leNormal->GetReferenceMode() == FbxLayerElement::eDirect)
	//					normIndex = vertexIndex;


	//				if (leNormal->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
	//					normIndex = leNormal->GetIndexArray().GetAt(vertexIndex);


	//				FbxVector4 norm = leNormal->GetDirectArray().GetAt(normIndex);
	//				m_buildData.normals[vertexIndex].x = -(float)norm[0];
	//				m_buildData.normals[vertexIndex].y = (float)norm[1];
	//				m_buildData.normals[vertexIndex].z = (float)norm[2];

	//				norm = leNormal->GetDirectArray().GetAt(normIndex + 2);
	//				m_buildData.normals[vertexIndex + 1].x = -(float)norm[0];
	//				m_buildData.normals[vertexIndex + 1].y = (float)norm[1];
	//				m_buildData.normals[vertexIndex + 1].z = (float)norm[2];

	//				norm = leNormal->GetDirectArray().GetAt(normIndex + 1);
	//				m_buildData.normals[vertexIndex + 2].x = -(float)norm[0];
	//				m_buildData.normals[vertexIndex + 2].y = (float)norm[1];
	//				m_buildData.normals[vertexIndex + 2].z = (float)norm[2];
	//			}
	//		}

	//		/*
	//		--	Tangents
	//		*/
	//		FbxGeometryElementTangent *geTang = mesh->GetElementTangent(0);
	//		if (geTang == nullptr && tangs) {
	//			Logger::Warning("Couldn't find any tangents in the mesh in the file " + filename);
	//			tangs = false;
	//		}
	//		else if (tangs) {
	//			if (geTang->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
	//				int tangIndex = 0;

	//				if (geTang->GetReferenceMode() == FbxLayerElement::eDirect)
	//					tangIndex = vertexIndex;


	//				if (geTang->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
	//					tangIndex = geTang->GetIndexArray().GetAt(vertexIndex);

	//				FbxVector4 tangent = geTang->GetDirectArray().GetAt(tangIndex);
	//				m_buildData.tangents[vertexIndex].x = (float)tangent[0];
	//				m_buildData.tangents[vertexIndex].y = (float)tangent[1];
	//				m_buildData.tangents[vertexIndex].z = (float)tangent[2];

	//				tangent = geTang->GetDirectArray().GetAt(tangIndex + 2);
	//				m_buildData.tangents[vertexIndex + 1].x = (float)tangent[0];
	//				m_buildData.tangents[vertexIndex + 1].y = (float)tangent[1];
	//				m_buildData.tangents[vertexIndex + 1].z = (float)tangent[2];

	//				tangent = geTang->GetDirectArray().GetAt(tangIndex + 1);
	//				m_buildData.tangents[vertexIndex + 2].x = (float)tangent[0];
	//				m_buildData.tangents[vertexIndex + 2].y = (float)tangent[1];
	//				m_buildData.tangents[vertexIndex + 2].z = (float)tangent[2];
	//			}
	//		}

	//		/*
	//		--	Binormals
	//		*/
	//		FbxGeometryElementBinormal *geBN = mesh->GetElementBinormal(0);
	//		if (geBN == nullptr && bitangs) {
	//			Logger::Warning("Couldn't find any binormals in the mesh in the file " + filename);
	//			bitangs = false;
	//		}
	//		else if (bitangs) {
	//			if (geBN->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
	//				int biNormIndex = 0;

	//				if (geBN->GetReferenceMode() == FbxLayerElement::eDirect)
	//					biNormIndex = vertexIndex;


	//				if (geBN->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
	//					biNormIndex = geBN->GetIndexArray().GetAt(vertexIndex);

	//				FbxVector4 biNorm = geBN->GetDirectArray().GetAt(biNormIndex);
	//				m_buildData.bitangents[vertexIndex].x = (float)biNorm[0];
	//				m_buildData.bitangents[vertexIndex].y = (float)biNorm[1];
	//				m_buildData.bitangents[vertexIndex].z = (float)biNorm[2];

	//				biNorm = geBN->GetDirectArray().GetAt(biNormIndex + 2);
	//				m_buildData.bitangents[vertexIndex + 1].x = (float)biNorm[0];
	//				m_buildData.bitangents[vertexIndex + 1].y = (float)biNorm[1];
	//				m_buildData.bitangents[vertexIndex + 1].z = (float)biNorm[2];

	//				biNorm = geBN->GetDirectArray().GetAt(biNormIndex + 1);
	//				m_buildData.bitangents[vertexIndex + 2].x = (float)biNorm[0];
	//				m_buildData.bitangents[vertexIndex + 2].y = (float)biNorm[1];
	//				m_buildData.bitangents[vertexIndex + 2].z = (float)biNorm[2];
	//			}
	//		}

	//		/*
	//		--	UV Coords
	//		*/
	//		FbxGeometryElementUV* geUV = mesh->GetElementUV(0);
	//		if (geUV == nullptr && uvs) {
	//			Logger::Warning("Couldn't find any texture coordinates in the mesh in the file " + filename);
	//			uvs = false;
	//		}
	//		else if (uvs) {
	//			FbxVector2 texCoord;
	//			int cpIndex;

	//			cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex);
	//			texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex);
	//			m_buildData.texCoords[vertexIndex].x = static_cast<float>(texCoord[0]);
	//			m_buildData.texCoords[vertexIndex].y = -static_cast<float>(texCoord[1]);

	//			cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 2);
	//			texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 2);
	//			m_buildData.texCoords[vertexIndex + 1].x = static_cast<float>(texCoord[0]);
	//			m_buildData.texCoords[vertexIndex + 1].y = -static_cast<float>(texCoord[1]);

	//			cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 1);
	//			texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 1);
	//			m_buildData.texCoords[vertexIndex + 2].x = static_cast<float>(texCoord[0]);
	//			m_buildData.texCoords[vertexIndex + 2].y = -static_cast<float>(texCoord[1]);
	//		}

	//		vertexIndex += 3;
	//	}

	//}
}
}