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
struct VSOut {
	float4 position : SV_POSITION;
	float3 normal 	: NORMAL0;
	float2 texCoord : TEXCOORD0;
};

#ifdef DIFFUSE_SLOT
Texture2D tex : register(MERGE(t, DIFFUSE_SLOT));
SamplerState ss : register(s0);
#endif

cbuffer DIFFUSE_TINT_NAME : register(MERGE(b, DIFFUSE_TINT)) {
	float4 diffuseTint;
}

float4 PSMain(VSOut input) : SV_TARGET0 {

#ifdef DIFFUSE_SLOT
    float4 color = tex.Sample(ss, input.texCoord);
#else
 	float4 color = float4(1.0, 1.0, 1.0, 1.0);
#endif

	return color * float4(diffuseTint.rgb, 1.0);
}