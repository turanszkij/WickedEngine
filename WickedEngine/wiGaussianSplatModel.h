#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiPrimitive.h"
#include "wiVector.h"
#include "wiMath.h"
#include "wiECS.h"
#include "wiScene_Decl.h"

namespace wi
{
	class Archive;

	// One gaussian splat asset:
	class GaussianSplatModel
	{
	public:
		wi::vector<XMFLOAT3> positions;
		wi::vector<XMFLOAT4> rotations;
		wi::vector<XMFLOAT3> scales;
		wi::vector<float> opacities;
		wi::vector<XMFLOAT3> f_dc;
		wi::vector<float> f_rest; // number of floats depends on SH degree (L1:9, L2:24, L3:45)

		// Below this are non-serialized attributes:
		wi::primitive::AABB aabb_rest; // aabb without transformation
		wi::primitive::AABB aabb; // aabb with transformation
		XMFLOAT4X4 transform = wi::math::IDENTITY_MATRIX;
		XMFLOAT4X4 transform_inverse = wi::math::IDENTITY_MATRIX;
		wi::graphics::GPUBuffer splatBuffer;
		wi::graphics::GPUBuffer shBuffer;

		size_t GetSplatCount() const { return positions.size(); };
		int GetSphericalHarmonicsDegree() const;
		size_t GetMemorySizeCPU() const;
		size_t GetMemorySizeGPU() const;

		void CreateRenderData();

		void Update(const XMFLOAT4X4& matrix);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		static void Initialize();
	};

	// Managing sorting for all gaussian splat models:
	class GaussianSplatScene
	{
	public:
		size_t model_capacity = 0;
		size_t splat_capacity = 0;
		wi::graphics::GPUBuffer modelBuffer;
		wi::graphics::GPUBuffer indirectBuffer;
		wi::graphics::GPUBuffer sortedIndexBuffer;
		wi::graphics::GPUBuffer distanceBuffer;
		wi::graphics::GPUBuffer splatLookupBuffer;

		constexpr bool IsValid() const { return splat_capacity > 0; }

		void MakeReservations(const GaussianSplatModel* models, size_t model_count);
		void UpdateGPU(const GaussianSplatModel** models, size_t model_count, wi::graphics::CommandList cmd, const XMFLOAT4X4* viewmatrices, uint32_t camera_count = 1) const; // culling and sorting. Multiple cameras can be provided when rendering will be performed to multiple destinations
		void Draw(wi::graphics::CommandList cmd, uint32_t camera_count = 1) const; // will be drawn with culling and sorting based on previous call to UpdateGPU
		void Clear();
	};
}
