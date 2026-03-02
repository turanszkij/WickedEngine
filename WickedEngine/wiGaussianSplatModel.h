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
		wi::vector<XMFLOAT4> rotations;
		wi::vector<XMFLOAT3> scales;
		wi::vector<float> opacities;
		wi::vector<XMFLOAT3> f_dc;
		wi::vector<float> f_rest; // 45 floats per splat (15 * rgb coefficient SH3)

		// Below this are non-serialized attributes:
		wi::primitive::AABB aabb;
		wi::graphics::GPUBuffer splatBuffer;
		wi::graphics::GPUBuffer shBuffer;
		wi::graphics::GPUBuffer indirectBuffer;
		wi::graphics::GPUBuffer sortedIndexBuffer;
		wi::graphics::GPUBuffer distanceBuffer;
		wi::graphics::GPUBuffer constantBuffer;

		size_t GetSplatCount() const { return positions.size(); };
		int GetSphericalHarmonicsDegree() const;
		size_t GetMemorySizeCPU() const;
		size_t GetMemorySizeGPU() const;

		void CreateRenderData();

		void Update(const XMFLOAT4X4& transform, wi::graphics::CommandList cmd);
		void Draw(wi::graphics::CommandList cmd);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		static void Initialize();
	};
}
