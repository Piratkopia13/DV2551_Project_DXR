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
	printAnimationStack(root);
	if (root) {
		fetchSkeleton(root, model, objName);
		int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
		model->reSizeAnimationStack(stackCount);
		fetchGeometry(root, model, objName);
		model->normalizeWeights();
		fetchTextures(root, model);
	}
	else {
		cout << "no root in " << objName << endl;
	}



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

	cout << "\t LOAD TIME OF " << fileName << ": " << to_string(loadTime) << "s" << " Size: " << to_string(model->getModelVertices().size()) << " Vertices" << to_string(model->getModelIndices().size()) << " Indices" << endl;



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

void PotatoFBXImporter::traverse(FbxNode*node, PotatoModel* model) {
	int numAttributes = node->GetNodeAttributeCount();
	for (int j = 0; j < numAttributes; j++) {
		

		FbxNodeAttribute *nodeAttribute = node->GetNodeAttributeByIndex(j);
		FbxNodeAttribute::EType attributeType = nodeAttribute->GetAttributeType();

		switch (attributeType) {
		case FbxNodeAttribute::eMesh:
			fetchGeometry(node, model, "potato");

			break;
		case FbxNodeAttribute::eSkeleton:
			//fetchSkeleton(node, "potato");
			break;

		default:
			break;
		}
	}

	for (int i = 0; i < node->GetChildCount(); i++) {
		traverse(node->GetChild(i), model);
	}
}


FbxVector2 PotatoFBXImporter::getTexCoord(int cpIndex, FbxGeometryElementUV* geUV, FbxMesh* mesh, int polyIndex, int vertIndex) const {
	FbxVector2 texCoord;

	switch (geUV->GetMappingMode()) {

		// UV per vertex
	case FbxGeometryElement::eByControlPoint:
		switch (geUV->GetReferenceMode()) {
			// Mapped directly per vertex
		case FbxGeometryElement::eDirect:
			texCoord = geUV->GetDirectArray().GetAt(cpIndex);
			break;
			// Mapped by indexed vertex
		case FbxGeometryElement::eIndexToDirect:
			texCoord = geUV->GetDirectArray().GetAt(geUV->GetIndexArray().GetAt(cpIndex));
			break;
		default:
			break;
		}
		break;

		// UV per indexed Vertex
	case FbxGeometryElement::eByPolygonVertex:
		switch (geUV->GetReferenceMode()) {
			// Mapped by indexed vertex
		case FbxGeometryElement::eIndexToDirect:
			texCoord = geUV->GetDirectArray().GetAt(mesh->GetTextureUVIndex(polyIndex, vertIndex));
			break;
		default:
			break;
		}
		break;
	}

	return texCoord;
}


