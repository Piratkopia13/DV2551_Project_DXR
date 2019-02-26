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

// Shader only globals
static float3 g_lightDirection = float3(-0.513861f, -0.596225f, -0.616817f);
#define M_1_PI 0.318309886183790671538

// Shader only functions
float2 wsVectorToLatLong(float3 dir) {
  float3 p = normalize(dir);
  float u = (1.f + atan2(p.x, -p.z) * M_1_PI) * 0.5f;
  float v = acos(p.y) * M_1_PI;
  return float2(u, v);
}


#else

#pragma once
using namespace DirectX;

#endif


#define MAX_RAY_RECURSION_DEPTH 30

static const int RT_ENABLE_AO = 1 << 0;
static const int RT_DRAW_NORMALS = 1 << 1;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texCoord;
};

struct RayPayload {
	XMFLOAT4 color;
	UINT recursionDepth;
	int inShadow;
	int aoVal;
};

struct RayGenSettings {
	int flags;
	float AORadius;
	UINT frameCount;
	UINT numAORays;
};

struct SceneConstantBuffer {
	XMMATRIX projectionToWorld;
	XMFLOAT3 cameraPosition;
};

struct CameraData {
	XMMATRIX VP;
	XMFLOAT3 cameraPosition;
};
