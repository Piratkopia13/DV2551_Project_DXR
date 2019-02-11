#define MERGE(a, b) a##b
struct VSIn {
	float4 position : POSITION0;
	float4 normal 	: NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct VSOut {
	float4 position : SV_POSITION;
	float4 normal 	: NORMAL0;
	float2 texCoord : TEXCOORD0;
};

cbuffer TRANSLATION_NAME : register(MERGE(b, TRANSLATION)) {
	float4 translate;
}

VSOut VSMain(VSIn input) {
	VSOut output = (VSOut)0;
	output.position = input.position + translate;

	#ifdef NORMAL
	 	output.normal = input.normal;
	#endif

	#ifdef TEXTCOORD
	 	output.texCoord = input.texCoord;
	 #endif

	return output;
}