void PotatoFBXImporter::fetchGeometry(FbxNode* node, PotatoModel* model, const std::string& filename) {

	// Number of polygon vertices 

	FbxScene* scene = node->GetScene();
	int numAttributes = node->GetNodeAttributeCount();
	for (int j = 0; j < numAttributes; j++) {

		FbxNodeAttribute *nodeAttribute = node->GetNodeAttributeByIndex(j);
		FbxNodeAttribute::EType attributeType = nodeAttribute->GetAttributeType();

		if (attributeType == FbxNodeAttribute::eMesh) {
			FbxMesh* mesh = node->GetMesh();
			FbxAMatrix geometryTransform(node->GetGeometricTranslation(FbxNode::eSourcePivot), node->GetGeometricRotation(FbxNode::eSourcePivot), node->GetGeometricScaling(FbxNode::eSourcePivot));

			unsigned int cpCount = mesh->GetControlPointsCount();
			FbxVector4* cps = mesh->GetControlPoints();
			model->reSizeControlPoints(cpCount);

			int polygonVertexCount= mesh->GetPolygonVertexCount();
			int* indices = mesh->GetPolygonVertices();
			if (int(polygonVertexCount / 3) != mesh->GetPolygonCount()) {
				cout << "The mesh in '" << filename << "' has to be triangulated.";
				return;
			}

			int vertexIndex = 0;
			if (cps == nullptr) {
				cout << "Couldn't find any vertices in the mesh in the file " << filename << endl;
				return;
			}
			for (int polyIndex = 0; polyIndex < mesh->GetPolygonCount(); polyIndex++) {
				int numVertices = mesh->GetPolygonSize(polyIndex);

				for (int vertIndex = 0; vertIndex < numVertices; vertIndex += 3) {

					/*NORMALS*/
					FbxLayerElementNormal* leNormal = mesh->GetLayer(0)->GetNormals();
					FbxVector4 norm[3] = { {0, 0, 0},{0, 0, 0},{0, 0, 0} };
					FbxVector2 texCoord[3] = { {0,0}, {0,0} };
					if (leNormal == nullptr) {
						cout << "Couldn't find any normals in the mesh in the file " << filename << endl;
					}
					else if (leNormal) {
						if (leNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
							int normIndex = 0;
							if (leNormal->GetReferenceMode() == FbxLayerElement::eDirect)
								normIndex = vertexIndex;
							if (leNormal->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
								normIndex = leNormal->GetIndexArray().GetAt(vertexIndex);
							norm[0] = leNormal->GetDirectArray().GetAt(normIndex);
							norm[1] = leNormal->GetDirectArray().GetAt(normIndex + 1);
							norm[2] = leNormal->GetDirectArray().GetAt(normIndex + 2);
						}
					}

					/*UV COORDS*/
					FbxGeometryElementUV* geUV = mesh->GetElementUV(0);
					if (geUV == nullptr) {
						cout << "Couldn't find any texture coordinates in the mesh in the file " << filename << endl;
					}
					else if (geUV) {
						int cpIndex;
						cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex);
						texCoord[0] = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex);
						cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 2);
						texCoord[1] = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 2);
						cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 1);
						texCoord[2] = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 1);
					}

					model->addVertex({
						DirectX::XMFLOAT3((float)cps[indices[vertexIndex]][0], (float)cps[indices[vertexIndex]][1],(float)cps[indices[vertexIndex]][2]),
						DirectX::XMFLOAT3((float)norm[0][0], (float)norm[0][1], (float)norm[0][2]),
						DirectX::XMFLOAT2(static_cast<float>(texCoord[0][0]),-static_cast<float>(texCoord[0][1]))
						}, polyIndex*3+vertIndex);
					model->addVertex({
						DirectX::XMFLOAT3((float)cps[indices[vertexIndex + 1]][0], (float)cps[indices[vertexIndex + 1]][1],(float)cps[indices[vertexIndex + 1]][2]),
						DirectX::XMFLOAT3((float)norm[1][0], (float)norm[1][1], (float)norm[1][2]),
						DirectX::XMFLOAT2(static_cast<float>(texCoord[2][0]),-static_cast<float>(texCoord[2][1]))
						}, polyIndex * 3 + vertIndex + 2);
					model->addVertex({
						DirectX::XMFLOAT3((float)cps[indices[vertexIndex + 2]][0], (float)cps[indices[vertexIndex + 2]][1],(float)cps[indices[vertexIndex + 2]][2]),
						DirectX::XMFLOAT3((float)norm[2][0], (float)norm[2][1], (float)norm[2][2]),
						DirectX::XMFLOAT2(static_cast<float>(texCoord[1][0]),-static_cast<float>(texCoord[1][1]))
						}, polyIndex * 3 + vertIndex + 1);

					vertexIndex += 3;
				}
			}

			/*CONTROLPOINTS*/
			for (unsigned int i = 0; i < cpCount; i++) {
				model->addControlPoint({ (float)cps[i][0], (float)cps[i][1], (float)cps[i][2] }, i);
			}


			/*BONE CONNECTIONS*/
			int largestIndex = -1;
			unsigned int deformerCount = mesh->GetDeformerCount();
			cout << "deformers: " << to_string(deformerCount) << endl;



			for (unsigned int deformerIndex = 0; deformerIndex < deformerCount; deformerIndex++) {
				FbxSkin* skin = reinterpret_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
				if (!skin) {
					cout << "not a skin at deformer " << to_string(deformerIndex) << endl;
					continue;
				}

				unsigned int clusterCount = skin->GetClusterCount();
				//cout << "  clusters: " << to_string(clusterCount) << endl;


				// CONNECTION AND GLOBALBINDPOSE FOR EACH CONNECTION FOR EACH LIMB

				std::vector<int> limbIndexes;
				limbIndexes.resize(clusterCount);
				for (unsigned int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
					FbxCluster* cluster = skin->GetCluster(clusterIndex);
					limbIndexes[clusterIndex] = model->findLimbIndex(cluster->GetLink()->GetUniqueID());
					if (limbIndexes[clusterIndex] == -1) {
						cout << "Could not find limb at clusterIndex: " << to_string(clusterIndex) << endl;
					}
				}


				for (unsigned int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex) {
					FbxCluster * cluster = skin->GetCluster(clusterIndex);
					FbxAMatrix transformMatrix;
					FbxAMatrix transformLinkMatrix;
					FbxAMatrix globalBindposeInverseMatrix;

					cluster->GetTransformMatrix(transformMatrix);	// The transformation of the mesh at binding time
					cluster->GetTransformLinkMatrix(transformLinkMatrix);	// The transformation of the cluster(joint) at binding time from joint space to world space
					globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

					// Update the information in mSkeleton 
					model->setGlobalBindposeInverse(limbIndexes[clusterIndex], convertToXMMatrix(globalBindposeInverseMatrix));


					unsigned int indexCount = cluster->GetControlPointIndicesCount();
					int* CPIndices = cluster->GetControlPointIndices();
					double* CPWeights = cluster->GetControlPointWeights();

					for (unsigned int index = 0; index < indexCount; ++index) {
						if (CPIndices[index] > largestIndex)
							largestIndex = CPIndices[index];

						model->addConnection(CPIndices[index], limbIndexes[clusterIndex], (float)CPWeights[index]);

					}
				}


				// ANIMATION STACK FOR EACH LIMB
				int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
#ifdef PERFORMANCE_TESTING
				stackCount = 1;
#endif
				for (unsigned int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex) {

					FbxCluster * cluster = skin->GetCluster(clusterIndex);
					//cout << filename << " StackSize: " << to_string(stackCount) << endl;
					for (int currentStack = 0; currentStack < stackCount; currentStack++) {
						FbxAnimStack* currAnimStack = scene->GetSrcObject<FbxAnimStack>(currentStack);

						FbxTakeInfo* takeInfo = scene->GetTakeInfo(currAnimStack->GetName());
						node->GetScene()->SetCurrentAnimationStack(currAnimStack);
						//FbxAnimEvaluator* eval = node->GetAnimationEvaluator();
						FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
						FbxTime end = takeInfo->mLocalTimeSpan.GetStop();
						//cout << "Animation Time: " << to_string((float)takeInfo->mLocalTimeSpan.GetDuration().GetSecondDouble()) << " Frame Count: " << to_string((int)end.GetFrameCount(FbxTime::eFrames24)) << endl;
						float firstFrameTime = 0.0f;
						for (FbxLongLong frame = start.GetFrameCount(FbxTime::eFrames24); frame <= end.GetFrameCount(FbxTime::eFrames24); frame++) {
							FbxTime currTime;
							currTime.SetFrame(frame, FbxTime::eFrames24);
							if (firstFrameTime == 0.0f)
								firstFrameTime = float(currTime.GetSecondDouble());
							//eval->GetNodeGlobalTransform(node, currTime);
							//eval->GetNodeGlobalTransform(node, currTime);
							FbxAMatrix currentTransformOffset = node->EvaluateGlobalTransform(currTime) * geometryTransform;
							model->addFrame(UINT(currentStack), limbIndexes[clusterIndex], float(currTime.GetSecondDouble()) - firstFrameTime,
								convertToXMMatrix(currentTransformOffset.Inverse() * cluster->GetLink()->EvaluateGlobalTransform(currTime)));
						}

					}
				}
			}
		}

		if(true){
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
	for (int childIndex = 0; childIndex < node->GetChildCount(); ++childIndex) {
		fetchGeometry(node->GetChild(childIndex), model, filename);
	}
}
void PotatoFBXImporter::fetchSkeleton(FbxNode * node, PotatoModel* model, const std::string & filename) {


	for (int childIndex = 0; childIndex < node->GetChildCount(); ++childIndex)
	{
		FbxNode* currNode = node->GetChild(childIndex);
		fetchSkeletonRecursive(currNode, model, 0, 0, -1);
	}

	//PotatoModel::Limb* limb = new PotatoModel::Limb();
	//if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
	//	limb->parent = parent;
	//	model->addBone(limb);
	//}
	//for (int i = 0; i < node->GetChildCount(); i++)
	//{
	//	fetchSkeleton(node->GetChild(i), model, limb, filename);
	//}

}

