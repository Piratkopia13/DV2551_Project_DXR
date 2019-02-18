#ifndef POSITION
	#define POSITION 0
	#define NORMAL 1
	#define TEXTCOORD 2
	#define INDEXBUFF 4

	#define TRANSLATION 5
	#define TRANSLATION_NAME TranslationBlock

	#define DIFFUSE_TINT 6
	#define DIFFUSE_TINT_NAME DiffuseColor

	#define DIFFUSE_SLOT 0
#endif

#define MERGE(a, b) a##b
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

cbuffer TRANSLATION_NAME : register(MERGE(b, TRANSLATION)) {
	float4 translate;
}

VSOut VSMain(VSIn input) {
	VSOut output = (VSOut)0;
	output.position = float4(input.position, 1.0) + translate;

	#ifdef NORMAL
	 	output.normal = input.normal;
	#endif

	#ifdef TEXTCOORD
	 	output.texCoord = input.texCoord;
	 #endif

	return output;
}
