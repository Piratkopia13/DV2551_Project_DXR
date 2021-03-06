#define HLSL
#include "CommonRT.hlsl"

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> lOutput : register(u0);

StructuredBuffer<Vertex> Vertices : register(t1, space0);
StructuredBuffer<uint> Indices : register(t1, space1);

Texture2DArray<float4> textureArray : register(t2, space0);
SamplerState ss : register(s0);

Texture2D<float4> skyboxTexture : register(t3, space0);

cbuffer CB_Global : register(b0, space0) {
	float RedChannel;
}

cbuffer CB_RayGen : register(b0, space1) {
	RayGenSettings RayGenSettings;
}

cbuffer CB_Material : register(b1, space0) {
	MaterialProperties material;
}

ConstantBuffer<SceneConstantBuffer> CB_SceneData : register(b0, space2);


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

void scatter(float3 direction, inout RayPayload payload, int flags = 0) {
	RayDesc ray;
	ray.Origin = HitWorldPosition();
	ray.Direction = direction;
	ray.TMin = 0.0001;
	ray.TMax = 100000;

	TraceRay(gRtScene, flags | RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
}

bool traceHitRay() {
	// Trace hit ray
	RayPayload payload;
	payload.hit = 1;
	scatter(-g_lightDirection, payload, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER);

	return payload.hit == 1;
}

float traceAmbientOcclusionRays(inout uint seed, float3 normal) {
	float ao = 0.0;
	for (int i = 0; i < RayGenSettings.numAORays; i++) {
		float3 aoDir = getCosHemisphereSample(seed, normal);
		// Trace Ambient Occlusion ray
		RayPayload payload;
		payload.hit = 1;
		RayDesc ray;
		ray.Origin = HitWorldPosition();
		ray.Direction = aoDir;
		ray.TMin = 0.0001;
		ray.TMax = RayGenSettings.AORadius;
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 0, 0, ray, payload);
		ao += 1 - payload.hit;
	}
	ao /= RayGenSettings.numAORays;
	return ao;
}

float3 globalIllumination(inout uint seed, inout RayPayload payload, float3 normal) {
	if (payload.hit != 10 + RayGenSettings.GIBounces) { // Check if recursive GI shoud be done
		if (payload.hit < 10)
			payload.hit = 10;
		else
			payload.hit++;

		float3 giColor = float3(0,0,0);
		for (uint i = 0; i < RayGenSettings.GISamples; i++) {
			float3 dir = getCosHemisphereSample(seed, normal);
			scatter(dir, payload, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH);

			giColor += payload.color.rgb * 1.0; // 100% of the light from global sources is applied
		}
		return giColor / RayGenSettings.GISamples;
	}
	return float3(1.0, 1.0, 1.0);
}

float3 phongShading(float3 diffuseColor, float3 normal, float2 texCoords) {
	float shininess = 5.0f;
	float spec = 1.0f;
	float ambient = 0.4f;

	float diffuseFactor = max(dot(-g_lightDirection, normal), ambient) * 3.0;
	float3 r = reflect(g_lightDirection, normal);
	r = normalize(r);
	float specularFactor = pow(saturate(dot(-WorldRayDirection(), r)), shininess) * spec;

	float3 color = diffuseColor * diffuseFactor + diffuseColor * specularFactor;
	return color;
}

// Approximation of reflectance and transmittance
// Provided by: https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf
// Ray, normal, refractive indices n1 (from), n2 (to)
// Refractive indices - air ~1, water 20*C ~1.33, glass ~1.5
// Reflectance = rSchlick2, Transmittance = 1 - Reflectance
float rSchlick2(in float3 incident, in float3 normal, in float n1, in float n2) {
	float mat1 = n1, mat2 = n2;
	float cosI = -dot(incident, normal);
	// Leaving the material
	//if(cosI < 0.0) {
	//	mat1 = n2;
	//	mat2 = n1;
	//}
	// Due to restriction in approximation
	if (mat1 > mat2) {
		float n = mat1 / mat2;
		float sinT2 = pow(n, 2) * (1.0 - pow(abs(cosI), 2));
		if (sinT2 > 1.0f) return 1.0f; // TIR
		cosI = sqrt(1.0f - sinT2);
	}
	float x = 1.0f - cosI;
	float r0 = pow(abs(mat1 - mat2) / (mat1 + mat2), 2);
	return r0 + (1.0f - r0) * pow(x, 5);
}

// Ray, normal, refractive indices n1 (from), n2 (to)
float3 GetTransmittanceRay(in float3 incident, in float3 normal, float n1, float n2) {
	float n = n1/n2;
	float cosI = -dot(incident, normal);
	float sinT2 = pow(n, 2) * (1.0f - pow(abs(cosI), 2));
	if(sinT2 > 1.0) return float3(0.0, 0.0, 0.0);
	float cosT = sqrt(1.0 - sinT2);
	return n * incident + (n * cosI - cosT) * normal;
}




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
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;

	RayPayload payload;
	payload.recursionDepth = 0;
	payload.hit = 0;
	payload.color = float4(0,0,0,0);
	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	lOutput[launchIndex] = payload.color;

	// if (RayGenSettings.frameCount == 0)
	// 	lOutput[launchIndex] = float4(1.0, 0, 0, 1);
}

