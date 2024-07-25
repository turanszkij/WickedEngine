#include "stdafx.h"
#include "DummyVisualizer.h"

using namespace wi::graphics;

void DummyVisualizer::Draw(
	const XMFLOAT3* vertices,
	uint32_t vertices_count,
	const unsigned int* indices,
	uint32_t indices_count,
	const XMMATRIX& matrix,
	const XMFLOAT4& color,
	bool depth,
	wi::graphics::CommandList cmd
)
{
	GraphicsDevice* device = GetDevice();

	static PipelineState pso[2];
	if (!pso[0].IsValid() || !pso[1].IsValid())
	{
		static auto LoadShaders = [depth] {
			PipelineStateDesc desc;
			desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
			desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
			desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
			desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
			desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
			desc.pt = PrimitiveTopology::TRIANGLELIST;
			desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
			wi::graphics::GetDevice()->CreatePipelineState(&desc, &pso[0]);
			desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEFAULT);
			wi::graphics::GetDevice()->CreatePipelineState(&desc, &pso[1]);
		};
		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();
	}
	struct Vertex
	{
		XMFLOAT4 position;
		XMFLOAT4 color;
	};

	if (!buffer.IsValid())
	{
		auto fill_data = [&](void* data) {
			Vertex* gpu_vertices = (Vertex*)data;
			for (size_t i = 0; i < vertices_count; ++i)
			{
				Vertex vert = {};
				vert.position.x = vertices[i].x;
				vert.position.y = vertices[i].y;
				vert.position.z = vertices[i].z;
				vert.position.w = 1;
				vert.color = XMFLOAT4(1, 1, 1, 1);
				std::memcpy(gpu_vertices + i, &vert, sizeof(vert));
			}

			uint32_t* gpu_indices = (uint32_t*)(gpu_vertices + vertices_count);
			std::memcpy(gpu_indices, indices, indices_count * sizeof(uint32_t));
			};

		GPUBufferDesc desc;
		desc.size = indices_count * sizeof(uint32_t) + vertices_count * sizeof(Vertex);
		desc.bind_flags = BindFlag::INDEX_BUFFER | BindFlag::VERTEX_BUFFER;
		device->CreateBuffer2(&desc, fill_data, &buffer);
		device->SetName(&buffer, "DummyVisualizer::buffer");
	}

	device->BindPipelineState(&pso[depth], cmd);

	MiscCB sb;
	XMStoreFloat4x4(&sb.g_xTransform, matrix);
	sb.g_xColor = color;
	device->BindDynamicConstantBuffer(sb, CB_GETBINDSLOT(MiscCB), cmd);

	const GPUBuffer* vbs[] = {
		&buffer,
	};
	const uint32_t strides[] = {
		sizeof(Vertex),
	};
	const uint64_t offsets[] = {
		0,
	};
	device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
	device->BindIndexBuffer(&buffer, IndexBufferFormat::UINT32, vertices_count * sizeof(Vertex), cmd);
	device->DrawIndexed((uint32_t)indices_count, 0, 0, cmd);
}

namespace dummy
{
	void draw_male(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd)
	{
#include "dummy_male.h"
		static DummyVisualizer vis;
		vis.Draw(vertices, arraysize(vertices), indices, arraysize(indices), matrix, color, depth, cmd);
	}
	void draw_female(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd)
	{
#include "dummy_female.h"
		static DummyVisualizer vis;
		vis.Draw(vertices, arraysize(vertices), indices, arraysize(indices), matrix, color, depth, cmd);
	}
	void draw_soldier(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd)
	{
#include "dummy_soldier.h"
		static DummyVisualizer vis;
		vis.Draw(vertices, arraysize(vertices), indices, arraysize(indices), matrix, color, depth, cmd);
	}
	void draw_direction(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd)
	{
#include "dummy_direction.h"
		static DummyVisualizer vis;
		vis.Draw(vertices, arraysize(vertices), indices, arraysize(indices), matrix, color, depth, cmd);
	}
	void draw_waypoint(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd)
	{
#include "dummy_waypoint.h"
		static DummyVisualizer vis;
		vis.Draw(vertices, arraysize(vertices), indices, arraysize(indices), matrix, color, depth, cmd);
	}
	void draw_pickup(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd)
	{
#include "dummy_pickup.h"
		static DummyVisualizer vis;
		vis.Draw(vertices, arraysize(vertices), indices, arraysize(indices), matrix, color, depth, cmd);
	}
}
