#ifdef HLSL
typedef float2 XMFLOAT2;
typedef float3 XMFLOAT3;
typedef float4 XMFLOAT4;
typedef float4x4 XMMATRIX;
typedef uint UINT;

#ifndef POSITION
	#define POSITION 0
	#define NORMAL 1
	#define TEXTCOORD 2
	#define INDEXBUFF 4

	#define TRANSFORM 5
	#define TRANSFORM_NAME TranslationBlock

	#define DIFFUSE_TINT 6
	#define DIFFUSE_TINT_NAME DiffuseColor

	#define DIFFUSE_SLOT 0
#endif

#define MERGE(a, b) a##b

#else
	#pragma once
	using namespace DirectX;
#endif


#define MAX_RAY_RECURSION_DEPTH 2

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texCoord;
};

struct RayPayload {
	XMFLOAT4 color;
	UINT recursionDepth;
};

struct SceneConstantBuffer {
	XMMATRIX projectionToWorld;
	XMFLOAT3 cameraPosition;
};