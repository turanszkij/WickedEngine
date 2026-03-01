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
			int targetSplatStride = splatStride;

			for (size_t splatIdx = 0; splatIdx < positions.size(); ++splatIdx)
			{
				aabb.AddPoint(positions[splatIdx]);

				GaussianSplat splat = {};
				splat.position = positions[splatIdx];
				splat.normal = normals[splatIdx];
				splat.rotation = rotations[splatIdx];

				splat.scale.x = std::exp(scales[splatIdx].x);
				splat.scale.y = std::exp(scales[splatIdx].y);
				splat.scale.z = std::exp(scales[splatIdx].z);

				static const float sqrt8 = std::sqrt(8.0f);
				splat.scale.x *= sqrt8;
				splat.scale.y *= sqrt8;
				splat.scale.z *= sqrt8;

				static constexpr float SH_C0 = 0.28209479177387814f;
				splat.f_dc.x = saturate(0.5f + SH_C0 * f_dc[splatIdx].x);
				splat.f_dc.y = saturate(0.5f + SH_C0 * f_dc[splatIdx].y);
				splat.f_dc.z = saturate(0.5f + SH_C0 * f_dc[splatIdx].z);

				splat.opacity = saturate(1.0f / (1.0f + std::exp(-opacities[splatIdx])));

				float* dst = (float*)splat.f_rest;

				const auto srcBase = splatStride * splatIdx;
				int dstOffset = 0;
				// degree 1, three coefs per component
				for (auto i = 0; i < 3; i++)
				{
					for (auto rgb = 0; rgb < 3; rgb++)
					{
						const auto srcIndex = srcBase + (sphericalHarmonicsCoefficientsPerChannel * rgb + i);
						dst[dstOffset++] = f_rest[srcIndex];
					}
				}
				// degree 2, five coefs per component
				for (auto i = 0; i < 5; i++)
				{
					for (auto rgb = 0; rgb < 3; rgb++)
					{
						const auto srcIndex = srcBase + (sphericalHarmonicsCoefficientsPerChannel * rgb + 3 + i);
						dst[dstOffset++] = f_rest[srcIndex];
					}
				}
				// degree 3, seven coefs per component
				for (auto i = 0; i < 7; i++)
				{
					for (auto rgb = 0; rgb < 3; rgb++)
					{
						const auto srcIndex = srcBase + (sphericalHarmonicsCoefficientsPerChannel * rgb + 3 + 5 + i);
						dst[dstOffset++] = f_rest[srcIndex];
					}
				}

				std::memcpy(splat_dest + splatIdx, &splat, sizeof(splat)); // memcpy into uncached
			}
		};

		GPUBufferDesc desc;
		desc.stride = sizeof(GaussianSplat);
		desc.size = positions.size() * desc.stride;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
		bool success = device->CreateBuffer2(&desc, fill_gpu, &splatBuffer);
		assert(success);
		device->SetName(&splatBuffer, "GaussianSplatModel::splatBuffer");

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
	}

	void GaussianSplatModel::Update(wi::graphics::CommandList cmd)
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Gaussian Splat Update", cmd);

		IndirectDrawArgsInstanced args = {};
		args.VertexCountPerInstance = 4;
		args.InstanceCount = 0; // shader atomic dest
		args.StartVertexLocation = 0;
		args.StartInstanceLocation = 0;
		device->UpdateBuffer(&indirectBuffer, &args, cmd);
		device->Barrier(GPUBarrier::Buffer(&indirectBuffer, ResourceState::COPY_DST, ResourceState::UNORDERED_ACCESS), cmd);

		device->BindComputeShader(&computeShader, cmd);
		device->BindResource(&splatBuffer, 0, cmd);
		device->BindUAV(&indirectBuffer, 0, cmd);
		device->BindUAV(&sortedIndexBuffer, 1, cmd);
		device->BindUAV(&distanceBuffer, 2, cmd);
		device->Dispatch(((uint32_t)positions.size() + 63u) / 64u, 1, 1, cmd);
		GPUBarrier barriers[] = {
			GPUBarrier::Buffer(&indirectBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
			GPUBarrier::Buffer(&sortedIndexBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
			GPUBarrier::Buffer(&distanceBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
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
		device->DrawInstancedIndirect(&indirectBuffer, 0, cmd);
		device->EventEnd(cmd);
	}

	void GaussianSplatModel::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> positions;
			archive >> normals;
			archive >> rotations;
			archive >> scales;
			archive >> opacities;
			archive >> f_dc;
			archive >> f_rest;
		}
		else
		{
			archive << positions;
			archive << normals;
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

		rasterizerState.cull_mode = CullMode::NONE;
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
}