[shader("miss")]
void miss(inout RayPayload payload) {
	if (payload.hit == 1) {
		payload.hit = 0;
		return;
	}
	//payload.color = float4(0.4f, 0.6f, 0.3f, 1.0f);
	float2 dims;
	skyboxTexture.GetDimensions(dims.x, dims.y);
	float2 uv = wsVectorToLatLong(WorldRayDirection());
	payload.color = skyboxTexture[uint2(uv * dims)];

}

float3 toneMap(float3 color) {
	float gamma = 0.6;

	float A = 0.15;
	float B = 0.25;
	float C = 0.10;
	float D = 0.10;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = 4.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(color, float3(1.0 / gamma, 1.0 / gamma, 1.0 / gamma));
	return color;
}

[shader("closesthit")]
void closestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.recursionDepth++;

	//for info
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint instanceID = InstanceID();
	uint primitiveID = PrimitiveIndex();

	// Initialize a random seed, per-pixel and per-frame
	uint randSeed = initRand( DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, RayGenSettings.frameCount );
	// uint randSeed = initRand( DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, 1 );

	uint verticesPerPrimitive = 3;
	Vertex vertex1 = Vertices[Indices[primitiveID * verticesPerPrimitive]];
	Vertex vertex2 = Vertices[Indices[primitiveID * verticesPerPrimitive + 1]];
	Vertex vertex3 = Vertices[Indices[primitiveID * verticesPerPrimitive + 2]];

	float3 normalInLocalSpace = barrypolation(barycentrics, vertex1.normal, vertex2.normal, vertex3.normal);
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), normalInLocalSpace));
	float2 texCoords = barrypolation(barycentrics, vertex1.texCoord, vertex2.texCoord, vertex3.texCoord);

	if (RayGenSettings.flags & RT_DRAW_NORMALS) {
		// Use normal as color
		payload.color = float4(normalInWorldSpace * 0.5f + 0.5f, 1.0f);
		return;
	}

	float3 diffuseColor = textureArray.SampleLevel(ss, float3(texCoords, DIFFUSE_TEXTURE_INDEX), 0).rgb * material.albedoColor;
	// float3 diffuseColor = float3(0.f, 0.8, 0.0);
	float3 giColor = float3(1,1,1);

	// Global illumination
	if (RayGenSettings.flags & RT_ENABLE_GI) {
		giColor = globalIllumination(randSeed, payload, normalInWorldSpace);
	}

	bool towardsLight = dot(-g_lightDirection, normalInWorldSpace) > 0.0;
	bool inShadow = true;
	// Trace shadow ray if normal is within 90 degrees of the light
	if (towardsLight) {
		inShadow = traceHitRay();
	}
	float3 color = diffuseColor * 0.4f; // In shadow
	if (!inShadow) {
		color = phongShading(diffuseColor, normalInWorldSpace, texCoords);
	}
	// Apply global illumination
	color *= giColor;

	// Ambient occlusion
	if (RayGenSettings.flags & RT_ENABLE_AO) {
		float ao = traceAmbientOcclusionRays(randSeed, normalInWorldSpace);
		color *= ao;
	}

	if (RayGenSettings.flags & RT_ENABLE_GI) {
		// Gamma correction
		// color = sqrt(color);
		float gamma = 2.0;
		color = pow(color, 1.0 / gamma);
	}

	float3 scatteredColor = color;
	float reflectionAttenuation = material.reflectionAttenuation;
	if (payload.recursionDepth < min(material.maxRecursionDepth+3, MAX_RAY_RECURSION_DEPTH) - 2) {
		// Check if multiple materials
		UINT width;
		UINT height;
		UINT numElements;
		// TODO: Check if this works
		textureArray.GetDimensions(width, height, numElements);

		float refractionFactor = 0;
		if (numElements > 1) {
			float2 matRGB = textureArray.SampleLevel(ss, float3(texCoords, MATERIAL_TEXTURE_INDEX), 0).rg;
			reflectionAttenuation = 1.0 - matRGB.r;
			refractionFactor = matRGB.g;
		}

		if (refractionFactor > 0.01) {
			// Material refraction indices
			float n1 = 1.0; // Air
			float n2 = 1.0 + refractionFactor;
			float reflectanceFactor = saturate(rSchlick2(WorldRayDirection(), normalInWorldSpace, n1, n2));

			// Refraction
			float3 dir = GetTransmittanceRay(WorldRayDirection(), normalInWorldSpace, n1, n2);
			scatter(dir, payload);
			scatteredColor = payload.color * (1.0 - reflectanceFactor);

			// Reflection
			dir = reflect(WorldRayDirection(), normalInWorldSpace);
			scatter(dir, payload);
			scatteredColor += payload.color * reflectanceFactor;

			reflectionAttenuation = 0.0;
		} 
		else if (reflectionAttenuation < 1.0) { // -1 to allow for shadow ray
			// Shoot a recursive reflected ray
			float3 dir = reflect(WorldRayDirection(), normalInWorldSpace);
			dir += (float3(nextRand(randSeed), nextRand(randSeed), nextRand(randSeed)) * 2.0 - 1.0) * material.fuzziness;
			scatter(dir, payload);
			scatteredColor = payload.color * (1.0 - reflectionAttenuation);
		}
	}

 	payload.color.rgb = reflectionAttenuation * color + scatteredColor;
 	payload.color.rgb = toneMap(payload.color.rgb);
 	payload.color.a = 1.0f;

}
