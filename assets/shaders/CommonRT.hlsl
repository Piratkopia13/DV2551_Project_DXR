#pragma once

#ifdef HLSL
typedef float3 XMFLOAT3;
typedef float4 XMFLOAT4;
typedef uint UINT;
#else
using namespace DirectX;
#endif


#define MAX_RAY_RECURSION_DEPTH 2

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
};

struct RayPayload {
	XMFLOAT4 color;
	UINT recursionDepth;
};