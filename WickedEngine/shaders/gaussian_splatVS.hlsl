#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

StructuredBuffer<uint> sortedIndexBuffer : register(t0);
StructuredBuffer<uint2> splatLookupBuffer : register(t1);
StructuredBuffer<ShaderGaussianSplatModel> modelBuffer : register(t2);

struct Push
{
	uint camera_count;
};
PUSHCONSTANT(push, Push);

static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),	// 0
	float3(1, -1, 0),	// 1
	float3(-1, 1, 0),	// 2
	float3(1, 1, 0),	// 4
};

// Helper functions from: https://github.com/nvpro-samples/vk_gaussian_splatting
//	Changed at places to fit Wicked Engine
/*
 * Copyright (c) 2023-2025, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2023-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

static const float splatScale = 1;
static const float frustumDilation = 0.2;

float3x3 fetchCovariance(in StructuredBuffer<GaussianSplat> splats, in uint splatIndex)
{
	const half3 cov3D_M11_M12_M13 = unpack_half4(splats[splatIndex].cov3D_M11_M12_M13_radius).xyz;
	const half3 cov3D_M22_M23_M33 = unpack_half3(splats[splatIndex].cov3D_M22_M23_M33);
	return float3x3(cov3D_M11_M12_M13.x, cov3D_M11_M12_M13.y, cov3D_M11_M12_M13.z, cov3D_M11_M12_M13.y, cov3D_M22_M23_M33.x, cov3D_M22_M23_M33.y, cov3D_M11_M12_M13.z, cov3D_M22_M23_M33.y, cov3D_M22_M23_M33.z);
}

// cov3Dm: 3D covariance matrix
// splatCenterView: splat center in view coordinates
// returns: the upper-left 2x2 portion of the projected 3D covariance matrix (see comments in function body). 
float3 threedgsCovarianceProjection(float3x3 cov3Dm, float4 splatCenterView, float2 focal, float4x4 modelViewTransform)
{
  // TODO: the ortho mode is not supported yet
#if ORTHOGRAPHIC_MODE == 1
  // Since the projection is linear, we don't need an approximation
  const float3x3 J = transpose(float3x3(orthoZoom, 0.0, 0.0, 0.0, orthoZoom, 0.0, 0.0, 0.0, 0.0));
#else
  // Construct the Jacobian of the affine approximation of the projection matrix. It will be used to transform the
  // 3D covariance matrix instead of using the actual projection matrix because that transformation would
  // require a non-linear component (perspective division) which would yield a non-gaussian result.
  const float s = 1.0 / (splatCenterView.z * splatCenterView.z);
  const float3x3  J = float3x3(focal.x / splatCenterView.z, 0., -(focal.x * splatCenterView.x) * s, 0.,
					   focal.y / splatCenterView.z, -(focal.y * splatCenterView.y) * s, 0., 0., 0.);
#endif

  // Concatenate the projection approximation with the model-view transformation
  // Row-major: extract 3x3 from the model-view transform (already in row-major)
  const float3x3 W = /*transpose*/((float3x3)modelViewTransform); // Wicked Engine: transpose not needed!
  // Row-major: matrix multiplication order (J * W instead of W * J)
  const float3x3 T = mul(J, W);

  // Transform the 3D covariance matrix (cov3Dm) to compute the 2D covariance matrix
  // Row-major: T^T * cov3Dm * T becomes T * cov3Dm * T^T
  const float3x3 cov2Dm = mul(mul(T, cov3Dm), transpose(T));

  // We are interested in the upper-left 2x2 portion of the projected 3D covariance matrix because
  // we only care about the X and Y values. We want the X-diagonal, cov2Dm[0][0],
  // the Y-diagonal, cov2Dm[1][1], and the correlation between the two cov2Dm[0][1]. We don't
  // need cov2Dm[1][0] because it is a symetric matrix.
  return float3(cov2Dm[0][0], cov2Dm[0][1], cov2Dm[1][1]);
}

