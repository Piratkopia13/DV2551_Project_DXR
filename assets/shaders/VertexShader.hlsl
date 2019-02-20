#define HLSL
#include "CommonRT.hlsl"

struct VSIn {
	float3 position : POSITION0;
	float3 normal 	: NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct VSOut {
	float4 position : SV_POSITION;
	float3 normal 	: NORMAL0;
	float2 texCoord : TEXCOORD0;
};

cbuffer CB_Transform : register(MERGE(b, CB_REG_TRANSFORM)) {
	matrix transform;
}

cbuffer CB_Camera : register(MERGE(b, CB_REG_CAMERA)) {
	CameraData camera;
}

VSOut VSMain(VSIn input) {
	VSOut output = (VSOut)0;
	output.position = mul(transform, float4(input.position, 1.0));
	output.position = mul(camera.VP, output.position);
 	output.normal = input.normal;
 	output.texCoord = input.texCoord;

	return output;
}
