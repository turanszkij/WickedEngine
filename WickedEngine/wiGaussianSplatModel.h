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

	class GaussianSplatModel
	{
	public:
		wi::vector<XMFLOAT3> positions;
		wi::vector<XMFLOAT4> rotations;
		wi::vector<XMFLOAT3> scales;
		wi::vector<float> opacities;
		wi::vector<XMFLOAT3> f_dc;
		wi::vector<float> f_rest; // number of floats depends on SH degree

		// Below this are non-serialized attributes:
		wi::primitive::AABB aabb_rest; // aabb without trasformation
		wi::primitive::AABB aabb; // aabb with trasformation
		XMFLOAT4X4 transform = wi::math::IDENTITY_MATRIX;
		XMFLOAT4X4 transform_inverse = wi::math::IDENTITY_MATRIX;
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

		void Update(const XMFLOAT4X4& matrix);
		void UpdateGPU(const wi::scene::CameraComponent& camera, wi::graphics::CommandList cmd); // culling and sorting
		void Draw(wi::graphics::CommandList cmd); // will be drawn with culling and sorting based on previous call to UpdateGPU

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		static void Initialize();
	};
}
