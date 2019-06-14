// Copyright (C) 2009-2019, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// Contains functions for light calculations

#pragma once

#include <shaders/Functions.glsl>
#include <shaders/Pack.glsl>
#include <shaders/glsl_cpp_common/ClusteredShading.h>

const U32 SHADOW_SAMPLE_COUNT = 16;
#if !defined(ESM_CONSTANT)
const F32 ESM_CONSTANT = 40.0;
#endif

// Fresnel term unreal
Vec3 F_Unreal(Vec3 specular, F32 VoH)
{
	return specular + (1.0 - specular) * pow(2.0, (-5.55473 * VoH - 6.98316) * VoH);
}

// Fresnel Schlick: "An Inexpensive BRDF Model for Physically-Based Rendering"
// It has lower VGRPs than F_Unreal
Vec3 F_Schlick(Vec3 specular, F32 VoH)
{
	const F32 a = 1.0 - VoH;
	const F32 a2 = a * a;
	const F32 a5 = a2 * a2 * a; // a5 = a^5
	return /*saturate(50.0 * specular.g) */ a5 + (1.0 - a5) * specular;
}

// D(n,h) aka NDF: GGX Trowbridge-Reitz
F32 D_GGX(F32 roughness, F32 NoH)
{
	const F32 a = roughness * roughness;
	const F32 a2 = a * a;

	const F32 D = (NoH * a2 - NoH) * NoH + 1.0;
	return a2 / (PI * D * D);
}

// Visibility term: Geometric shadowing divided by BRDF denominator
F32 V_Schlick(F32 roughness, F32 NoV, F32 NoL)
{
	const F32 k = (roughness * roughness) * 0.5;
	const F32 Vis_SchlickV = NoV * (1.0 - k) + k;
	const F32 Vis_SchlickL = NoL * (1.0 - k) + k;
	return 0.25 / (Vis_SchlickV * Vis_SchlickL);
}

Vec3 envBRDF(Vec3 specular, F32 roughness, texture2D integrationLut, sampler integrationLutSampler, F32 NoV)
{
	const F32 a = roughness * roughness;
	const F32 a2 = a * a;
	const Vec2 envBRDF = textureLod(integrationLut, integrationLutSampler, Vec2(a2, NoV), 0.0).xy;
	return specular * envBRDF.x + /*min(1.0, 50.0 * specular.g) */ envBRDF.y;
}

Vec3 diffuseLambert(Vec3 diffuse)
{
	return diffuse * (1.0 / PI);
}

// Performs BRDF specular lighting
Vec3 computeSpecularColorBrdf(GbufferInfo gbuffer, Vec3 viewDir, Vec3 frag2Light)
{
	const Vec3 H = normalize(frag2Light + viewDir);

	const F32 NoL = max(EPSILON, dot(gbuffer.m_normal, frag2Light));
	const F32 VoH = max(EPSILON, dot(viewDir, H));
	const F32 NoH = max(EPSILON, dot(gbuffer.m_normal, H));
	const F32 NoV = max(EPSILON, dot(gbuffer.m_normal, viewDir));

	// F
#if 0
	const Vec3 F = F_Unreal(gbuffer.m_specular, VoH);
#else
	const Vec3 F = F_Schlick(gbuffer.m_specular, VoH);
#endif

	// D
	const F32 D = D_GGX(gbuffer.m_roughness, NoH);

	// Vis
	const F32 V = V_Schlick(gbuffer.m_roughness, NoV, NoL);

	return F * (V * D);
}

F32 computeSpotFactor(Vec3 l, F32 outerCos, F32 innerCos, Vec3 spotDir)
{
	const F32 costheta = -dot(l, spotDir);
	const F32 spotFactor = smoothstep(outerCos, innerCos, costheta);
	return spotFactor;
}

U32 computeShadowSampleCount(const U32 COUNT, F32 zVSpace)
{
	const F32 MAX_DISTANCE = 5.0;

	const F32 z = max(zVSpace, -MAX_DISTANCE);
	F32 sampleCountf = F32(COUNT) + z * (F32(COUNT) / MAX_DISTANCE);
	sampleCountf = max(sampleCountf, 1.0);
	const U32 sampleCount = U32(sampleCountf);

	return sampleCount;
}

