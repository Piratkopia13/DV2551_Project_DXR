#define HLSL
#include "CommonRT.hlsl"

struct VSOut {
	float4 position : SV_POSITION;
	float3 normal 	: NORMAL0;
	float2 texCoord : TEXCOORD0;
};

Texture2DArray tex : register(MERGE(t, TEX_REG_DIFFUSE_SLOT));
SamplerState ss : register(s0);
SamplerState static_ss : register(s1);

cbuffer CB_DiffuseTint : register(MERGE(b, CB_REG_DIFFUSE_TINT)) {
	float4 diffuseTint;
}

float4 PSMain(VSOut input) : SV_TARGET0 {
    float4 color = tex.Sample(static_ss, float3(input.texCoord, 0.0)) * tex.Sample(static_ss, float3(input.texCoord, 1.0));
    //color = float4(input.texCoord, 0.0, 0.0);
	return color * float4(diffuseTint.rgb, 1.0);
}