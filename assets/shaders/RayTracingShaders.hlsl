#define HLSL
#include "CommonRT.hlsl"

// Retrieve hit world position.
float3 HitWorldPosition() {
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Barycentric interpolation
float2 barrypolation(float3 barry, float2 in1, float2 in2, float2 in3) {
	return barry.x * in1 + barry.y * in2 + barry.z * in3;
}
float3 barrypolation(float3 barry, float3 in1, float3 in2, float3 in3) {
	return barry.x * in1 + barry.y * in2 + barry.z * in3;
}

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff = 16) {
	uint v0 = val0, v1 = val1, s0 = 0;

	[unroll]
	for (uint n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s) {
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 getPerpendicularVector(float3 u) {
	float3 a = abs(u);
	uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
	uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, float3(xm, ym, zm));
}

// Get a cosine-weighted random vector centered around a specified normal direction.
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm) {
	// Get 2 random numbers to select our sample with
	float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	float3 bitangent = getPerpendicularVector(hitNorm);
	float3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(randVal.x);
	float phi = 2.0f * 3.14159265f * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}



RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> lOutput : register(u0);

StructuredBuffer<Vertex> Vertices : register(t1, space0);

Texture2D<float4> diffuseTexture : register(t2, space0);
SamplerState ss : register(s0);

Texture2D<float4> skyboxTexture : register(t3, space0);

cbuffer CB_Global : register(b0, space0) {
	float RedChannel;
}

cbuffer CB_RayGen : register(b0, space1) {
	RayGenSettings RayGenSettings;
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

	// if (RayGenSettings.frameCount == 0)
	// 	lOutput[launchIndex] = float4(1.0, 0, 0, 1);
}

[shader("miss")]
void miss(inout RayPayload payload) {
	//payload.color = float4(0.4f, 0.6f, 0.3f, 1.0f);
	float2 dims;
	skyboxTexture.GetDimensions(dims.x, dims.y);
	float2 uv = wsVectorToLatLong(WorldRayDirection());
	payload.color = skyboxTexture[uint2(uv * dims)];

	if (payload.inShadow == -1) {
		payload.inShadow = 0;
	}
	// If ray is AO ray
	if (payload.aoVal == -1) {
		payload.aoVal = 1;
		return;
	}
}

[shader("closesthit")]
void closestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.recursionDepth++;

	//for info
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint instanceID = InstanceID();
	uint primitiveID = PrimitiveIndex();

	// If ray is shadow ray
	if (payload.inShadow == -1) {
		payload.inShadow = 1;
		return;
	}
	// If ray is AO ray
	if (payload.aoVal == -1) {
		payload.aoVal = 0;
		return;
	}


	uint verticesPerPrimitive = 3;
	Vertex vertex1 = Vertices[primitiveID * verticesPerPrimitive];
	Vertex vertex2 = Vertices[primitiveID * verticesPerPrimitive + 1];
	Vertex vertex3 = Vertices[primitiveID * verticesPerPrimitive + 2];

	float3 normalInLocalSpace = barrypolation(barycentrics, vertex1.normal, vertex2.normal, vertex3.normal);
	// float3 normalInWorldSpace = inverse(transpose(ObjectToWorld3x4())) * normalInLocalSpace;
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), normalInLocalSpace));

	if ( (instanceID == 1 || instanceID == 2)  && payload.recursionDepth < MAX_RAY_RECURSION_DEPTH - 1) { // -1 to allow for shadow ray
		RayDesc ray;
		ray.Origin = HitWorldPosition();
		ray.Direction = reflect(WorldRayDirection(), normalInWorldSpace);
		ray.TMin = 0.0001;
		ray.TMax = 100000;

		TraceRay(gRtScene, 0/*RAY_FLAG_CULL_BACK_FACING_TRIANGLES*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	} else {


		// Trace shadow ray
		payload.inShadow = -1;
		RayDesc ray;
		ray.Origin = HitWorldPosition();
		ray.Direction = -g_lightDirection;
		ray.TMin = 0.01;
		ray.TMax = 100000;
		TraceRay(gRtScene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray, payload);


		float2 texCoords = barrypolation(barycentrics, vertex1.texCoord, vertex2.texCoord, vertex3.texCoord);

		float diffuseFactor = max(dot(-g_lightDirection, normalInWorldSpace), 0.4f); // 0.4 ambient
		float3 r = reflect(g_lightDirection, normalInWorldSpace);
		r = normalize(r);
		float shininess = 5.0f;
		float spec = 1.0f;
		float specularFactor = pow(saturate(dot(-WorldRayDirection(), r)), shininess) * spec;

		float3 clr = diffuseTexture.SampleLevel(ss, texCoords, 0).rgb;
		float3 color = clr * diffuseFactor + clr * specularFactor; // Not in shadow
		if (payload.inShadow == 1) {
			color = clr * 0.5f; // In shadow
		}

		if (RayGenSettings.flags & RT_ENABLE_AO) {

			// Initialize a random seed, per-pixel and per-frame
			uint randSeed = initRand( DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, RayGenSettings.frameCount );

			float ao = 0.0;
			for (int i = 0; i < RayGenSettings.numAORays; i++) {

				float3 aoDir = getCosHemisphereSample(randSeed, normalInWorldSpace);
				// Trace Ambient Occlusion ray
				payload.aoVal = -1;
				RayDesc ray;
				ray.Origin = HitWorldPosition();
				ray.Direction = aoDir;
				ray.TMin = 0.01;
				ray.TMax = RayGenSettings.AORadius;
				TraceRay(gRtScene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray, payload);
				ao += payload.aoVal;
			}
			ao /= RayGenSettings.numAORays;
			color *= ao;

		}

		if (RayGenSettings.flags & RT_DRAW_NORMALS) {
			// Use normal as color
			payload.color = float4(normalInWorldSpace * 0.5f + 0.5f, 1.0f);
		} else {
			payload.color = float4(color, 1.0f);
		}
	}


}