F32 computeShadowFactorSpotLight(SpotLight light, Vec3 worldPos, texture2D spotMap, sampler spotMapSampler)
{
	const Vec4 texCoords4 = light.m_texProjectionMat * Vec4(worldPos, 1.0);
	const Vec3 texCoords3 = texCoords4.xyz / texCoords4.w;

	const F32 near = LIGHT_FRUSTUM_NEAR_PLANE;
	const F32 far = light.m_radius;

	const F32 linearDepth = linearizeDepth(texCoords3.z, near, far);

	const F32 shadowFactor = textureLod(spotMap, spotMapSampler, texCoords3.xy, 0.0).r;

	return saturate(exp(ESM_CONSTANT * (shadowFactor - linearDepth)));
}

// Compute the shadow factor of point (omni) lights.
F32 computeShadowFactorPointLight(PointLight light, Vec3 frag2Light, texture2D shadowMap, sampler shadowMapSampler)
{
	const Vec3 dir = -frag2Light;
	const Vec3 dirabs = abs(dir);
	const F32 dist = max(dirabs.x, max(dirabs.y, dirabs.z));

	const F32 near = LIGHT_FRUSTUM_NEAR_PLANE;
	const F32 far = light.m_radius;

	const F32 linearDepth = (dist - near) / (far - near);

	// Read tex
	F32 shadowFactor;
	{
		// Convert cube coords
		U32 faceIdxu;
		Vec2 uv = convertCubeUvsu(dir, faceIdxu);

		// Get the atlas offset
		Vec2 atlasOffset;
		atlasOffset.x = light.m_shadowAtlasTileOffsets[faceIdxu >> 1u][(faceIdxu & 1u) << 1u];
		atlasOffset.y = light.m_shadowAtlasTileOffsets[faceIdxu >> 1u][((faceIdxu & 1u) << 1u) + 1u];

		// Compute UV
		uv = fma(uv, Vec2(light.m_shadowAtlasTileScale), atlasOffset);

		// Sample
		shadowFactor = textureLod(shadowMap, shadowMapSampler, uv, 0.0).r;
	}

	return saturate(exp(ESM_CONSTANT * (shadowFactor - linearDepth)));
}

// Compute the shadow factor of a directional light
F32 computeShadowFactorDirLight(
	DirectionalLight light, U32 cascadeIdx, Vec3 worldPos, texture2D shadowMap, sampler shadowMapSampler)
{
	const Mat4 lightProjectionMat = light.m_textureMatrices[cascadeIdx];

	const Vec4 texCoords4 = lightProjectionMat * Vec4(worldPos, 1.0);
	const Vec3 texCoords3 = texCoords4.xyz / texCoords4.w;

	const F32 cascadeLinearDepth = texCoords3.z;

	F32 shadowFactor = textureLod(shadowMap, shadowMapSampler, texCoords3.xy, 0.0).r;
	shadowFactor = saturate(exp(ESM_CONSTANT * 3.0 * (shadowFactor - cascadeLinearDepth)));

	return shadowFactor;
}

// Compute the shadow factor of a directional light
F32 computeShadowFactorDirLight(
	Mat4 lightProjectionMat, Vec3 worldPos, texture2D shadowMap, samplerShadow shadowMapSampler)
{
	const Vec4 texCoords4 = lightProjectionMat * Vec4(worldPos, 1.0);
	const Vec3 texCoords3 = texCoords4.xyz / texCoords4.w;

	const F32 shadowFactor = textureLod(shadowMap, shadowMapSampler, texCoords3, 0.0);
	return shadowFactor;
}

// Compute the cubemap texture lookup vector given the reflection vector (r) the radius squared of the probe (R2) and
// the frag pos in sphere space (f)
Vec3 computeCubemapVecAccurate(Vec3 r, F32 R2, Vec3 f)
{
	// Compute the collision of the r to the inner part of the sphere
	// From now on we work on the sphere's space

	// Project the center of the sphere (it's zero now since we are in sphere space) in ray "f,r"
	const Vec3 p = f - r * dot(f, r);

	// The collision to the sphere is point x where x = p + T * r
	// Because of the pythagorean theorem: R^2 = dot(p, p) + dot(T * r, T * r)
	// solving for T, T = R / |p|
	// then x becomes x = sqrt(R^2 - dot(p, p)) * r + p;
	F32 pp = dot(p, p);
	pp = min(pp, R2);
	const F32 sq = sqrt(R2 - pp);
	const Vec3 x = p + sq * r;

	return x;
}

