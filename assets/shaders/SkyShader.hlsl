#define HLSL
#include "CommonRT.hlsl"

struct VSIn {
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC : TEXCOORD;
};

struct VSOut {
	float4 PosH : SV_POSITION;
	float3 PosL : POSITION;
};

//Texture2DArray gCubeMap : register(MERGE(t, TEX_REG_DIFFUSE_SLOT));
Texture2D gLatLongTex : register(MERGE(t, TEX_REG_DIFFUSE_SLOT));
SamplerState ss : register(s1);

cbuffer CB_Transform : register(MERGE(b, CB_REG_TRANSFORM)) {
	matrix transform;
}

cbuffer CB_Camera : register(MERGE(b, CB_REG_CAMERA)) {
	CameraData camera;
}

VSOut VSMain(VSIn vin) {
	VSOut vout;
	// Use local vertex position as cubemap lookup
	vout.PosL = vin.PosL;
	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), transform);
	// Always center sky about camera.
	posW.xyz += camera.cameraPosition;
	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
	vout.PosH = mul(camera.VP, posW).xyww;
	//vout.PosH = float4(vin.PosL, 1.0f);
	return vout;
}

float4 PSMain(VSOut pin) : SV_Target {
	return gLatLongTex.Sample(ss, wsVectorToLatLong(pin.PosL));
}