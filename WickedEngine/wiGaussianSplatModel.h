#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiPrimitive.h"
#include "wiVector.h"
#include "wiMath.h"
#include "wiECS.h"

namespace wi
{
	class Archive;

	class GaussianSplatModel
	{
	public:
		wi::vector<XMFLOAT3> positions;
		wi::vector<XMFLOAT3> normals;
		wi::vector<XMFLOAT4> rotations;
		wi::vector<XMFLOAT3> scales;
		wi::vector<float> opacities;
		wi::vector<XMFLOAT3> f_dc;
		wi::vector<float> f_rest; // 45 floats per splat (15 * rgb coefficient SH3)

		wi::primitive::AABB aabb;

		wi::graphics::GPUBuffer splatBuffer;
		wi::graphics::GPUBuffer indirectBuffer;
		wi::graphics::GPUBuffer sortedIndexBuffer;
		wi::graphics::GPUBuffer distanceBuffer;

		void CreateRenderData();

		void Update(wi::graphics::CommandList cmd);
		void Draw(wi::graphics::CommandList cmd);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		static void Initialize();
	};
}
