#include "wiGaussianSplatModel.h"
#include "shaders/ShaderInterop_GaussianSplat.h"
#include "wiRenderer.h"
#include "wiBacklog.h"
#include "wiTimer.h"
#include "wiEventHandler.h"
#include "wiGPUSortLib.h"

using namespace wi::math;
using namespace wi::graphics;
using namespace wi::primitive;

namespace wi
{
	static Shader computeShader;
	static Shader vertexShader;
	static Shader pixelShader;
	static BlendState blendState;
	static RasterizerState rasterizerState;
	static DepthStencilState depthStencilState;
	static PipelineState pipelineState;

	void GaussianSplatModel::CreateRenderData()
	{
		GraphicsDevice* device = GetDevice();

		aabb = AABB();

		auto fill_gpu = [&](void* dest) {
			GaussianSplat* splat_dest = (GaussianSplat*)dest;
			for (size_t splatIdx = 0; splatIdx < positions.size(); ++splatIdx)
			{
				aabb.AddPoint(positions[splatIdx]);

				GaussianSplat splat = {};
				splat.position = positions[splatIdx];

				XMFLOAT3 scale = XMFLOAT3(std::exp(scales[splatIdx].x), std::exp(scales[splatIdx].y), std::exp(scales[splatIdx].z));
				static const float sqrt8 = std::sqrt(8.0f);
				splat.radius = std::max(scale.x, std::max(scale.y, scale.z)) * sqrt8; // culling

				// f_dc is L0 spherical harmonics (not view dependent), so it's converted to rgb color here
				// https://github.com/nvpro-samples/vk_gaussian_splatting/blob/main/src/splat_set_vk.cpp
				static constexpr float SH_C0 = 0.28209479177387814f;
				float4 color = {};
				color.x = saturate(0.5f + SH_C0 * f_dc[splatIdx].x);
				color.y = saturate(0.5f + SH_C0 * f_dc[splatIdx].y);
				color.z = saturate(0.5f + SH_C0 * f_dc[splatIdx].z);
				color.w = saturate(1.0f / (1.0f + std::exp(-opacities[splatIdx])));
				splat.color = pack_half4(color);

				// covariance from: https://github.com/nvpro-samples/vk_gaussian_splatting/blob/main/src/splat_set_vk.cpp
				//	changed from glm to DirectXMath (column->row major, matrix mul order changed)
				const XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
				const XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(XMLoadFloat4(&rotations[splatIdx]));
				const XMMATRIX covarianceMatrix = XMMatrixMultiply(scaleMatrix, rotationMatrix);
				const XMMATRIX transformedCovarianceMatrix = XMMatrixMultiply(XMMatrixTranspose(covarianceMatrix), covarianceMatrix);
				XMFLOAT3X3 transformedCovariance;
				XMStoreFloat3x3(&transformedCovariance, transformedCovarianceMatrix);
				splat.cov3D_M11_M12_M13.x = transformedCovariance._11;
				splat.cov3D_M11_M12_M13.y = transformedCovariance._12;
				splat.cov3D_M11_M12_M13.z = transformedCovariance._13;
				splat.cov3D_M22_M23_M33.x = transformedCovariance._22;
				splat.cov3D_M22_M23_M33.y = transformedCovariance._23;
				splat.cov3D_M22_M23_M33.z = transformedCovariance._33;

				std::memcpy(splat_dest + splatIdx, &splat, sizeof(splat)); // memcpy into uncached
			}
		};

		GPUBufferDesc desc;
		desc.stride = sizeof(GaussianSplat);
		desc.size = positions.size() * desc.stride;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
		desc.format = Format::UNKNOWN;
		bool success = device->CreateBuffer2(&desc, fill_gpu, &splatBuffer);
		assert(success);
		device->SetName(&splatBuffer, "GaussianSplatModel::splatBuffer");

		// I create separate float16 array for view dependent SH data, it will be tightly packed and supports variable order SH without predefining it into the splat structured buffer
		auto fill_gpu_sh = [&](void* dest) {
			const uint32_t totalSphericalHarmonicsComponentCount = uint32_t(f_rest.size() / positions.size());
			const uint32_t sphericalHarmonicsCoefficientsPerChannel = totalSphericalHarmonicsComponentCount / 3;
			int sphericalHarmonicsDegree = 0;
			int splatStride = 0;
			if (sphericalHarmonicsCoefficientsPerChannel >= 3)
			{
				sphericalHarmonicsDegree = 1;
				splatStride += 3 * 3;
			}
			if (sphericalHarmonicsCoefficientsPerChannel >= 8)
			{
				sphericalHarmonicsDegree = 2;
				splatStride += 5 * 3;
			}
			if (sphericalHarmonicsCoefficientsPerChannel == 15)
			{
				sphericalHarmonicsDegree = 3;
				splatStride += 7 * 3;
			}
			for (size_t splatIdx = 0; splatIdx < positions.size(); ++splatIdx)
			{
				// View dependent SH data is deinterleaved, now I interleave it into 16x (rgb) vectors per splat:
				uint16_t* dst = (uint16_t*)dest + splatStride * splatIdx;
				const auto srcBase = splatStride * splatIdx;
				int dstOffset = 0;
				// degree 1, three coefs per component
				for (auto i = 0; i < 3; i++)
				{
					for (auto rgb = 0; rgb < 3; rgb++)
					{
						const auto srcIndex = srcBase + (sphericalHarmonicsCoefficientsPerChannel * rgb + i);
						dst[dstOffset++] = XMConvertFloatToHalf(f_rest[srcIndex]);
					}
				}
				// degree 2, five coefs per component
				for (auto i = 0; i < 5; i++)
				{
					for (auto rgb = 0; rgb < 3; rgb++)
					{
						const auto srcIndex = srcBase + (sphericalHarmonicsCoefficientsPerChannel * rgb + 3 + i);
						dst[dstOffset++] = XMConvertFloatToHalf(f_rest[srcIndex]);
					}
				}
				// degree 3, seven coefs per component
				for (auto i = 0; i < 7; i++)
				{
					for (auto rgb = 0; rgb < 3; rgb++)
					{
						const auto srcIndex = srcBase + (sphericalHarmonicsCoefficientsPerChannel * rgb + 3 + 5 + i);
						dst[dstOffset++] = XMConvertFloatToHalf(f_rest[srcIndex]);
					}
				}
			}
		};
		desc.format = Format::R16_FLOAT;
		desc.stride = sizeof(uint16_t);
		desc.size = f_rest.size() * desc.stride;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.misc_flags = ResourceMiscFlag::NONE;
		success = device->CreateBuffer2(&desc, fill_gpu_sh, &shBuffer);
		assert(success);
		device->SetName(&shBuffer, "GaussianSplatModel::shBuffer");

		desc.format = Format::UNKNOWN;
		desc.stride = sizeof(IndirectDrawArgsInstanced);
		desc.size = desc.stride;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED | ResourceMiscFlag::INDIRECT_ARGS;
		success = device->CreateBuffer(&desc, nullptr, &indirectBuffer);
		assert(success);
		device->SetName(&indirectBuffer, "GaussianSplatModel::indirectBuffer");

		desc.stride = sizeof(uint32_t);
		desc.size = positions.size() * desc.stride;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
		success = device->CreateBuffer(&desc, nullptr, &sortedIndexBuffer);
		assert(success);
		device->SetName(&sortedIndexBuffer, "GaussianSplatModel::sortedIndexBuffer");

		desc.stride = sizeof(float);
		desc.size = positions.size() * desc.stride;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
		success = device->CreateBuffer(&desc, nullptr, &distanceBuffer);
		assert(success);
		device->SetName(&distanceBuffer, "GaussianSplatModel::distanceBuffer");

		desc.stride = sizeof(GaussianSplatCB);
		desc.size = desc.stride;
		desc.bind_flags = BindFlag::CONSTANT_BUFFER;
		desc.misc_flags = ResourceMiscFlag::NONE;
		success = device->CreateBuffer(&desc, nullptr, &constantBuffer);
		assert(success);
		device->SetName(&constantBuffer, "GaussianSplatModel::constantBuffer");
	}