void PotatoFBXImporter::fetchSkeletonRecursive(FbxNode* inNode, PotatoModel* model, int inDepth, int myIndex, int inParentIndex) {

	if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		PotatoModel::Limb limb;
		limb.parentIndex = inParentIndex;
		limb.uniqueID = inNode->GetUniqueID();
		model->addBone(limb);

	}
	for (int i = 0; i < inNode->GetChildCount(); i++) {
		fetchSkeletonRecursive(inNode->GetChild(i), model, inDepth + 1, UINT(model->getSkeleton().size()), myIndex);
	}


}

void PotatoFBXImporter::fetchTextures(FbxNode * node, PotatoModel * model) {

	int materialIndex;
	FbxProperty fbxProperty;
	int materialCount = node->GetSrcObjectCount<FbxSurfaceMaterial>();
	// Loops through all materials to find the textures of the model
	for (materialIndex = 0; materialIndex < materialCount; materialIndex++) {
		FbxSurfaceMaterial* material = node->GetSrcObject<FbxSurfaceMaterial>(materialIndex);
		if (material) {
			int textureIndex;
			FBXSDK_FOR_EACH_TEXTURE(textureIndex) {
				fbxProperty = material->FindProperty(FbxLayerElement::sTextureChannelNames[textureIndex]);
				if (fbxProperty.IsValid()) {
					int textureCount = fbxProperty.GetSrcObjectCount<FbxTexture>();
					for (int i = 0; i < textureCount; ++i) {
						FbxLayeredTexture* layeredTexture = fbxProperty.GetSrcObject<FbxLayeredTexture>(i);
						// Checks if there's multiple layers to the texture
						if (layeredTexture) {
							int numTextures = layeredTexture->GetSrcObjectCount<FbxTexture>();

							// Loops through all the textures of the material (Engine currently only supports one texture per spec/diff/norm.)
							for (int j = 0; j < numTextures; j++) {

								FbxTexture* texture = layeredTexture->GetSrcObject<FbxTexture>(j);

								FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);

								std::string filename = fileTexture->GetRelativeFileName();
								std::cout << filename << std::endl;
								if (filename.find("specular")) {
									model->addSpecularTexture(filename);
								}
								if (filename.find("diffuse")) {
									model->addDiffuseTexture(filename);
								}
								if (filename.find("normal")) {
									model->addNormalTexture(filename);
								}
							}
						}
					}
				}
			}
		}
	}


	for (int i = 0; i < node->GetChildCount(); i++) {
		fetchTextures(node->GetChild(i), model);
	}
}


void PotatoFBXImporter::printAnimationStack(FbxNode* node) {

	int stackCount = node->GetScene()->GetSrcObjectCount<FbxAnimStack>();
	cout << " StackSize: " << to_string(stackCount) << endl;
	for (int currentStack = 0; currentStack < stackCount; currentStack++) {
		FbxAnimStack* currAnimStack = node->GetScene()->GetSrcObject<FbxAnimStack>(currentStack);
		std::cout << currAnimStack->GetName() << std::endl;
	}
}