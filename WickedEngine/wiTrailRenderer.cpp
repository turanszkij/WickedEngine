#include "wiTrailRenderer.h"
#include "wiEventHandler.h"
#include "wiBacklog.h"
#include "wiTimer.h"
#include "wiRenderer.h"
#include "wiTextureHelper.h"
#include "wiScene.h"

using namespace wi::graphics;
using namespace wi::enums;
using namespace wi::scene;

namespace wi
{
	static Shader vertexShader;
	static Shader pixelShader;
	static InputLayout inputLayout;
	static BlendState			blendStates[BLENDMODE_COUNT];
	static RasterizerState		rasterizerState;
	static RasterizerState		wireFrameRS;
	static DepthStencilState	depthStencilState;
	static PipelineState		PSO[BLENDMODE_COUNT];
	static PipelineState		PSO_wire;

	void TrailRenderer::Cut()
	{
		if (points.empty())
			return;
		if (cuts.empty() || cuts.back() != points.size())
		{
			cuts.push_back((uint32_t)points.size());
		}
	}

	void TrailRenderer::Draw(const CameraComponent& camera, CommandList cmd) const
	{
		if (points.size() < 2)
			return;

		GraphicsDevice* device = GetDevice();

		device->EventBegin("TrailRenderer", cmd);

		if (wi::renderer::IsWireRender())
		{
			device->BindPipelineState(&PSO_wire, cmd);
		}
		else
		{
			device->BindPipelineState(&PSO[blendMode], cmd);
		}

		TrailRendererCB sb = {};
		sb.g_xTrailTransform = camera.VP;
		sb.g_xTrailColor = color;
		sb.g_xTrailTexMulAdd = texMulAdd;
		sb.g_xTrailTexMulAdd2 = texMulAdd2;
		sb.g_xTrailDepthSoften = 1.0f / (width * 10);
		sb.g_xTrailTextureIndex1 = device->GetDescriptorIndex(texture.IsValid() ? &texture : wi::texturehelper::getWhite(), SubresourceType::SRV);
		sb.g_xTrailTextureIndex2 = device->GetDescriptorIndex(texture2.IsValid() ? &texture : wi::texturehelper::getWhite(), SubresourceType::SRV);
		sb.g_xTrailLinearDepthTextureIndex = camera.texture_lineardepth_index;
		sb.g_xTrailCameraFar = camera.zFarP;
		device->BindDynamicConstantBuffer(sb, CBSLOT_TRAILRENDERER, cmd);

		struct Vertex
		{
			XMFLOAT3 position;
			uint16_t uvx;
			uint16_t uvy;
			XMHALF4 color;
		};

		uint32_t num_cuts = uint32_t(cuts.size());
		if (num_cuts == 0)
		{
			num_cuts += 1; // there were no cuts
		}
		if (!cuts.empty() && cuts.back() != (uint32_t)points.size())
		{
			num_cuts += 1; // when last segment was not indicated with cut
		}
		const uint32_t num_segments = uint32_t(points.size()) - num_cuts;

		const uint32_t subdivision = std::max(1u, this->subdivision);
		const uint32_t vertexCountAlloc = (num_segments * subdivision + num_cuts) * 2;

		const uint32_t indexCountAlloc = num_segments * subdivision * 6;

		auto mem = device->AllocateGPU(sizeof(Vertex) * vertexCountAlloc + sizeof(uint32_t) * indexCountAlloc, cmd);
		Vertex* vertices = (Vertex*)mem.data;
		uint32_t* indices = (uint32_t*)(vertices + vertexCountAlloc);

		const float resolution_rcp = 1.0f / subdivision;
		const XMVECTOR CAM = camera.GetEye();

		uint32_t next_cut = 0;

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		int i = 0;
		while (next_cut <= cuts.size())
		{
			int startcut = i;
			int count = next_cut < cuts.size() ? cuts[next_cut] : (uint32_t)points.size();
			float subdiv_count_within_cut = float((count - 1 - startcut) * subdivision);
			float subdiv_within_cut = 0;

			for (; i < count; ++i)
			{
				XMVECTOR P0 = XMLoadFloat3(&points[std::max(startcut, std::min(i - 1, count - 1))].position);
				XMVECTOR P1 = XMLoadFloat3(&points[std::max(startcut, std::min(i, count - 1))].position);
				XMVECTOR P2 = XMLoadFloat3(&points[std::max(startcut, std::min(i + 1, count - 1))].position);
				XMVECTOR P3 = XMLoadFloat3(&points[std::max(startcut, std::min(i + 2, count - 1))].position);

				float width_current = points[i].width;
				float width_next = points[std::min(i + 1, count - 1)].width;

				XMFLOAT4 color_current = points[i].color;
				XMFLOAT4 color_next = points[std::min(i + 1, count - 1)].color;

				if (i == startcut)
				{
					// when P0 == P1, centripetal catmull doesn't work, so we have to do a dummy control point
					P0 += P1 - P2;
				}
				if (i >= count - 2)
				{
					// when P2 == P3, centripetal catmull doesn't work, so we have to do a dummy control point
					P3 += P2 - P1;
				}

				bool cap = i == (count - 1);
				uint32_t segment_resolution = cap ? 1 : subdivision;

				for (uint32_t j = 0; j < segment_resolution; ++j)
				{
					float t = float(j) / float(segment_resolution);

					XMVECTOR P = cap ? XMVectorLerp(P1, P2, t) : wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t);
					XMVECTOR P_prev = cap ? XMVectorLerp(P0, P1, t - resolution_rcp) : wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t - resolution_rcp);
					XMVECTOR P_next = cap ? XMVectorLerp(P1, P2, t + resolution_rcp) : wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t + resolution_rcp);
					float width_interpolated = wi::math::Lerp(width_current, width_next, t);
					XMFLOAT4 color_interpolated = wi::math::Lerp(color_current, color_next, t);

					XMVECTOR T = XMVector3Normalize(P_next - P_prev);
					XMVECTOR B = XMVector3Normalize(XMVector3Cross(T, P - CAM));
					B *= width_interpolated * width;

					if (!cap)
					{
						indices[indexCount++] = vertexCount;
						indices[indexCount++] = vertexCount + 1;
						indices[indexCount++] = vertexCount + 2;
						indices[indexCount++] = vertexCount + 2;
						indices[indexCount++] = vertexCount + 1;
						indices[indexCount++] = vertexCount + 3;
					}

					const float cut_percent = subdiv_within_cut / subdiv_count_within_cut;
					subdiv_within_cut += 1;

					Vertex vert = {};
					vert.uvx = uint16_t(cut_percent * 65535);
					vert.color.x = XMConvertFloatToHalf(color_interpolated.x);
					vert.color.y = XMConvertFloatToHalf(color_interpolated.y);
					vert.color.z = XMConvertFloatToHalf(color_interpolated.z);
					vert.color.w = XMConvertFloatToHalf(color_interpolated.w);

					vert.uvy = 0;
					XMStoreFloat3(&vert.position, XMVectorSetW(P - B, 1));
					std::memcpy(vertices + vertexCount, &vert, sizeof(vert));
					vertexCount++;

					vert.uvy = 65535;
					XMStoreFloat3(&vert.position, XMVectorSetW(P + B, 1));
					std::memcpy(vertices + vertexCount, &vert, sizeof(vert));
					vertexCount++;
				}
			}
			next_cut++;
		}
		assert(vertexCount == vertexCountAlloc);
		assert(indexCount == indexCountAlloc);

		const GPUBuffer* vbs[] = {
			&mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint64_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		device->BindIndexBuffer(&mem.buffer, IndexBufferFormat::UINT32, mem.offset + sizeof(Vertex) * vertexCountAlloc, cmd);

		device->DrawIndexed(indexCount, 0, 0, cmd);

		device->EventEnd(cmd);
	}

	namespace TrailRenderer_Internal
	{
		void LoadShaders()
		{
			wi::renderer::LoadShader(ShaderStage::VS, vertexShader, "trailVS.cso");
			wi::renderer::LoadShader(ShaderStage::PS, pixelShader, "trailPS.cso");

			inputLayout.elements = {
				{ "POSITION", 0, Format::R32G32B32_FLOAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, InputClassification::PER_VERTEX_DATA },
				{ "TEXCOORD", 0, Format::R16G16_UNORM, 0, InputLayout::APPEND_ALIGNED_ELEMENT, InputClassification::PER_VERTEX_DATA },
				{ "COLOR", 0, Format::R16G16B16A16_FLOAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, InputClassification::PER_VERTEX_DATA },
			};

			GraphicsDevice* device = wi::graphics::GetDevice();

			for (int i = 0; i < BLENDMODE_COUNT; ++i)
			{
				PipelineStateDesc desc;
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				desc.vs = &vertexShader;
				desc.ps = &pixelShader;
				desc.il = &inputLayout;
				desc.bs = &blendStates[i];
				desc.rs = &rasterizerState;
				desc.dss = &depthStencilState;
				device->CreatePipelineState(&desc, &PSO[i]);
			}

			{
				PipelineStateDesc desc;
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				desc.vs = &vertexShader;
				desc.ps = &pixelShader;
				desc.il = &inputLayout;
				desc.bs = &blendStates[BLENDMODE_ALPHA];
				desc.rs = &wireFrameRS;
				desc.dss = &depthStencilState;

				device->CreatePipelineState(&desc, &PSO_wire);
			}
		}
	}
	void TrailRenderer::Initialize()
	{
		wi::Timer timer;

		RasterizerState rs;
		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::NONE;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = false;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rasterizerState = rs;


		rs.fill_mode = FillMode::WIREFRAME;
		rs.cull_mode = CullMode::NONE;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = false;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		wireFrameRS = rs;


		DepthStencilState dsd;
		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ZERO;
		dsd.depth_func = ComparisonFunc::GREATER_EQUAL;
		dsd.stencil_enable = false;
		depthStencilState = dsd;


		BlendState bd;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_ALPHA] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::ONE;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ZERO;
		bd.render_target[0].dest_blend_alpha = Blend::ONE;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_ADDITIVE] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::ONE;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::ONE;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_PREMULTIPLIED] = bd;

		bd.render_target[0].src_blend = Blend::DEST_COLOR;
		bd.render_target[0].dest_blend = Blend::ZERO;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::DEST_ALPHA;
		bd.render_target[0].dest_blend_alpha = Blend::ZERO;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.alpha_to_coverage_enable = false;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_MULTIPLY] = bd;

		bd.render_target[0].blend_enable = false;
		blendStates[BLENDMODE_OPAQUE] = bd;


		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { TrailRenderer_Internal::LoadShaders(); });
		TrailRenderer_Internal::LoadShaders();

		wi::backlog::post("wi::TrailRenderer Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}
}
