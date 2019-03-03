Texture2D<float4> currFrame : register(t0);
Texture2D<float4> lastFrame : register(t1);
SamplerState ss : register(s2);

cbuffer PerFrameCB : register(b0) {
    uint gAccumCount;
}

struct VSIn {
	float3 position : POSITION0;
};

struct VSOut {
	float4 pos : SV_Position;
	float2 texC : TEXCOORD;
};

VSOut VSMain(VSIn input) {
	VSOut output = (VSOut)0;
	
	output.pos = float4(input.position, 1.0);
	output.texC = input.position * 0.5 + 0.5;
	output.texC.y = 1.0 - output.texC.y;

	return output;
}

float4 PSMain(VSOut input) : SV_Target0 {
	// return float4(0.8, 0.2, 0.2, 1.0);
    float4 curColor  = currFrame.Sample(ss, input.texC);
	float4 prevColor = lastFrame.Sample(ss, input.texC);
	// return (prevColor + curColor) * 0.5;
	return (gAccumCount * prevColor + curColor) / (gAccumCount + 1.0);
}
