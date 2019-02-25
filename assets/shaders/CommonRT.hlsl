#define TEX_REG_DIFFUSE_SLOT 0
#define CB_REG_TRANSFORM 0
#define CB_REG_DIFFUSE_TINT 1
#define CB_REG_CAMERA 2

#ifdef HLSL

typedef float2 XMFLOAT2;
typedef float3 XMFLOAT3;
typedef float4 XMFLOAT4;
typedef float4x4 XMMATRIX;
typedef uint UINT;
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

struct CameraData {
	XMMATRIX VP;
	XMFLOAT3 cameraPosition;
};
