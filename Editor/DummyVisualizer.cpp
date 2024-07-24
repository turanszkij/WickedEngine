#include "stdafx.h"
#include "DummyVisualizer.h"

using namespace wi::graphics;

void DummyVisualizer::Draw(
	const XMFLOAT3* vertices,
	uint32_t vertices_count,
	const unsigned int* indices,
	uint32_t indices_count,
	XMMATRIX matrix,
	XMFLOAT4 color,
	CommandList cmd
)
{
	GraphicsDevice* device = GetDevice();

	static PipelineState pso;
	if (!pso.IsValid())
	{
		static auto LoadShaders = [] {
			PipelineStateDesc desc;
			desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
			desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
			desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
			desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHREAD);
			desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
			desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
			desc.pt = PrimitiveTopology::TRIANGLELIST;
			wi::graphics::GetDevice()->CreatePipelineState(&desc, &pso);
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

	device->BindPipelineState(&pso, cmd);

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
