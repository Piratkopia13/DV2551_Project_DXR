#define HLSL
#include "CommonRT.hlsl"

// Retrieve hit world position.
float3 HitWorldPosition() {
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float2 barrypolation(float3 barry, float2 in1, float2 in2, float2 in3) {
	return barry.x * in1 + barry.y * in2 + barry.z * in3;
}
float3 barrypolation(float3 barry, float3 in1, float3 in2, float3 in3) {
	return barry.x * in1 + barry.y * in2 + barry.z * in3;
}

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> lOutput : register(u0);

StructuredBuffer<Vertex> Vertices : register(t1, space0);

Texture2D<float4> diffuseTexture : register(t2, space0);
SamplerState ss : register(s0);

cbuffer CB_Global : register(b0, space0) {
	float RedChannel;
}

cbuffer CB_ShaderTableLocal : register(b0, space1) {
	float3 ShaderTableColor;
}

ConstantBuffer<SceneConstantBuffer> CB_SceneData : register(b0, space2);

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction) {
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
void rayGen() {
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
void miss(inout RayPayload payload) {
	payload.color = float4(0.4f, 0.6f, 0.3f, 1.0f);
}

[shader("closesthit")]
void closestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.recursionDepth++;

	//for info
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint instanceID = InstanceID();
	uint primitiveID = PrimitiveIndex();

	uint verticesPerPrimitive = 3;
	Vertex vertex1 = Vertices[primitiveID * verticesPerPrimitive];
	Vertex vertex2 = Vertices[primitiveID * verticesPerPrimitive + 1];
	Vertex vertex3 = Vertices[primitiveID * verticesPerPrimitive + 2];

	float3 normalInLocalSpace = barrypolation(barycentrics, vertex1.normal, vertex2.normal, vertex3.normal);
	// float3 normalInWorldSpace = inverse(transpose(ObjectToWorld3x4())) * normalInLocalSpace;
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), normalInLocalSpace));

	if ( (instanceID == 1 || instanceID == 2)  && payload.recursionDepth < MAX_RAY_RECURSION_DEPTH) {
		RayDesc ray;
		ray.Origin = HitWorldPosition();
		ray.Direction = reflect(WorldRayDirection(), normalInWorldSpace);
		ray.TMin = 0.01;
		ray.TMax = 100000;

		TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	} else {
		float2 texCoords = barrypolation(barycentrics, vertex1.texCoord, vertex2.texCoord, vertex3.texCoord);

		float diffuseFactor = max(dot(-g_lightDirection, normalInWorldSpace), 0.4f); // 0.4 ambient
		float3 r = reflect(g_lightDirection, normalInWorldSpace);
		r = normalize(r);
		float shininess = 5.0f;
		float spec = 1.0f;
		float specularFactor = pow(saturate(dot(-WorldRayDirection(), r)), shininess) * spec;

		float3 clr = diffuseTexture.SampleLevel(ss, texCoords, 0).rgb;
		payload.color = float4(clr * diffuseFactor + clr * specularFactor, 1.0f);

		// payload.color = float4(normalInWorldSpace * 0.5f + 0.5f, 1.0f);
	}


}