// Cheap version of computeCubemapVecAccurate
Vec3 computeCubemapVecCheap(Vec3 r, F32 R2, Vec3 f)
{
	return r;
}

F32 computeAttenuationFactor(F32 lightRadius, Vec3 frag2Light)
{
	const F32 fragLightDist = dot(frag2Light, frag2Light);
	F32 att = 1.0 - fragLightDist * lightRadius;
	att = max(0.0, att);
	return att * att;
}

// Given the probe properties trace a ray inside the probe and find the cube tex coordinates to sample
Vec3 intersectProbe(Vec3 fragPos, // Ray origin
	Vec3 rayDir, // Ray direction
	Vec3 probeAabbMin,
	Vec3 probeAabbMax,
	Vec3 probeOrigin // Cubemap origin
)
{
	// Compute the intersection point
	const F32 intresectionDist = rayAabbIntersectionInside(fragPos, rayDir, probeAabbMin, probeAabbMax);
	const Vec3 intersectionPoint = fragPos + intresectionDist * rayDir;

	// Compute the cubemap vector
	return intersectionPoint - probeOrigin;
}

// Compute a weight (factor) of fragPos against some probe's bounds. The weight will be zero when fragPos is close to
// AABB bounds and 1.0 at fadeDistance and less.
F32 computeProbeBlendWeight(Vec3 fragPos, // Doesn't need to be inside the AABB
	Vec3 probeAabbMin,
	Vec3 probeAabbMax,
	F32 fadeDistance)
{
	// Compute the min distance of fragPos from the edges of the AABB
	const Vec3 distFromMin = fragPos - probeAabbMin;
	const Vec3 distFromMax = probeAabbMax - fragPos;
	const Vec3 minDistVec = min(distFromMin, distFromMax);
	const F32 minDist = min(minDistVec.x, min(minDistVec.y, minDistVec.z));

	// Use saturate because minDist might be negative.
	return saturate(minDist / fadeDistance);
}

// Given the value of the 6 faces of the dice and a normal, sample the correct weighted value.
// https://www.shadertoy.com/view/XtcBDB
Vec3 sampleAmbientDice(Vec3 posx, Vec3 negx, Vec3 posy, Vec3 negy, Vec3 posz, Vec3 negz, Vec3 normal)
{
	const Vec3 axisWeights = abs(normal);
	const Vec3 uv = NDC_TO_UV(normal);

	Vec3 col = mix(negx, posx, uv.x) * axisWeights.x;
	col += mix(negy, posy, uv.y) * axisWeights.y;
	col += mix(negz, posz, uv.z) * axisWeights.z;

	// Divide by weight
	col /= axisWeights.x + axisWeights.y + axisWeights.z + EPSILON;

	return col;
}

// Sample the irradiance term from the clipmap
Vec3 sampleGlobalIllumination(const Vec3 worldPos,
	const Vec3 normal,
	const GlobalIlluminationProbe probe,
	texture3D textures[MAX_VISIBLE_GLOBAL_ILLUMINATION_PROBES],
	sampler linearAnyClampSampler)
{
	// Find the UVW
	Vec3 uvw = (worldPos - probe.m_aabbMin) / (probe.m_aabbMax - probe.m_aabbMin);

	// The U contains the 6 directions so divide
	uvw.x /= 6.0;

	// Calmp it to avoid direction leaking
	uvw.x = clamp(uvw.x, probe.m_halfTexelSizeU, (1.0 / 6.0) - probe.m_halfTexelSizeU);

	// Read the irradiance
	Vec3 irradiancePerDir[6u];
	ANKI_UNROLL for(U32 dir = 0u; dir < 6u; ++dir)
	{
		// Point to the correct UV
		Vec3 shiftedUVw = uvw;
		shiftedUVw.x += (1.0 / 6.0) * F32(dir);

		irradiancePerDir[dir] =
			textureLod(textures[nonuniformEXT(probe.m_textureIndex)], linearAnyClampSampler, shiftedUVw, 0.0).rgb;
	}

	// Sample the irradiance
	const Vec3 irradiance = sampleAmbientDice(irradiancePerDir[0],
		irradiancePerDir[1],
		irradiancePerDir[2],
		irradiancePerDir[3],
		irradiancePerDir[4],
		irradiancePerDir[5],
		normal);

	return irradiance;
}