	void GaussianSplatModel::Update(const XMFLOAT4X4& transform, wi::graphics::CommandList cmd)
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Gaussian Splat Update", cmd);

		XMFLOAT4X4 transform_inverse;
		XMStoreFloat4x4(&transform_inverse, XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform)));

		const uint32_t totalSphericalHarmonicsComponentCount = uint32_t(f_rest.size() / positions.size());
		const uint32_t sphericalHarmonicsCoefficientsPerChannel = totalSphericalHarmonicsComponentCount / 3;
		int sphericalHarmonicsDegree = 0;
		int splatStride = 0;
		if (sphericalHarmonicsCoefficientsPerChannel >= 3)
		{
			sphericalHarmonicsDegree = 1;
			splatStride += 3 * 3;
		}
		if (sphericalHarmonicsCoefficientsPerChannel >= 8)
		{
			sphericalHarmonicsDegree = 2;
			splatStride += 5 * 3;
		}
		if (sphericalHarmonicsCoefficientsPerChannel == 15)
		{
			sphericalHarmonicsDegree = 3;
			splatStride += 7 * 3;
		}

		GaussianSplatCB cb = {};
		cb.transform.Create(transform);
		cb.transform_inverse.Create(transform_inverse);
		cb.sphericalHarmonicsDegree = sphericalHarmonicsDegree;
		cb.splatStride = splatStride;
		device->UpdateBuffer(&constantBuffer, &cb, cmd);

		IndirectDrawArgsInstanced args = {};
		args.VertexCountPerInstance = 4;
		args.InstanceCount = 0; // shader atomic dest
		args.StartVertexLocation = 0;
		args.StartInstanceLocation = 0;
		device->UpdateBuffer(&indirectBuffer, &args, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&constantBuffer, ResourceState::COPY_DST, ResourceState::CONSTANT_BUFFER),
				GPUBarrier::Buffer(&indirectBuffer, ResourceState::COPY_DST, ResourceState::UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->BindComputeShader(&computeShader, cmd);
		device->BindResource(&splatBuffer, 0, cmd);
		device->BindUAV(&indirectBuffer, 0, cmd);
		device->BindUAV(&sortedIndexBuffer, 1, cmd);
		device->BindUAV(&distanceBuffer, 2, cmd);
		device->BindConstantBuffer(&constantBuffer, CBSLOT_GAUSSIANSPLAT, cmd);

		// Some GPU can't exceed dispatch group count of 65535 in single dimension (DX12 validation), so I do multiple dispatches with max 65535 group count each:
		int remaining_threadgroups = ((int)positions.size() + GAUSSIAN_COMPUTE_THREADSIZE - 1) / GAUSSIAN_COMPUTE_THREADSIZE;
		uint32_t group_offset = 0;
		while (remaining_threadgroups > 0)
		{
			uint32_t dispatch_offset = group_offset * GAUSSIAN_COMPUTE_THREADSIZE;
			device->PushConstants(&dispatch_offset, sizeof(dispatch_offset), cmd);
			const uint32_t threadgroups = (uint32_t)std::min(remaining_threadgroups, 65535);
			device->Dispatch(threadgroups, 1, 1, cmd);
			remaining_threadgroups -= threadgroups;
			group_offset += threadgroups;
		}

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&indirectBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&sortedIndexBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&distanceBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		wi::gpusortlib::Sort((uint32_t)positions.size(), distanceBuffer, indirectBuffer, offsetof(IndirectDrawArgsInstanced, InstanceCount), sortedIndexBuffer, cmd);
		device->Barrier(GPUBarrier::Buffer(&indirectBuffer, ResourceState::SHADER_RESOURCE, ResourceState::INDIRECT_ARGUMENT), cmd);

		device->EventEnd(cmd);
	}

	void GaussianSplatModel::Draw(wi::graphics::CommandList cmd)
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Gaussian Splat Render", cmd);
		device->BindPipelineState(&pipelineState, cmd);
		device->BindResource(&splatBuffer, 0, cmd);
		device->BindResource(&sortedIndexBuffer, 1, cmd);
		device->BindResource(&shBuffer, 2, cmd);
		device->BindConstantBuffer(&constantBuffer, CBSLOT_GAUSSIANSPLAT, cmd);
		device->DrawInstancedIndirect(&indirectBuffer, 0, cmd);
		device->EventEnd(cmd);
	}

	void GaussianSplatModel::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> positions;
			archive >> rotations;
			archive >> scales;
			archive >> opacities;
			archive >> f_dc;
			archive >> f_rest;

			wi::jobsystem::Execute(seri.ctx, [this](wi::jobsystem::JobArgs args) {
				CreateRenderData();
			});
		}
		else
		{
			archive << positions;
			archive << rotations;
			archive << scales;
			archive << opacities;
			archive << f_dc;
			archive << f_rest;
		}
	}

	void GaussianSplatModel::Initialize()
	{
		wi::Timer timer;

		static auto LoadShaders = []{
			wi::renderer::LoadShader(ShaderStage::CS, computeShader, "gaussian_splatCS.cso");
			wi::renderer::LoadShader(ShaderStage::VS, vertexShader, "gaussian_splatVS.cso");
			wi::renderer::LoadShader(ShaderStage::PS, pixelShader, "gaussian_splatPS.cso");
		};
		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		GraphicsDevice* device = GetDevice();

		depthStencilState.depth_enable = true;
		depthStencilState.depth_write_mask = DepthWriteMask::ZERO;
		depthStencilState.depth_func = ComparisonFunc::GREATER;

		rasterizerState.cull_mode = CullMode::BACK;
		rasterizerState.fill_mode = FillMode::SOLID;

		blendState.render_target[0].blend_enable = true;
		blendState.render_target[0].src_blend = Blend::ONE;
		blendState.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		blendState.render_target[0].blend_op = BlendOp::ADD;
		blendState.render_target[0].src_blend_alpha = Blend::ONE;
		blendState.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		blendState.render_target[0].blend_op_alpha = BlendOp::ADD;
		blendState.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;

		PipelineStateDesc desc;
		desc.vs = &vertexShader;
		desc.ps = &pixelShader;
		desc.rs = &rasterizerState;
		desc.bs = &blendState;
		desc.dss = &depthStencilState;
		desc.pt = PrimitiveTopology::TRIANGLESTRIP;
		bool success = device->CreatePipelineState(&desc, &pipelineState);
		assert(success);

		wilog("wi::GaussianSplatModel Initialized (%d ms)", (int)std::round(timer.elapsed()));
	}

	int GaussianSplatModel::GetSphericalHarmonicsDegree() const
	{
		const uint32_t totalSphericalHarmonicsComponentCount = uint32_t(f_rest.size() / positions.size());
		const uint32_t sphericalHarmonicsCoefficientsPerChannel = totalSphericalHarmonicsComponentCount / 3;
		int sphericalHarmonicsDegree = 0;
		if (sphericalHarmonicsCoefficientsPerChannel >= 3)
		{
			sphericalHarmonicsDegree = 1;
		}
		if (sphericalHarmonicsCoefficientsPerChannel >= 8)
		{
			sphericalHarmonicsDegree = 2;
		}
		if (sphericalHarmonicsCoefficientsPerChannel == 15)
		{
			sphericalHarmonicsDegree = 3;
		}
		return sphericalHarmonicsDegree;
	}
	size_t GaussianSplatModel::GetMemorySizeCPU() const
	{
		size_t ret = 0;
		ret += positions.size() * sizeof(XMFLOAT3);
		ret += rotations.size() * sizeof(XMFLOAT4);
		ret += scales.size() * sizeof(XMFLOAT3);
		ret += opacities.size() * sizeof(float);
		ret += f_dc.size() * sizeof(XMFLOAT3);
		ret += f_rest.size() * sizeof(float);
		return ret;
	}
	size_t GaussianSplatModel::GetMemorySizeGPU() const
	{
		size_t ret = 0;
		ret += splatBuffer.desc.size;
		ret += shBuffer.desc.size;
		ret += indirectBuffer.desc.size;
		ret += sortedIndexBuffer.desc.size;
		ret += distanceBuffer.desc.size;
		ret += constantBuffer.desc.size;
		return ret;
	}
}
