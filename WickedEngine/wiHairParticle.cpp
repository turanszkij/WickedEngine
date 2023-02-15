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

	uint64_t HairParticleSystem::GetMemorySizeInBytes() const
	{
		if (!simulationBuffer.IsValid())
			return 0;

		uint64_t retVal = 0;

		retVal += simulationBuffer.GetDesc().size;
		retVal += constantBuffer.GetDesc().size;
		retVal += vertexBuffer_POS[0].GetDesc().size;
		retVal += vertexBuffer_POS[1].GetDesc().size;
		retVal += vertexBuffer_UVS.GetDesc().size;
		retVal += culledIndexBuffer.GetDesc().size;
		retVal += indirectBuffer.GetDesc().size;
		retVal += indexBuffer.GetDesc().size;
		retVal += vertexBuffer_length.GetDesc().size;

		return retVal;
	}

	void HairParticleSystem::CreateFromMesh(const wi::scene::MeshComponent& mesh)
	{
		if (vertex_lengths.size() != mesh.vertex_positions.size())
		{
			vertex_lengths.resize(mesh.vertex_positions.size());
			std::fill(vertex_lengths.begin(), vertex_lengths.end(), 1.0f);
		}

		vertex_lengths_packed.resize(vertex_lengths.size());
		for (size_t i = 0; i < vertex_lengths.size(); ++i)
		{
			vertex_lengths_packed[i] = uint8_t(wi::math::Clamp(vertex_lengths[i], 0, 1) * 255.0f);
		}

		indices.clear();
		uint32_t first_subset = 0;
		uint32_t last_subset = 0;
		mesh.GetLODSubsetRange(0, first_subset, last_subset);
		for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
		{
			const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
			for (size_t i = 0; i < subset.indexCount; i += 3)
			{
				const uint32_t i0 = mesh.indices[subset.indexOffset + i + 0];
				const uint32_t i1 = mesh.indices[subset.indexOffset + i + 1];
				const uint32_t i2 = mesh.indices[subset.indexOffset + i + 2];
				if (vertex_lengths[i0] > 0 || vertex_lengths[i1] > 0 || vertex_lengths[i2] > 0)
				{
					indices.push_back(i0);
					indices.push_back(i1);
					indices.push_back(i2);
				}
			}
		}
	}

	void HairParticleSystem::CreateRenderData()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		BLAS = {};
		_flags &= ~REBUILD_BUFFERS;
		regenerate_frame = true;

		GPUBufferDesc bd;
		bd.usage = Usage::DEFAULT;
		bd.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;

		const uint32_t particleCount = GetParticleCount();
		if (particleCount > 0)
		{
			bd.stride = sizeof(PatchSimulationData);
			bd.size = bd.stride * particleCount;
			device->CreateBuffer(&bd, nullptr, &simulationBuffer);
			device->SetName(&simulationBuffer, "HairParticleSystem::simulationBuffer");

			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
			}
			bd.stride = sizeof(MeshComponent::Vertex_POS);
			bd.size = bd.stride * 4 * particleCount;
			device->CreateBuffer(&bd, nullptr, &vertexBuffer_POS[0]);
			device->SetName(&vertexBuffer_POS[0], "HairParticleSystem::vertexBuffer_POS[0]");
			device->CreateBuffer(&bd, nullptr, &vertexBuffer_POS[1]);
			device->SetName(&vertexBuffer_POS[1], "HairParticleSystem::vertexBuffer_POS[1]");

			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.stride = sizeof(MeshComponent::Vertex_UVS);
			bd.size = bd.stride * 4 * particleCount;
			device->CreateBuffer(&bd, nullptr, &vertexBuffer_UVS);
			device->SetName(&vertexBuffer_UVS, "HairParticleSystem::vertexBuffer_UVS");

			bd.misc_flags = ResourceMiscFlag::NONE;
			bd.bind_flags = BindFlag::INDEX_BUFFER | BindFlag::UNORDERED_ACCESS;
			bd.format = ((particleCount * 4) < 65536) ? Format::R16_UINT : Format::R32_UINT;
			bd.stride = GetFormatStride(bd.format);
			bd.size = bd.stride * 6 * particleCount;
			device->CreateBuffer(&bd, nullptr, &culledIndexBuffer);
			device->SetName(&culledIndexBuffer, "HairParticleSystem::culledIndexBuffer");

			primitiveBuffer = wi::renderer::GetIndexBufferForQuads(particleCount);
		}

		bd.usage = Usage::DEFAULT;
		bd.size = sizeof(HairParticleCB);
		bd.bind_flags = BindFlag::CONSTANT_BUFFER;
		bd.misc_flags = ResourceMiscFlag::NONE;
		device->CreateBuffer(&bd, nullptr, &constantBuffer);
		device->SetName(&constantBuffer, "HairParticleSystem::constantBuffer");

		if (!vertex_lengths.empty())
		{
			bd.misc_flags = ResourceMiscFlag::NONE;
			bd.bind_flags = BindFlag::SHADER_RESOURCE;
			bd.format = Format::R8_UNORM;
			bd.stride = sizeof(uint8_t);
			bd.size = bd.stride * vertex_lengths_packed.size();
			device->CreateBuffer(&bd, vertex_lengths_packed.data(), &vertexBuffer_length);
			device->SetName(&vertexBuffer_length, "HairParticleSystem::vertexBuffer_length");
		}
		if (!indices.empty())
		{
			bd.misc_flags = ResourceMiscFlag::NONE;
			bd.bind_flags = BindFlag::SHADER_RESOURCE;
			bd.format = Format::R32_UINT;
			bd.stride = sizeof(uint32_t);
			bd.size = bd.stride * indices.size();
			device->CreateBuffer(&bd, indices.data(), &indexBuffer);
			device->SetName(&indexBuffer, "HairParticleSystem::indexBuffer");
		}

		if (!indirectBuffer.IsValid())
		{
			GPUBufferDesc desc;
			desc.size = sizeof(uint) + sizeof(IndirectDrawArgsIndexedInstanced); // counter + draw args
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::INDIRECT_ARGS;
			desc.bind_flags = BindFlag::UNORDERED_ACCESS;
			device->CreateBuffer(&desc, nullptr, &indirectBuffer);
		}
	}
	void HairParticleSystem::CreateRaytracingRenderData()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

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
			geometry.triangles.index_format = GetIndexBufferFormat(primitiveBuffer.desc.format);
			geometry.triangles.index_count = GetParticleCount() * 6;
			geometry.triangles.index_offset = 0;
			geometry.triangles.vertex_count = (uint32_t)(vertexBuffer_POS[0].desc.size / vertexBuffer_POS[0].desc.stride);
			geometry.triangles.vertex_format = Format::R32G32B32_FLOAT;
			geometry.triangles.vertex_stride = sizeof(MeshComponent::Vertex_POS);

			bool success = device->CreateRaytracingAccelerationStructure(&desc, &BLAS);
			assert(success);
			device->SetName(&BLAS, "HairParticleSystem::BLAS");
		}
	}

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

		if ((_flags & REBUILD_BUFFERS) || indices.empty())
		{
			CreateFromMesh(mesh);
		}

		if ((_flags & REBUILD_BUFFERS) || !constantBuffer.IsValid() || GetParticleCount() != simulationBuffer.GetDesc().size / sizeof(PatchSimulationData))
		{
			CreateRenderData();
		}

		std::swap(vertexBuffer_POS[0], vertexBuffer_POS[1]);

		if (BLAS.IsValid() && !BLAS.desc.bottom_level.geometries.empty())
		{
			BLAS.desc.bottom_level.geometries.back().triangles.vertex_buffer = vertexBuffer_POS[0];
		}
	}
	void HairParticleSystem::UpdateGPU(
		const UpdateGPUItem* items,
		uint32_t itemCount,
		CommandList cmd
	)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("HairParticleSystem - UpdateGPU", cmd);

		static thread_local wi::vector<GPUBarrier> barrier_stack;
		auto barrier_stack_flush = [&]()
		{
			if (barrier_stack.empty())
				return;
			device->Barrier(barrier_stack.data(), (uint32_t)barrier_stack.size(), cmd);
			barrier_stack.clear();
		};

		for (uint32_t i = 0; i < itemCount; ++i)
		{
			const UpdateGPUItem& item = items[i];
			const HairParticleSystem& hair = *item.hair;
			if (hair.strandCount == 0 || !hair.simulationBuffer.IsValid())
			{
				continue;
			}
			const MeshComponent& mesh = *item.mesh;
			const MaterialComponent& material = *item.material;

			TextureDesc desc;
			if (material.textures[MaterialComponent::BASECOLORMAP].resource.IsValid())
			{
				desc = material.textures[MaterialComponent::BASECOLORMAP].resource.GetTexture().GetDesc();
			}
			HairParticleCB hcb;
			hcb.xHairTransform.Create(hair.world);
			hcb.xHairRegenerate = hair.regenerate_frame ? 1 : 0;
			hcb.xLength = hair.length;
			hcb.xStiffness = hair.stiffness;
			hcb.xHairRandomness = hair.randomness;
			hcb.xHairStrandCount = hair.strandCount;
			hcb.xHairSegmentCount = std::max(hair.segmentCount, 1u);
			hcb.xHairParticleCount = hcb.xHairStrandCount * hcb.xHairSegmentCount;
			hcb.xHairRandomSeed = hair.randomSeed;
			hcb.xHairViewDistance = hair.viewDistance;
			hcb.xHairBaseMeshIndexCount = (uint)hair.indices.size();
			hcb.xHairBaseMeshVertexPositionStride = sizeof(MeshComponent::Vertex_POS);
			hcb.xHairFramesXY = uint2(std::max(1u, hair.framesX), std::max(1u, hair.framesY));
			hcb.xHairFrameCount = std::max(1u, hair.frameCount);
			hcb.xHairFrameStart = hair.frameStart;
			hcb.xHairTexMul = float2(1.0f / (float)hcb.xHairFramesXY.x, 1.0f / (float)hcb.xHairFramesXY.y);
			hcb.xHairAspect = (float)std::max(1u, desc.width) / (float)std::max(1u, desc.height);
			hcb.xHairLayerMask = hair.layerMask;
			hcb.xHairInstanceIndex = item.instanceIndex;
			device->UpdateBuffer(&hair.constantBuffer, &hcb, cmd);

			if (hair.regenerate_frame)
			{
				hair.regenerate_frame = false;
				device->ClearUAV(&hair.simulationBuffer, 0, cmd);
				device->ClearUAV(&hair.vertexBuffer_POS[0], 0, cmd);
				device->ClearUAV(&hair.vertexBuffer_POS[1], 0, cmd);
				device->ClearUAV(&hair.vertexBuffer_UVS, 0, cmd);
				device->ClearUAV(&hair.culledIndexBuffer, 0, cmd);
				device->ClearUAV(&hair.indirectBuffer, 0, cmd);

				barrier_stack.push_back(GPUBarrier::Memory(&hair.simulationBuffer));
				barrier_stack.push_back(GPUBarrier::Memory(&hair.vertexBuffer_POS[0]));
				barrier_stack.push_back(GPUBarrier::Memory(&hair.vertexBuffer_POS[1]));
				barrier_stack.push_back(GPUBarrier::Memory(&hair.vertexBuffer_UVS));
				barrier_stack.push_back(GPUBarrier::Memory(&hair.culledIndexBuffer));
				barrier_stack.push_back(GPUBarrier::Memory(&hair.indirectBuffer));
			}

			barrier_stack.push_back(GPUBarrier::Buffer(&hair.constantBuffer, ResourceState::COPY_DST, ResourceState::CONSTANT_BUFFER));
		}

		barrier_stack_flush();

		// Simulate:
		device->BindComputeShader(&cs_simulate, cmd);
		for (uint32_t i = 0; i < itemCount; ++i)
		{
			const UpdateGPUItem& item = items[i];
			const HairParticleSystem& hair = *item.hair;
			if (hair.strandCount == 0 || !hair.simulationBuffer.IsValid())
			{
				continue;
			}
			const MeshComponent& mesh = *item.mesh;

			device->BindConstantBuffer(&hair.constantBuffer, CB_GETBINDSLOT(HairParticleCB), cmd);

			const GPUResource* uavs[] = {
				&hair.simulationBuffer,
				&hair.vertexBuffer_POS[0],
				&hair.vertexBuffer_UVS,
				&hair.culledIndexBuffer,
				&hair.indirectBuffer,
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			if (hair.indexBuffer.IsValid())
			{
				device->BindResource(&hair.indexBuffer, 0, cmd);
			}
			else
			{
				device->BindResource(&mesh.generalBuffer, 0, cmd, mesh.ib.subresource_srv);
			}
			if (mesh.streamoutBuffer.IsValid())
			{
				device->BindResource(&mesh.streamoutBuffer, 1, cmd, mesh.so_pos_nor_wind.subresource_srv);
			}
			else
			{
				device->BindResource(&mesh.generalBuffer, 1, cmd, mesh.vb_pos_nor_wind.subresource_srv);
			}
			device->BindResource(&hair.vertexBuffer_length, 2, cmd);

			device->Dispatch((hair.strandCount + THREADCOUNT_SIMULATEHAIR - 1) / THREADCOUNT_SIMULATEHAIR, 1, 1, cmd);

			barrier_stack.push_back(GPUBarrier::Memory(&hair.simulationBuffer));
			barrier_stack.push_back(GPUBarrier::Memory(&hair.indirectBuffer));
			barrier_stack.push_back(GPUBarrier::Buffer(&hair.vertexBuffer_POS[0], ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE));
			barrier_stack.push_back(GPUBarrier::Buffer(&hair.vertexBuffer_UVS, ResourceState::UNORDERED_ACCESS, ResourceState::SHADER_RESOURCE));
			barrier_stack.push_back(GPUBarrier::Buffer(&hair.culledIndexBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::INDEX_BUFFER));
		}

		barrier_stack_flush();

		// Finish update (reset counter, create indirect draw args):
		device->BindComputeShader(&cs_finishUpdate, cmd);
		for (uint32_t i = 0; i < itemCount; ++i)
		{
			const UpdateGPUItem& item = items[i];
			const HairParticleSystem& hair = *item.hair;
			if (hair.strandCount == 0 || !hair.simulationBuffer.IsValid())
			{
				continue;
			}

			const GPUResource* uavs[] = {
				&hair.indirectBuffer
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			device->Dispatch(1, 1, 1, cmd);

			barrier_stack.push_back(GPUBarrier::Buffer(&hair.indirectBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT));
		}

		barrier_stack_flush();

		device->EventEnd(cmd);
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

		device->BindIndexBuffer(&culledIndexBuffer, GetIndexBufferFormat(culledIndexBuffer.desc.format), 0, cmd);

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
