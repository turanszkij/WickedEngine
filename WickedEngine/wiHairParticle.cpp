#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiPrimitive.h"
#include "wiRandom.h"
#include "wiArchive.h"
#include "shaders/ShaderInterop.h"
#include "wiTextureHelper.h"
#include "wiScene.h"
#include "shaders/ShaderInterop_HairParticle.h"
#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiTimer.h"

using namespace wi::primitive;
using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::enums;

namespace wi
{
	static Shader vs;
	static Shader ps_prepass;
	static Shader ps;
	static Shader ps_simple;
	static Shader cs_simulate;
	static Shader cs_finishUpdate;
	static DepthStencilState dss_default, dss_equal;
	static RasterizerState rs, ncrs, wirers;
	static BlendState bs;
	static PipelineState PSO[RENDERPASS_COUNT];
	static PipelineState PSO_wire;

	void HairParticleSystem::UpdateCPU(const TransformComponent& transform, const MeshComponent& mesh, float dt)
	{
		world = transform.world;

		XMFLOAT3 _min = mesh.aabb.getMin();
		XMFLOAT3 _max = mesh.aabb.getMax();

		_max.x += length;
		_max.y += length;
		_max.z += length;

		_min.x -= length;
		_min.y -= length;
		_min.z -= length;

		aabb = AABB(_min, _max);
		aabb = aabb.transform(world);

		GraphicsDevice* device = wi::graphics::GetDevice();

		if (_flags & REBUILD_BUFFERS || !constantBuffer.IsValid() || (strandCount * segmentCount) != simulationBuffer.GetDesc().size / sizeof(PatchSimulationData))
		{
			_flags &= ~REBUILD_BUFFERS;
			regenerate_frame = true;

			GPUBufferDesc bd;
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;

			const uint32_t particleCount = strandCount * segmentCount;
			if (particleCount > 0)
			{
				bd.stride = sizeof(PatchSimulationData);
				bd.size = bd.stride * particleCount;
				device->CreateBuffer(&bd, nullptr, &simulationBuffer);
				device->SetName(&simulationBuffer, "simulationBuffer");

				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
				{
					bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
				}
				bd.stride = sizeof(MeshComponent::Vertex_POS);
				bd.size = bd.stride * 4 * particleCount;
				device->CreateBuffer(&bd, nullptr, &vertexBuffer_POS[0]);
				device->SetName(&vertexBuffer_POS[0], "vertexBuffer_POS[0]");
				device->CreateBuffer(&bd, nullptr, &vertexBuffer_POS[1]);
				device->SetName(&vertexBuffer_POS[1], "vertexBuffer_POS[1]");

				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.stride = sizeof(MeshComponent::Vertex_TEX);
				bd.size = bd.stride * 4 * particleCount;
				device->CreateBuffer(&bd, nullptr, &vertexBuffer_TEX);
				device->SetName(&vertexBuffer_TEX, "vertexBuffer_TEX");

				bd.bind_flags = BindFlag::SHADER_RESOURCE;
				bd.misc_flags = ResourceMiscFlag::NONE;
				if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
				{
					bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
				}
				bd.format = Format::R32_UINT;
				bd.stride = sizeof(uint);
				bd.size = bd.stride * 6 * particleCount;
				wi::vector<uint> primitiveData(6 * particleCount);
				for (uint particleID = 0; particleID < particleCount; ++particleID)
				{
					uint v0 = particleID * 4;
					uint i0 = particleID * 6;
					primitiveData[i0 + 0] = v0 + 0;
					primitiveData[i0 + 1] = v0 + 1;
					primitiveData[i0 + 2] = v0 + 2;
					primitiveData[i0 + 3] = v0 + 2;
					primitiveData[i0 + 4] = v0 + 1;
					primitiveData[i0 + 5] = v0 + 3;
				}
				device->CreateBuffer(&bd, primitiveData.data(), &primitiveBuffer);
				device->SetName(&primitiveBuffer, "primitiveBuffer");

				bd.bind_flags = BindFlag::INDEX_BUFFER | BindFlag::UNORDERED_ACCESS;
				bd.misc_flags = ResourceMiscFlag::NONE;
				bd.format = Format::R32_UINT;
				bd.stride = sizeof(uint);
				bd.size = bd.stride * 6 * particleCount;
				device->CreateBuffer(&bd, nullptr, &culledIndexBuffer);
				device->SetName(&culledIndexBuffer, "culledIndexBuffer");
			}

			bd.usage = Usage::DEFAULT;
			bd.size = sizeof(HairParticleCB);
			bd.bind_flags = BindFlag::CONSTANT_BUFFER;
			bd.misc_flags = ResourceMiscFlag::NONE;
			device->CreateBuffer(&bd, nullptr, &constantBuffer);
			device->SetName(&constantBuffer, "constantBuffer");

			if (vertex_lengths.size() != mesh.vertex_positions.size())
			{
				vertex_lengths.resize(mesh.vertex_positions.size());
				std::fill(vertex_lengths.begin(), vertex_lengths.end(), 1.0f);
			}

			indices.clear();
			for (size_t j = 0; j < mesh.indices.size(); j += 3)
			{
				const uint32_t triangle[] = {
					mesh.indices[j + 0],
					mesh.indices[j + 1],
					mesh.indices[j + 2],
				};
				if (vertex_lengths[triangle[0]] > 0 || vertex_lengths[triangle[1]] > 0 || vertex_lengths[triangle[2]] > 0)
				{
					indices.push_back(triangle[0]);
					indices.push_back(triangle[1]);
					indices.push_back(triangle[2]);
				}
			}

			if (!vertex_lengths.empty())
			{
				wi::vector<uint8_t> ulengths;
				ulengths.reserve(vertex_lengths.size());
				for (auto& x : vertex_lengths)
				{
					ulengths.push_back(uint8_t(wi::math::Clamp(x, 0, 1) * 255.0f));
				}

				bd.misc_flags = ResourceMiscFlag::NONE;
				bd.bind_flags = BindFlag::SHADER_RESOURCE;
				bd.format = Format::R8_UNORM;
				bd.stride = sizeof(uint8_t);
				bd.size = bd.stride * (uint32_t)ulengths.size();
				device->CreateBuffer(&bd, ulengths.data(), &vertexBuffer_length);
			}
			if (!indices.empty())
			{
				bd.misc_flags = ResourceMiscFlag::NONE;
				bd.bind_flags = BindFlag::SHADER_RESOURCE;
				bd.format = Format::R32_UINT;
				bd.stride = sizeof(uint32_t);
				bd.size = bd.stride * (uint32_t)indices.size();
				device->CreateBuffer(&bd, indices.data(), &indexBuffer);
			}

			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING) && primitiveBuffer.IsValid())
			{
				RaytracingAccelerationStructureDesc desc;
				desc.type = RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL;
				desc.flags |= RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE;
				desc.flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;

				desc.bottom_level.geometries.emplace_back();
				auto& geometry = desc.bottom_level.geometries.back();
				geometry.type = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES;
				geometry.triangles.vertex_buffer = vertexBuffer_POS[0];
				geometry.triangles.index_buffer = primitiveBuffer;
				geometry.triangles.index_format = IndexBufferFormat::UINT32;
				geometry.triangles.index_count = (uint32_t)(primitiveBuffer.desc.size / primitiveBuffer.desc.stride);
				geometry.triangles.index_offset = 0;
				geometry.triangles.vertex_count = (uint32_t)(vertexBuffer_POS[0].desc.size / vertexBuffer_POS[0].desc.stride);
				geometry.triangles.vertex_format = Format::R32G32B32_FLOAT;
				geometry.triangles.vertex_stride = sizeof(MeshComponent::Vertex_POS);

				bool success = device->CreateRaytracingAccelerationStructure(&desc, &BLAS);
				assert(success);
				device->SetName(&BLAS, "BLAS_hair");
			}
		}

		if (!indirectBuffer.IsValid())
		{
			GPUBufferDesc desc;
			desc.size = sizeof(uint) + sizeof(IndirectDrawArgsIndexedInstanced); // counter + draw args
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::INDIRECT_ARGS;
			desc.bind_flags = BindFlag::UNORDERED_ACCESS;
			device->CreateBuffer(&desc, nullptr, &indirectBuffer);
		}

		if (!subsetBuffer.IsValid())
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMeshSubset);
			desc.size = desc.stride;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			device->CreateBuffer(&desc, nullptr, &subsetBuffer);
		}

		std::swap(vertexBuffer_POS[0], vertexBuffer_POS[1]);

		if (BLAS.IsValid() && !BLAS.desc.bottom_level.geometries.empty())
		{
			BLAS.desc.bottom_level.geometries.back().triangles.vertex_buffer = vertexBuffer_POS[0];
		}

	}
	void HairParticleSystem::UpdateGPU(uint32_t instanceIndex, uint32_t materialIndex, const MeshComponent& mesh, const MaterialComponent& material, CommandList cmd) const
	{
		if (strandCount == 0 || !simulationBuffer.IsValid())
		{
			return;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("HairParticle - UpdateGPU", cmd);

		TextureDesc desc;
		if (material.textures[MaterialComponent::BASECOLORMAP].resource.IsValid())
		{
			desc = material.textures[MaterialComponent::BASECOLORMAP].resource.GetTexture().GetDesc();
		}
		HairParticleCB hcb;
		hcb.xHairWorld = world;
		hcb.xHairRegenerate = regenerate_frame ? 1 : 0;
		hcb.xLength = length;
		hcb.xStiffness = stiffness;
		hcb.xHairRandomness = randomness;
		hcb.xHairStrandCount = strandCount;
		hcb.xHairSegmentCount = std::max(segmentCount, 1u);
		hcb.xHairParticleCount = hcb.xHairStrandCount * hcb.xHairSegmentCount;
		hcb.xHairRandomSeed = randomSeed;
		hcb.xHairViewDistance = viewDistance;
		hcb.xHairBaseMeshIndexCount = (indices.empty() ? (uint)mesh.indices.size() : (uint)indices.size());
		hcb.xHairBaseMeshVertexPositionStride = sizeof(MeshComponent::Vertex_POS);
		// segmentCount will be loop in the shader, not a threadgroup so we don't need it here:
		hcb.xHairNumDispatchGroups = (hcb.xHairParticleCount + THREADCOUNT_SIMULATEHAIR - 1) / THREADCOUNT_SIMULATEHAIR;
		hcb.xHairFramesXY = uint2(std::max(1u, framesX), std::max(1u, framesY));
		hcb.xHairFrameCount = std::max(1u, frameCount);
		hcb.xHairFrameStart = frameStart;
		hcb.xHairTexMul = float2(1.0f / (float)hcb.xHairFramesXY.x, 1.0f / (float)hcb.xHairFramesXY.y);
		hcb.xHairAspect = (float)std::max(1u, desc.width) / (float)std::max(1u, desc.height);
		hcb.xHairLayerMask = layerMask;
		hcb.xHairInstanceIndex = instanceIndex;
		device->UpdateBuffer(&constantBuffer, &hcb, cmd);

		ShaderMeshSubset subset;
		subset.init();
		subset.indexOffset = 0;
		subset.materialIndex = materialIndex;
		device->UpdateBuffer(&subsetBuffer, &subset, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&constantBuffer, ResourceState::COPY_DST, ResourceState::CONSTANT_BUFFER),
				GPUBarrier::Buffer(&subsetBuffer, ResourceState::COPY_DST, ResourceState::SHADER_RESOURCE),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		// Simulate:
		{
			device->BindComputeShader(&cs_simulate, cmd);
			device->BindConstantBuffer(&constantBuffer, CB_GETBINDSLOT(HairParticleCB), cmd);

			const GPUResource* uavs[] = {
				&simulationBuffer,
				&vertexBuffer_POS[0],
				&vertexBuffer_TEX,
				&culledIndexBuffer,
				&indirectBuffer
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			const GPUResource* res[] = {
				indexBuffer.IsValid() ? &indexBuffer : &mesh.indexBuffer,
				mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
				&vertexBuffer_length
			};
			device->BindResources(res, 0, arraysize(res), cmd);

			device->Dispatch(hcb.xHairNumDispatchGroups, 1, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory()
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

		}

		// Finish update (reset counter, create indirect draw args):
		{
			device->BindComputeShader(&cs_finishUpdate, cmd);

			const GPUResource* uavs[] = {
				&indirectBuffer
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			device->Dispatch(1, 1, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(&indirectBuffer),
				GPUBarrier::Buffer(&indirectBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT),
				GPUBarrier::Buffer(&vertexBuffer_POS[0], ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&vertexBuffer_TEX, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&culledIndexBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::INDEX_BUFFER),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

		}

		device->EventEnd(cmd);

		regenerate_frame = false;
	}

	void HairParticleSystem::Draw(const MaterialComponent& material, wi::enums::RENDERPASS renderPass, CommandList cmd) const
	{
		if (strandCount == 0 || !constantBuffer.IsValid())
		{
			return;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("HairParticle - Draw", cmd);

		device->BindStencilRef(STENCILREF_DEFAULT, cmd);

		if (wi::renderer::IsWireRender())
		{
			if (renderPass == RENDERPASS_PREPASS)
			{
				return;
			}
			device->BindPipelineState(&PSO_wire, cmd);
			device->BindResource(wi::texturehelper::getWhite(), 0, cmd);
		}
		else
		{
			device->BindPipelineState(&PSO[renderPass], cmd);

			if (renderPass != RENDERPASS_PREPASS) // depth only alpha test will be full res
			{
				device->BindShadingRate(material.shadingRate, cmd);
			}
		}

		device->BindConstantBuffer(&constantBuffer, CB_GETBINDSLOT(HairParticleCB), cmd);
		device->BindResource(&primitiveBuffer, 0, cmd);

		device->BindIndexBuffer(&culledIndexBuffer, IndexBufferFormat::UINT32, 0, cmd);

		device->DrawIndexedInstancedIndirect(&indirectBuffer, 4, cmd);

		device->EventEnd(cmd);
	}


	void HairParticleSystem::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			wi::ecs::SerializeEntity(archive, meshID, seri);
			archive >> strandCount;
			archive >> segmentCount;
			archive >> randomSeed;
			archive >> length;
			archive >> stiffness;
			archive >> randomness;
			archive >> viewDistance;
			if (archive.GetVersion() >= 39)
			{
				archive >> vertex_lengths;
			}
			if (archive.GetVersion() >= 42)
			{
				archive >> framesX;
				archive >> framesY;
				archive >> frameCount;
				archive >> frameStart;
			}

			if (archive.GetVersion() == 48)
			{
				uint8_t shadingRate;
				archive >> shadingRate; // no longer needed
			}
		}
		else
		{
			archive << _flags;
			wi::ecs::SerializeEntity(archive, meshID, seri);
			archive << strandCount;
			archive << segmentCount;
			archive << randomSeed;
			archive << length;
			archive << stiffness;
			archive << randomness;
			archive << viewDistance;
			if (archive.GetVersion() >= 39)
			{
				archive << vertex_lengths;
			}
			if (archive.GetVersion() >= 42)
			{
				archive << framesX;
				archive << framesY;
				archive << frameCount;
				archive << frameStart;
			}
		}
	}

	namespace HairParticleSystem_Internal
	{
		void LoadShaders()
		{
			wi::renderer::LoadShader(ShaderStage::VS, vs, "hairparticleVS.cso");

			wi::renderer::LoadShader(ShaderStage::PS, ps_simple, "hairparticlePS_simple.cso");
			wi::renderer::LoadShader(ShaderStage::PS, ps_prepass, "hairparticlePS_prepass.cso");
			wi::renderer::LoadShader(ShaderStage::PS, ps, "hairparticlePS.cso");

			wi::renderer::LoadShader(ShaderStage::CS, cs_simulate, "hairparticle_simulateCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, cs_finishUpdate, "hairparticle_finishUpdateCS.cso");

			GraphicsDevice* device = wi::graphics::GetDevice();

			for (int i = 0; i < RENDERPASS_COUNT; ++i)
			{
				if (i == RENDERPASS_PREPASS || i == RENDERPASS_MAIN)
				{
					PipelineStateDesc desc;
					desc.vs = &vs;
					desc.bs = &bs;
					desc.rs = &ncrs;
					desc.dss = &dss_default;
					desc.pt = PrimitiveTopology::TRIANGLELIST;

					switch (i)
					{
					case RENDERPASS_PREPASS:
						desc.ps = &ps_prepass;
						break;
					case RENDERPASS_MAIN:
						desc.ps = &ps;
						desc.dss = &dss_equal;
						break;
					}

					device->CreatePipelineState(&desc, &PSO[i]);
				}
			}

			{
				PipelineStateDesc desc;
				desc.vs = &vs;
				desc.ps = &ps_simple;
				desc.bs = &bs;
				desc.rs = &wirers;
				desc.dss = &dss_default;
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				device->CreatePipelineState(&desc, &PSO_wire);
			}


		}
	}

	void HairParticleSystem::Initialize()
	{
		wi::Timer timer;

		RasterizerState rsd;
		rsd.fill_mode = FillMode::SOLID;
		rsd.cull_mode = CullMode::BACK;
		rsd.front_counter_clockwise = true;
		rsd.depth_bias = 0;
		rsd.depth_bias_clamp = 0;
		rsd.slope_scaled_depth_bias = 0;
		rsd.depth_clip_enable = true;
		rsd.multisample_enable = false;
		rsd.antialiased_line_enable = false;
		rs = rsd;

		rsd.fill_mode = FillMode::SOLID;
		rsd.cull_mode = CullMode::NONE;
		rsd.front_counter_clockwise = true;
		rsd.depth_bias = 0;
		rsd.depth_bias_clamp = 0;
		rsd.slope_scaled_depth_bias = 0;
		rsd.depth_clip_enable = true;
		rsd.multisample_enable = false;
		rsd.antialiased_line_enable = false;
		ncrs = rsd;

		rsd.fill_mode = FillMode::WIREFRAME;
		rsd.cull_mode = CullMode::NONE;
		rsd.front_counter_clockwise = true;
		rsd.depth_bias = 0;
		rsd.depth_bias_clamp = 0;
		rsd.slope_scaled_depth_bias = 0;
		rsd.depth_clip_enable = true;
		rsd.multisample_enable = false;
		rsd.antialiased_line_enable = false;
		wirers = rsd;


		DepthStencilState dsd;
		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ALL;
		dsd.depth_func = ComparisonFunc::GREATER;

		dsd.stencil_enable = true;
		dsd.stencil_read_mask = 0xFF;
		dsd.stencil_write_mask = 0xFF;
		dsd.front_face.stencil_func = ComparisonFunc::ALWAYS;
		dsd.front_face.stencil_pass_op = StencilOp::REPLACE;
		dsd.front_face.stencil_fail_op = StencilOp::KEEP;
		dsd.front_face.stencil_depth_fail_op = StencilOp::KEEP;
		dsd.back_face.stencil_func = ComparisonFunc::ALWAYS;
		dsd.back_face.stencil_pass_op = StencilOp::REPLACE;
		dsd.back_face.stencil_fail_op = StencilOp::KEEP;
		dsd.back_face.stencil_depth_fail_op = StencilOp::KEEP;
		dss_default = dsd;

		dsd.depth_write_mask = DepthWriteMask::ZERO;
		dsd.depth_func = ComparisonFunc::EQUAL;
		dsd.stencil_enable = false;
		dss_equal = dsd;


		BlendState bld;
		bld.render_target[0].blend_enable = false;
		bld.alpha_to_coverage_enable = false;
		bs = bld;

		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { HairParticleSystem_Internal::LoadShaders(); });
		HairParticleSystem_Internal::LoadShaders();

		wi::backlog::post("wi::HairParticleSystem Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}
}
