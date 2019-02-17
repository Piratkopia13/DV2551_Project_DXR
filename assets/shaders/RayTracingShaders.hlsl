#define HLSL
#include "CommonRT.hlsl"

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> lOutput : register(u0);

StructuredBuffer<Vertex> Vertices : register(t1, space0);

cbuffer CB_Global : register(b0, space0)
{
	float RedChannel;
}

cbuffer CB_ShaderTableLocal : register(b0, space1)
{
	float3 ShaderTableColor;
}

[shader("raygeneration")]
void rayGen()
{
	uint3 launchIndex = DispatchRaysIndex();
	uint3 launchDim = DispatchRaysDimensions();

	float2 crd = float2(launchIndex.xy);
	float2 dims = float2(launchDim.xy);

	float2 d = ((crd / dims) * 2.f - 1.f);
	float aspectRatio = dims.x / dims.y;

	RayDesc ray;
	ray.Origin = float3(0, 0, -2);
	ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));

	ray.TMin = 0;
	ray.TMax = 100000;

	RayPayload payload;
	payload.recursionDepth = 0;
	TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	lOutput[launchIndex.xy] = payload.color;
	// lOutput[launchIndex.xy] = float4(RedChannel, 0, 0, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
	payload.color = float4(0.4f, 0.6f, 0.3f, 1.0f);


	// RayDesc ray;
	// ray.Origin = float3(0, 0, -2);
	// ray.Direction = float3(-1,0,0);

	// ray.TMin = 0;
	// ray.TMax = 100000;

	// payload.recursionDepth = 0;
	// TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);

}

[shader("closesthit")]
void closestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.recursionDepth++;

	//for info
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint instanceID = InstanceID();
	uint primitiveID = PrimitiveIndex();

	uint verticesPerPrimitive = 3;
	float3 normalInLocalSpace = Vertices[primitiveID * verticesPerPrimitive].normal;
	// float3 normalInWorldSpace = inverse(transpose(ObjectToWorld3x4())) * normalInLocalSpace;
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), normalInLocalSpace));

	if (instanceID == 1 && payload.recursionDepth < MAX_RAY_RECURSION_DEPTH) {
		RayDesc ray;
		ray.Origin = HitWorldPosition();
		ray.Direction = normalInWorldSpace;
		ray.TMin = 0.01;
		ray.TMax = 100000;

		TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	} else {

		payload.color.rgb = normalInWorldSpace / 2.0 + 0.5;
		// payload.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
		payload.color.rgb = float3(RedChannel, 0, 0) + ShaderTableColor;
		payload.color.a = 1.0f;
	}


}
