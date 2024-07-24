#pragma once

struct DummyVisualizer
{
	wi::graphics::GPUBuffer buffer;

	void Draw(
		const XMFLOAT3* vertices,
		uint32_t vertices_count,
		const unsigned int* indices,
		uint32_t indices_count,
		XMMATRIX matrix,
		XMFLOAT4 color,
		wi::graphics::CommandList cmd
	);
};
