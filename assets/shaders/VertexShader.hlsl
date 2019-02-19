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

cbuffer TRANSFORM_NAME : register(MERGE(b, TRANSFORM)) {
	matrix transform;
}

VSOut VSMain(VSIn input) {
	VSOut output = (VSOut)0;
	output.position = mul(transform, float4(input.position, 1.0));

	#ifdef NORMAL
	 	output.normal = input.normal;
	#endif

	#ifdef TEXTCOORD
	 	output.texCoord = input.texCoord;
	 #endif

	return output;
}
