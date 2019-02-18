#define MAX_RAY_RECURSION_DEPTH 2

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

struct Vertex
{
    float3 position;
    float3 normal;
};

struct SceneConstantBuffer {
	float4x4 projectionToWorld;
	float4 cameraPosition;
};

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

ConstantBuffer<SceneConstantBuffer> CB_SceneData : register(b0, space2);

struct RayPayload
{
	float4 color;
	uint recursionDepth;
};

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
	float2 xy = index + 0.5f; // center in the middle of the pixel.
	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates.
	screenPos.y = -screenPos.y;

	// Unproject the pixel coordinate into a ray.
	float4 world = mul(CB_SceneData.projectionToWorld, float4(screenPos, 0, 1));

	world.xyz /= world.w;
	origin = CB_SceneData.cameraPosition.xyz;
	direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void rayGen()
{
	float3 rayDir;
	float3 origin;

	uint2 launchIndex = DispatchRaysIndex().xy;
	// Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
	GenerateCameraRay(launchIndex, origin, rayDir);

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;

	// Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
	// TMin should be kept small to prevent missing geometry at close contact areas.
	ray.TMin = 0.001;
	ray.TMax = 10000.0;

	RayPayload payload;
	payload.recursionDepth = 0;
	TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	lOutput[launchIndex] = payload.color;
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
