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

	void TrailRenderer::Clear()
	{
		points.clear();
		cuts.clear();
	}

	void TrailRenderer::AddPoint(const XMFLOAT3& position, float width, const XMFLOAT4& color, const XMFLOAT4& rotation)
	{
		TrailPoint& point = points.emplace_back();
		point.position = position;
		point.width = width;
		point.color = color;
		point.rotation = rotation;
	}

	void TrailRenderer::Cut(bool looped)
	{
		if (points.empty())
			return;
		if (cuts.empty() || cuts.back().id != points.size())
		{
			CutSegment cut;
			cut.id = (uint32_t)points.size();
			cut.looped = looped;
			cuts.push_back(cut);
		}
	}

	void TrailRenderer::Fade(float amount)
	{
		if (points.size() > 1)
		{
			for (auto& point : points)
			{
				point.color.w = saturate(point.color.w - amount);
			}
			if (points[0].color.w <= 0 && points[1].color.w <= 0)
			{
				points.erase(points.begin());
				for (auto& cut : cuts)
				{
					if (cut.id > 0)
					{
						cut.id--;
					}
				}
				if (!cuts.empty() && cuts[0].id == 0)
				{
					cuts.erase(cuts.begin());
				}
			}
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
		sb.g_xTrailDepthSoften = 1.0f / (width * depth_soften);
		sb.g_xTrailTextureIndex1 = device->GetDescriptorIndex(texture.IsValid() ? &texture : wi::texturehelper::getWhite(), SubresourceType::SRV);
		sb.g_xTrailTextureIndex2 = device->GetDescriptorIndex(texture2.IsValid() ? &texture2 : wi::texturehelper::getWhite(), SubresourceType::SRV);
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

		const uint32_t num_segments = uint32_t(points.size());
		const uint32_t subdivision = std::max(1u, this->subdivision);
		const uint32_t vertexCountAlloc = num_segments * subdivision * 2;

		const uint32_t indexCountAlloc = num_segments * (subdivision - 1) * 6;

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
			const int startcut = i;
			const int count = next_cut < cuts.size() ? cuts[next_cut].id : (uint32_t)points.size();
			const bool looped = next_cut < cuts.size() ? (bool)cuts[next_cut].looped : false;
			const float subdiv_count_within_cut = looped ? float((count - startcut) * subdivision) : float((count - 1 - startcut) * subdivision);
			float subdiv_within_cut = 0;
			int last = looped ? count : (count - 1);

			for (; i < last; ++i)
			{
				int t0 = 0; // prev
				int t1 = 0; // current
				int t2 = 1; // next
				int t3 = 1; // after next
				if (looped)
				{
					int cut_index = i - startcut;
					int cut_count = count - startcut;
					t0 = startcut + (cut_index + cut_count - 1) % cut_count;
					t1 = startcut + (cut_index + cut_count) % cut_count;
					t2 = startcut + (cut_index + cut_count + 1) % cut_count;
					t3 = startcut + (cut_index + cut_count + 2) % cut_count;
				}
				else
				{
					t0 = clamp(i - 1, startcut, last);
					t1 = clamp(i, startcut, last);
					t2 = clamp(i + 1, startcut, last);
					t3 = clamp(i + 2, startcut, last);
				}

				XMVECTOR P0 = XMLoadFloat3(&points[t0].position);
				XMVECTOR P1 = XMLoadFloat3(&points[t1].position);
				XMVECTOR P2 = XMLoadFloat3(&points[t2].position);
				XMVECTOR P3 = XMLoadFloat3(&points[t3].position);

				XMVECTOR W0 = XMVectorReplicate(points[t0].width);
				XMVECTOR W1 = XMVectorReplicate(points[t1].width);
				XMVECTOR W2 = XMVectorReplicate(points[t2].width);
				XMVECTOR W3 = XMVectorReplicate(points[t3].width);

				XMVECTOR C0 = XMLoadFloat4(&points[t0].color);
				XMVECTOR C1 = XMLoadFloat4(&points[t1].color);
				XMVECTOR C2 = XMLoadFloat4(&points[t2].color);
				XMVECTOR C3 = XMLoadFloat4(&points[t3].color);

				XMVECTOR Q0 = XMLoadFloat4(&points[t0].rotation);
				XMVECTOR Q1 = XMLoadFloat4(&points[t1].rotation);
				XMVECTOR Q2 = XMLoadFloat4(&points[t2].rotation);
				XMVECTOR Q3 = XMLoadFloat4(&points[t3].rotation);

				const bool rotation_enabled = points[i].rotation.x != 0 || points[i].rotation.y != 0 || points[i].rotation.z != 0 || points[i].rotation.w != 0;
				if (rotation_enabled)
				{
					Q0 = XMQuaternionNormalize(Q0);
					Q1 = XMQuaternionNormalize(Q1);
					Q2 = XMQuaternionNormalize(Q2);
					Q3 = XMQuaternionNormalize(Q3);
				}

				if (!looped)
				{
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
				}

				for (uint32_t j = 0; j < subdivision; ++j)
				{
					float t = float(j) / float(subdivision - 1);

					const XMVECTOR P = wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t);
					const XMVECTOR P_prev = wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t - resolution_rcp);
					const XMVECTOR P_next = wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t + resolution_rcp);
					const float width_interpolated = XMVectorGetX(XMVectorCatmullRom(W0, W1, W2, W3, t));
					const XMVECTOR C = XMVectorCatmullRom(C0, C1, C2, C3, t);
					XMFLOAT4 color_interpolated;
					XMStoreFloat4(&color_interpolated, C);

					XMVECTOR N;
					if (rotation_enabled)
					{
						XMVECTOR Q = XMVectorCatmullRom(Q0, Q1, Q2, Q3, t);
						Q = XMQuaternionNormalize(Q);
						N = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), Q);
					}
					else
					{
						N = P - CAM; // camera facing
					}
					N = XMVector3Normalize(N);
					XMVECTOR T = XMVector3Normalize(P_next - P_prev);
					XMVECTOR B = XMVector3Normalize(XMVector3Cross(T, N));
					N = XMVector3Normalize(XMVector3Cross(B, T)); // re-orthogonalize
					B *= width_interpolated * width;

					if (j < subdivision - 1)
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

			if (!looped)
				i++;
		}
		assert(vertexCount <= vertexCountAlloc);
		assert(indexCount <= indexCountAlloc);

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

		wilog("wi::TrailRenderer Initialized (%d ms)", (int)std::round(timer.elapsed()));
	}
}
