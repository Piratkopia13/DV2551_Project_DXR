RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);

cbuffer CB_Global : register(b0, space0)
{
	float RedChannel;
}

cbuffer CB_ShaderTableLocal : register(b0, space1)
{
	float3 ShaderTableColor;
}

struct RayPayload
{
	float4 color;
};

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
	TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	gOutput[launchIndex.xy] = payload.color;
}

[shader("miss")]
void miss(inout RayPayload payload)
{
	payload.color = float4(1.4f, 0.6f, 0.3f, 0.0f);
}

[shader("closesthit")]
void closestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	//for info
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint instanceID = InstanceID();
	uint primitiveID = PrimitiveIndex();

	payload.color.rgb = float3(RedChannel, 0, 0) + ShaderTableColor;
	payload.color.a = 1.0f;
}