// This function ingests the projected 2D covariance and outputs the basis vectors of its 2D extent
// opacity is updated if MipSplatting antialiasing is applied.
bool threedgsProjectedExtentBasis(float3 cov2Dv, float stdDev, float splatScale, inout float opacity, out float2 basisVector1, out float2 basisVector2)
{

#if MS_ANTIALIASING == 1
  // This mode is used when model is reconstructed using MipSplatting
  // https://niujinshuchong.github.io/mip-splatting/
  const float detOrig = cov2Dv[0] * cov2Dv[2] - cov2Dv[1] * cov2Dv[1];
#endif

  cov2Dv[0] += 0.3;
  cov2Dv[2] += 0.3;

#if MS_ANTIALIASING == 1
  const float detBlur = cov2Dv[0] * cov2Dv[2] - cov2Dv[1] * cov2Dv[1];
  // apply the alpha compensation
  opacity *= sqrt(max(detOrig / detBlur, 0.0));
#endif

  // We now need to solve for the eigen-values and eigen vectors of the 2D covariance matrix
  // so that we can determine the 2D basis for the splat. This is done using the method described
  // here: https://people.math.harvard.edu/~knill/teaching/math21b2004/exhibits/2dmatrices/index.html
  // After calculating the eigen-values and eigen-vectors, we calculate the basis for rendering the splat
  // by normalizing the eigen-vectors and then multiplying them by (stdDev * eigen-value), which is
  // equal to scaling them by stdDev standard deviations.
  //
  // This is a different approach than in the original work at INRIA. In that work they compute the
  // max extents of the projected splat in screen space to form a screen-space aligned bounding rectangle
  // which forms the geometry that is actually rasterized. The dimensions of that bounding box are 3.0
  // times the maximum eigen-value, or 3 standard deviations. They then use the inverse 2D covariance
  // matrix (called 'conic') in the CUDA rendering thread to determine fragment opacity by calculating the
  // full gaussian: exp(-0.5 * (X - mean) * conic * (X - mean)) * splat opacity
  const float a           = cov2Dv.x;
  const float d           = cov2Dv.z;
  const float b           = cov2Dv.y;
  const float D           = a * d - b * b;
  const float trace       = a + d;
  const float traceOver2  = 0.5 * trace;
  const float term2       = sqrt(max(0.1f, traceOver2 * traceOver2 - D));
  float       eigenValue1 = traceOver2 + term2;
  float       eigenValue2 = traceOver2 - term2;

  if(eigenValue2 <= 0.0)
  {
#pragma warning(disable: 41018) // Disable warning 41018 : returning without initializing some variables/parameters
    return false;
#pragma warning(default: 41018)
  }

#if POINT_CLOUD_MODE
  eigenValue1 = eigenValue2 = 0.2;
#endif

  const float2 eigenVector1 = normalize(float2(b, eigenValue1 - a));
  // since the eigen vectors are orthogonal, we derive the second one from the first
  const float2 eigenVector2 = float2(eigenVector1.y, -eigenVector1.x);

  basisVector1 = eigenVector1 * splatScale * min(stdDev * sqrt(eigenValue1), 2048.0);
  basisVector2 = eigenVector2 * splatScale * min(stdDev * sqrt(eigenValue2), 2048.0);

  return true;
}

static const float sqrt8    = sqrt(8.0);
static const float SH_C1    = 0.4886025119029199f;
static const float SH_C2[5] = { 1.0925484, -1.0925484, 0.3153916, -1.0925484, 0.5462742 };
static const float SH_C3[7] = { -0.5900435899266435f, 2.890611442640554f, -0.4570457994644658f, 0.3731763325901154f, -0.4570457994644658f, 1.445305721320277f, -0.5900435899266435f };

half3 nextSHCoeff(in Buffer<half> shBuffer, inout uint shIndex)
{
	const half r = shBuffer[shIndex++];
	const half g = shBuffer[shIndex++];
	const half b = shBuffer[shIndex++];
	return half3(r, g, b);
}

half3 fetchViewDependentRadiance(in ShaderGaussianSplatModel model, in uint splatIndex, in half3 worldViewDir)
{
	// contribution is null if MAX_SH_DEGREE < 1
	half3 rgb = half3(0.0, 0.0, 0.0);

	Buffer<half> shBuffer = bindless_buffers_half[NonUniformResourceIndex(descriptor_index(model.descriptor_shBuffer))];
	uint shIndex = splatIndex * model.splatStride;

	const half x = worldViewDir.x;
	const half y = worldViewDir.y;
	const half z = worldViewDir.z;

	if (model.sphericalHarmonicsDegree >= 1)
	{
		// SH coefficients for degree 1 (1,2,3)
		half3 shd1[3] = {
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
		};
		// Degree 1 contributions
		rgb += SH_C1 * (-shd1[0] * y + shd1[1] * z - shd1[2] * x);
	}

	const half xx = x * x;
	const half yy = y * y;
	const half zz = z * z;
	const half xy = x * y;
	const half yz = y * z;
	const half xz = x * z;

	if (model.sphericalHarmonicsDegree >= 2)
	{
		// SH coefficients for degree 2 (4 5 6 7 8)
		half3 shd2[5] = {
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
		};
		// Degree 2 contributions
		rgb += (SH_C2[0] * xy) * shd2[0] + (SH_C2[1] * yz) * shd2[1] + (SH_C2[2] * (2.0 * zz - xx - yy)) * shd2[2]
				+ (SH_C2[3] * xz) * shd2[3] + (SH_C2[4] * (xx - yy)) * shd2[4];
	}

	const half xyy = x * yy;
	const half yzz = y * zz;
	const half zxx = z * xx;
	const half xyz = x * y * z;

	if (model.sphericalHarmonicsDegree >= 3)
	{
		// SH coefficients for degree 3 (9,10,11,12,13,14,15)
		half3 shd3[7] = {
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
			nextSHCoeff(shBuffer, shIndex),
		};
		// Degree 3 contributions
		rgb += SH_C3[0] * shd3[0] * (3.0 * x * x - y * y) * y + SH_C3[1] * shd3[1] * x * y * z
				+ SH_C3[2] * shd3[2] * (4.0 * z * z - x * x - y * y) * y
				+ SH_C3[3] * shd3[3] * z * (2.0 * z * z - 3.0 * x * x - 3.0 * y * y)
				+ SH_C3[4] * shd3[4] * x * (4.0 * z * z - x * x - y * y) + SH_C3[5] * shd3[5] * (x * x - y * y) * z
				+ SH_C3[6] * shd3[6] * x * (x * x - 3.0 * y * y);
	}

	return rgb;
}

