#pragma once

struct DummyVisualizer
{
	wi::graphics::GPUBuffer buffer;

	void Draw(
		const XMFLOAT3* vertices,
		uint32_t vertices_count,
		const unsigned int* indices,
		uint32_t indices_count,
		const XMMATRIX& matrix,
		const XMFLOAT4& color,
		bool depth,
		wi::graphics::CommandList cmd
	);
};

namespace dummy
{
	void draw_male(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd);
	void draw_female(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd);
	void draw_soldier(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd);
	void draw_direction(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd);
	void draw_waypoint(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd);
	void draw_pickup(const XMMATRIX& matrix, const XMFLOAT4& color, bool depth, wi::graphics::CommandList cmd);
}
