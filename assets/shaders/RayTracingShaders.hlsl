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


float3 CalculatePhong(in float3 albedo, in float3 normal, in float diffuseCoef = 1.0f, in float specularCoef = 1.0f, in float specularPower = 1.0f) {
	float diffuseFactor = max(dot(-g_lightDirection, normal), diffuseCoef);
	float3 r = reflect(g_lightDirection, normal);
	r = normalize(r);
	float specularFactor = pow(saturate(dot(-WorldRayDirection(), r)), specularPower) * specularCoef;

	return albedo * diffuseFactor + albedo * specularFactor;
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

float3 GetReflectionColor(in float3 incident, in float3 normal, in float3 albedo) {
	float cosI = saturate(-dot(incident, normal));
	return albedo + (1.0 - albedo) * pow(1.0 - cosI, 5);
}



RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> lOutput : register(u0);

StructuredBuffer<Vertex> Vertices : register(t1, space0);

Texture2DArray<float4> diffuseTexture : register(t2, space0);
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
	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
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

	//if ( (instanceID == 1 || instanceID == 2)  && payload.recursionDepth < MAX_RAY_RECURSION_DEPTH - 1) { // -1 to allow for shadow ray
	// 	RayDesc ray;
	// 	ray.Origin = HitWorldPosition();
	// 	ray.Direction = reflect(WorldRayDirection(), normalInWorldSpace);
	// 	ray.TMin = 0.0001;
	// 	ray.TMax = 100000;

	// 	TraceRay(gRtScene, 0/*RAY_FLAG_CULL_BACK_FACING_TRIANGLES*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);
	// }




	// Trace shadow ray
	payload.inShadow = -1;
	RayDesc ray;
	ray.Origin = HitWorldPosition();
	ray.Direction = -g_lightDirection;
	ray.TMin = 0.01;
	ray.TMax = 100000;
	TraceRay(gRtScene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray, payload);



	float2 texCoords = barrypolation(barycentrics, vertex1.texCoord, vertex2.texCoord, vertex3.texCoord);

	// Sample diffuse texture
	float3 diffuseColor = diffuseTexture.SampleLevel(ss, float3(texCoords, 0.0), 0).rgb;

	float3 phongColor = CalculatePhong(diffuseColor, normalInWorldSpace, 0.2f, 1.0, 100.0f);
	float3 color = float3(0.0, 0.0, 0.0);

	// Reflection and refraction values
	float2 matValues = diffuseTexture.SampleLevel(ss, float3(texCoords, 1.0), 0).rg;
	float3 reflectedColor = float3(0.0, 0.0, 0.0);
	float3 refractionColor = float3(0.0, 0.0, 0.0);

	// Reflection / Refraction
	if(payload.recursionDepth < MAX_RAY_RECURSION_DEPTH - 1) {

		// Reflection
		if (matValues.r > 0.01) {
			RayDesc reflectionRay;
			reflectionRay.Origin = HitWorldPosition();
			reflectionRay.Direction = reflect(WorldRayDirection(), normalInWorldSpace);
			reflectionRay.TMin = 0.0001;
			reflectionRay.TMax = 100000;
			TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, reflectionRay, payload);


			reflectedColor = /*GetReflectionColor(WorldRayDirection(), normalInWorldSpace, diffuseColor)*/ matValues.r * payload.color;
			//reflectedColor = float3(1.0, 1.0, 1.0);
			//reflectedColor = GetReflectionColor(WorldRayDirection(), normalInWorldSpace, diffuseColor) * (1.0 - rSchlick2(WorldRayDirection(), normalInWorldSpace, 1.0, matValues.g + 1.0)) * payload.color;
			//reflectedColor = rShlick2(WorldRayDirection(), normalInWorldSpace, 1.0, matValues.g + 1.0) * payload.color;
			//reflectedColor = payload.color;
		}

		// Refraction
		if (matValues.g > 0.01) {
			// Material refraction indices
			float n1 = 1.0; /* Air */
			float n2 = 1.0 + matValues.g;
			n2 = 1.5;
			float reflectanceFactor = saturate(rSchlick2(WorldRayDirection(), normalInWorldSpace, n1, n2));


			RayDesc refractionRay;
			refractionRay.Origin = HitWorldPosition();
			refractionRay.Direction = GetTransmittanceRay(WorldRayDirection(), normalInWorldSpace, n1, n2);
			refractionRay.TMin = 0.0001;
			refractionRay.TMax = 100000;
			RayPayload refracRayPayload;
			refracRayPayload.color = float4(0.0, 0.0, 0.0, 1.0);;
			refracRayPayload.recursionDepth = payload.recursionDepth;
			TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, refractionRay, refracRayPayload);

			refractionColor = refracRayPayload.color * (1.0 - reflectanceFactor);
			


			RayDesc reflectionRay;
			reflectionRay.Origin = HitWorldPosition();
			reflectionRay.Direction = reflect(WorldRayDirection(), normalInWorldSpace);
			reflectionRay.TMin = 0.0001;
			reflectionRay.TMax = 100000;
			TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, reflectionRay, payload);

			reflectedColor = /*GetReflectionColor(WorldRayDirection(), normalInWorldSpace, diffuseColor)*/ payload.color * reflectanceFactor;
			//color = float3(reflectanceFactor, reflectanceFactor, reflectanceFactor);
			//color = reflectedColor;
			//color = reflectedColor * reflectanceFactor;
			// refractionColor = float3(1.0, 1.0, 1.0) * (1.0 - reflectanceFactor);
			//color = float3(1.0, 1.0, 1.0);
		}
	}

	color = phongColor + reflectedColor + refractionColor;

	//color *= diffuseColor;

	if (payload.inShadow == 1 && matValues.r == 0.0 && matValues.g == 0.0) {
		color = diffuseColor * 0.5f; // In shadow
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