void main(in uint vertexID : SV_VertexID, in uint instanceID : SV_InstanceID, out float4 pos : SV_Position, out half4 color : COLOR, out half2 localPos : LOCALPOS, out uint RTIndex : SV_RenderTargetArrayIndex)
{
	const uint splatIndexGlobal = sortedIndexBuffer[instanceID / push.camera_count];
	const uint cameraIndex = instanceID % push.camera_count;

	ShaderCamera camera = GetCameraIndexed(cameraIndex);
	RTIndex = camera.output_index;

	const uint2 lookup = splatLookupBuffer[splatIndexGlobal];
	const uint modelIndex = lookup.x;
	const uint splatIndex = lookup.y;
	ShaderGaussianSplatModel model = modelBuffer[modelIndex];
	StructuredBuffer<GaussianSplat> splats = bindless_structured_gaussian_splats[NonUniformResourceIndex(descriptor_index(model.descriptor_splatBuffer))];

	//const float4x4 modelViewMatrix = mul(camera.view, model.transform.GetMatrix());
	const float4x4 modelViewMatrix = model.modelViewMatrices[cameraIndex]; // optimization: precomputed above matrix on CPU

	const float3 splatCenter = float4(splats[splatIndex].position, 1).xyz;
	color = unpack_half4(splats[splatIndex].color);
	half3 viewDir = normalize(splatCenter - mul(model.transform_inverse.GetMatrix(), float4(camera.position, 1)).xyz);
	color.rgb += fetchViewDependentRadiance(model, splatIndex, viewDir);
	color.rgb = RemoveSRGBCurve_Fast(color.rgb); // Checked against SuperSplat Editor, result is more similar with gamma remove

	const float4 viewCenter = mul(modelViewMatrix, float4(splatCenter, 1.0));
	const float4 clipCenter = mul(camera.projection, viewCenter);
	const float3x3 cov3Dm = fetchCovariance(splats, splatIndex);
	const float3 cov2Dv = threedgsCovarianceProjection(cov3Dm, viewCenter, camera.focal, modelViewMatrix); // computes the basis vectors of the extent of the projected covariance
	const float2 fragPos = BILLBOARD[vertexID].xy;
	localPos = fragPos * sqrt8;

	// Extra frustum culling in vertex shader:
	//	This is additionally on top of compute shader culling, fixes issues for multicamera render
	const float clip = (1.0 + frustumDilation) * clipCenter.w;
	if(abs(clipCenter.x) > clip || abs(clipCenter.y) > clip || clipCenter.z < (0.0 - frustumDilation) * clipCenter.w || clipCenter.z > clipCenter.w)
	{
		pos = 0; // degenerate, culled
		return;
	}

	// We use sqrt(8) standard deviations instead of 3 to eliminate more of the splat with a very low opacity.
	float2 basisVector1, basisVector2;
	if (!threedgsProjectedExtentBasis(cov2Dv, sqrt8, splatScale, color.a, basisVector1, basisVector2))
	{
		pos = 0; // degenerate, culled
		return;
	}

	const float3 ndcCenter = clipCenter.xyz / clipCenter.w;
	const float2 ndcOffset = float2(fragPos.x * basisVector1 + fragPos.y * basisVector2) * camera.internal_resolution_rcp * 2.0;
	const float4 quadPos = float4(ndcCenter.xy + ndcOffset, ndcCenter.z, 1.0);
	pos = quadPos;

}
