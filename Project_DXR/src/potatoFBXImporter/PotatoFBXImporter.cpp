#include <pch.h>

#include "PotatoFBXImporter.h"
//#include <Windows.h>

using namespace std;
PotatoFBXImporter::PotatoFBXImporter() {
	m_Manager = FbxManager::Create();
	m_IOSettings = FbxIOSettings::Create(m_Manager, IOSROOT);
	

}

PotatoFBXImporter::~PotatoFBXImporter() {
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
	
	if (fileName.find(".fbx") != fileName.size() - 4) {
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
	for (int i = 0; i < root->GetChildCount(); i++) {
		FbxNode * child = root->GetChild(i);
		FbxMesh * mesh = child->GetMesh();
		FbxVector4* points = mesh->GetControlPoints();

#if _DEBUG
		cout << "\tC:" + to_string(i) << " ";
		if (child) {
			cout << "Children: " << to_string(child->GetChildCount());
			if (mesh) {
				cout << " Points: " << to_string(mesh->GetControlPointsCount()) << " ";
				
			}
			else {
				cout << "mesh not found" << endl;
			}
		}
		else {
			cout << " not found " << endl;
		}
		cout << endl;

#endif


		for (int v = 0; v < mesh->GetControlPointsCount(); v++) {
			//mesh->getNormal
			model->addVertex({ DirectX::XMFLOAT3(points[v].mData[0],points[v].mData[1],points[v].mData[2]),DirectX::XMFLOAT3(0.0f,0.0f,0.0f),DirectX::XMFLOAT2(0,0) });
		}

	/*	for (int m = 0; m < child->GetMaterialCount(); m++) {
			FbxSurfaceMaterial * mat = child->GetMaterial(m);
			mat->get

		}*/
		



	}









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

void PotatoFBXImporter::traverse() {


}

