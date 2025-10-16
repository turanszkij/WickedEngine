#pragma once
#include "CommonInclude.h"
#include "wiArchive.h"
#include "wiCanvas.h"
#include "wiAudio.h"
#include "wiVideo.h"
#include "wiEnums.h"
#include "wiOcean.h"
#include "wiPrimitive.h"
#include "shaders/ShaderInterop_Renderer.h"
#include "wiResourceManager.h"
#include "wiVector.h"
#include "wiRectPacker.h"
#include "wiUnorderedSet.h"
#include "wiBVH.h"
#include "wiPathQuery.h"
#include "wiAllocator.h"

namespace wi::scene
{

	struct NameComponent
	{
		std::string name;

		inline void operator=(const std::string& str) { name = str; }
		inline void operator=(std::string&& str) { name = std::move(str); }
		inline bool operator==(const std::string& str) const { return name.compare(str) == 0; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct LayerComponent
	{
		uint32_t layerMask = ~0u;

		// Non-serialized attributes:
		uint32_t propagationMask = ~0u; // This shouldn't be modified by user usually

		constexpr uint32_t GetLayerMask() const { return layerMask & propagationMask; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct alignas(16) TransformComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
		};

		XMFLOAT3 scale_local = XMFLOAT3(1, 1, 1);
		uint32_t _flags = DIRTY;
		XMFLOAT4 rotation_local = XMFLOAT4(0, 0, 0, 1);	// this is a quaternion
		XMFLOAT3 translation_local = XMFLOAT3(0, 0, 0);

		// Non-serialized attributes:
		float _padding = 0;

		// The world matrix can be computed from local scale, rotation, translation
		//	- by calling UpdateTransform()
		//	- or by calling SetDirty() and letting the TransformUpdateSystem handle the updating
		XMFLOAT4X4 world = wi::math::IDENTITY_MATRIX;

		constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		constexpr bool IsDirty() const { return _flags & DIRTY; }

		XMFLOAT3 GetPosition() const;
		XMFLOAT4 GetRotation() const;
		XMFLOAT3 GetScale() const;
		XMVECTOR GetPositionV() const;
		XMVECTOR GetRotationV() const;
		XMVECTOR GetScaleV() const;
		XMFLOAT3 GetForward() const;
		XMFLOAT3 GetUp() const;
		XMFLOAT3 GetRight() const;
		XMVECTOR GetForwardV() const;
		XMVECTOR GetUpV() const;
		XMVECTOR GetRightV() const;
		void GetPositionRotationScale(XMFLOAT3& position, XMFLOAT4& rotation, XMFLOAT3& scale) const;
		// Computes the local space matrix from scale, rotation, translation and returns it
		XMMATRIX GetLocalMatrix() const;
		// Returns the stored world matrix that was computed the last time UpdateTransform() was called
		XMMATRIX GetWorldMatrix() const { return XMLoadFloat4x4(&world); };
		// Applies the local space to the world space matrix. This overwrites world matrix
		void UpdateTransform();
		// Apply a parent transform relative to the local space. This overwrites world matrix
		void UpdateTransform_Parented(const TransformComponent& parent);
		// Apply the world matrix to the local space. This overwrites scale, rotation, translation
		void ApplyTransform();
		// Clears the local space. This overwrites scale, rotation, translation
		void ClearTransform();
		void Translate(const XMFLOAT3& value);
		void Translate(const XMVECTOR& value);
		void RotateRollPitchYaw(const XMFLOAT3& value);
		void RotateRollPitchYaw(const XMVECTOR& value);
		void Rotate(const XMFLOAT4& quaternion);
		void Rotate(const XMVECTOR& quaternion);
		void Scale(const XMFLOAT3& value);
		void Scale(const XMVECTOR& value);
		void MatrixTransform(const XMFLOAT4X4& matrix);
		void MatrixTransform(const XMMATRIX& matrix);
		void Lerp(const TransformComponent& a, const TransformComponent& b, float t);
		void CatmullRom(const TransformComponent& a, const TransformComponent& b, const TransformComponent& c, const TransformComponent& d, float t);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct alignas(16) HierarchyComponent
	{
		wi::ecs::Entity parentID = wi::ecs::INVALID_ENTITY;
		uint32_t layerMask_bind; // saved child layermask at the time of binding

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct alignas(16) MaterialComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			CAST_SHADOW = 1 << 1,
			_DEPRECATED_PLANAR_REFLECTION = 1 << 2,
			_DEPRECATED_WATER = 1 << 3,
			_DEPRECATED_FLIP_NORMALMAP = 1 << 4,
			USE_VERTEXCOLORS = 1 << 5,
			SPECULAR_GLOSSINESS_WORKFLOW = 1 << 6,
			OCCLUSION_PRIMARY = 1 << 7,
			OCCLUSION_SECONDARY = 1 << 8,
			USE_WIND = 1 << 9,
			DISABLE_RECEIVE_SHADOW = 1 << 10,
			DOUBLE_SIDED = 1 << 11,
			OUTLINE = 1 << 12,
			PREFER_UNCOMPRESSED_TEXTURES = 1 << 13,
			DISABLE_VERTEXAO = 1 << 14,
			DISABLE_TEXTURE_STREAMING = 1 << 15,
			COPLANAR_BLENDING = 1 << 16, // force transparent material draw in opaque pass (useful for coplanar polygons)
			DISABLE_CAPSULE_SHADOW = 1 << 17,
			INTERNAL = 1 << 18 // used only for internal purposes
		};
		uint32_t _flags = CAST_SHADOW;

		enum SHADERTYPE
		{
			SHADERTYPE_PBR,
			SHADERTYPE_PBR_PLANARREFLECTION,
			SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING,
			SHADERTYPE_PBR_ANISOTROPIC,
			SHADERTYPE_WATER,
			SHADERTYPE_CARTOON,
			SHADERTYPE_UNLIT,
			SHADERTYPE_PBR_CLOTH,
			SHADERTYPE_PBR_CLEARCOAT,
			SHADERTYPE_PBR_CLOTH_CLEARCOAT,
			SHADERTYPE_PBR_TERRAINBLENDED,
			SHADERTYPE_INTERIORMAPPING,
			SHADERTYPE_COUNT
		} shaderType = SHADERTYPE_PBR;
		static_assert(SHADERTYPE_COUNT == SHADERTYPE_BIN_COUNT, "These values must match!");

		inline static const wi::vector<std::string> shaderTypeDefines[] = {
			{}, // SHADERTYPE_PBR,
			{"PLANARREFLECTION"}, // SHADERTYPE_PBR_PLANARREFLECTION,
			{"PARALLAXOCCLUSIONMAPPING"}, // SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING,
			{"ANISOTROPIC"}, // SHADERTYPE_PBR_ANISOTROPIC,
			{"WATER"}, // SHADERTYPE_WATER,
			{"CARTOON"}, // SHADERTYPE_CARTOON,
			{"UNLIT"}, // SHADERTYPE_UNLIT,
			{"SHEEN"}, // SHADERTYPE_PBR_CLOTH,
			{"CLEARCOAT"}, // SHADERTYPE_PBR_CLEARCOAT,
			{"SHEEN", "CLEARCOAT"}, // SHADERTYPE_PBR_CLOTH_CLEARCOAT,
			{"TERRAINBLENDED"}, //SHADERTYPE_PBR_TERRAINBLENDED
			{"INTERIORMAPPING"}, //SHADERTYPE_INTERIORMAPPING
		};
		static_assert(SHADERTYPE_COUNT == arraysize(shaderTypeDefines), "These values must match!");

		wi::enums::STENCILREF engineStencilRef = wi::enums::STENCILREF_DEFAULT;
		wi::enums::BLENDMODE userBlendMode = wi::enums::BLENDMODE_OPAQUE;

		XMFLOAT4 baseColor = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 specularColor = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 emissiveColor = XMFLOAT4(1, 1, 1, 0);
		XMFLOAT4 subsurfaceScattering = XMFLOAT4(1, 1, 1, 0);
		XMFLOAT4 extinctionColor = XMFLOAT4(0, 0.9f, 1, 1);
		XMFLOAT4 texMulAdd = XMFLOAT4(1, 1, 0, 0); // dynamic multiplier (.xy) and addition (.zw) for UV coordinates
		float roughness = 0.2f;
		float reflectance = 0.02f;
		float metalness = 0.0f;
		float normalMapStrength = 1.0f;
		float parallaxOcclusionMapping = 0.0f;
		float displacementMapping = 0.0f;
		float refraction = 0.0f;
		float transmission = 0.0f;
		float alphaRef = 1.0f;
		float anisotropy_strength = 0;
		float anisotropy_rotation = 0; //radians, counter-clockwise
		float blend_with_terrain_height = 0;
		float cloak = 0;
		float chromatic_aberration = 0;
		float saturation = 1;
		float mesh_blend = 0;

		XMFLOAT4 sheenColor = XMFLOAT4(1, 1, 1, 1);
		float sheenRoughness = 0;
		float clearcoat = 0;
		float clearcoatRoughness = 0;

		uint8_t userStencilRef = 0;
		wi::graphics::ShadingRate shadingRate = wi::graphics::ShadingRate::RATE_1X1;

		XMFLOAT2 texAnimDirection = XMFLOAT2(0, 0);
		float texAnimFrameRate = 0.0f;
		float texAnimElapsedTime = 0.0f;

		float interiorMappingRotation = 0; // horizontal rotation in radians
		XMFLOAT3 interiorMappingScale = XMFLOAT3(1, 1, 1);
		XMFLOAT3 interiorMappingOffset = XMFLOAT3(0, 0, 0);

		enum TEXTURESLOT
		{
			BASECOLORMAP,
			NORMALMAP,
			SURFACEMAP,
			EMISSIVEMAP,
			DISPLACEMENTMAP,
			OCCLUSIONMAP,
			TRANSMISSIONMAP,
			SHEENCOLORMAP,
			SHEENROUGHNESSMAP,
			CLEARCOATMAP,
			CLEARCOATROUGHNESSMAP,
			CLEARCOATNORMALMAP,
			SPECULARMAP,
			ANISOTROPYMAP,
			TRANSPARENCYMAP,

			TEXTURESLOT_COUNT
		};
		struct TextureMap
		{
			std::string name;
			wi::Resource resource;
			uint32_t uvset = 0;
			const wi::graphics::GPUResource* GetGPUResource() const
			{
				if (!resource.IsValid() || !resource.GetTexture().IsValid())
					return nullptr;
				return &resource.GetTexture();
			}

			// Non-serialized attributes:
			float lod_clamp = 0;						// optional, can be used by texture streaming
			uint32_t virtual_anisotropy = 0;			// optional, can be used by texture streaming
			int sparse_residencymap_descriptor = -1;	// optional, can be used by texture streaming
			int sparse_feedbackmap_descriptor = -1;		// optional, can be used by texture streaming
		};
		TextureMap textures[TEXTURESLOT_COUNT];

		int customShaderID = -1;
		uint4 userdata = uint4(0, 0, 0, 0); // can be accessed by custom shader

		wi::ecs::Entity cameraSource = wi::ecs::INVALID_ENTITY; // take texture from camera render

		// Non-serialized attributes:
		uint32_t layerMask = ~0u;
		int sampler_descriptor = -1; // optional

		// User stencil value can be in range [0, 15]
		constexpr void SetUserStencilRef(uint8_t value) { userStencilRef = value & 0x0F; }
		uint32_t GetStencilRef() const;

		constexpr float GetOpacity() const { return baseColor.w; }
		constexpr float GetEmissiveStrength() const { return emissiveColor.w; }
		constexpr int GetCustomShaderID() const { return customShaderID; }

		constexpr bool HasPlanarReflection() const { return shaderType == SHADERTYPE_PBR_PLANARREFLECTION || shaderType == SHADERTYPE_WATER; }

		constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		constexpr bool IsDirty() const { return _flags & DIRTY; }

		constexpr void SetInternal(bool value = true) { if (value) { _flags |= INTERNAL; } else { _flags &= ~INTERNAL; } }
		constexpr bool IsInternal() const { return _flags & INTERNAL; }

		constexpr void SetCastShadow(bool value) { SetDirty(); if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		constexpr void SetReceiveShadow(bool value) { SetDirty(); if (value) { _flags &= ~DISABLE_RECEIVE_SHADOW; } else { _flags |= DISABLE_RECEIVE_SHADOW; } }
		constexpr void SetOcclusionEnabled_Primary(bool value) { SetDirty(); if (value) { _flags |= OCCLUSION_PRIMARY; } else { _flags &= ~OCCLUSION_PRIMARY; } }
		constexpr void SetOcclusionEnabled_Secondary(bool value) { SetDirty(); if (value) { _flags |= OCCLUSION_SECONDARY; } else { _flags &= ~OCCLUSION_SECONDARY; } }

		wi::enums::BLENDMODE GetBlendMode() const { if (userBlendMode == wi::enums::BLENDMODE_OPAQUE && (GetFilterMask() & wi::enums::FILTER_TRANSPARENT)) return wi::enums::BLENDMODE_ALPHA; else return userBlendMode; }

		constexpr bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		constexpr bool IsAlphaTestEnabled() const { return alphaRef <= 1.0f - 1.0f / 256.0f; }
		constexpr bool IsUsingVertexColors() const { return _flags & USE_VERTEXCOLORS; }
		constexpr bool IsUsingWind() const { return _flags & USE_WIND; }
		constexpr bool IsReceiveShadow() const { return (_flags & DISABLE_RECEIVE_SHADOW) == 0; }
		constexpr bool IsUsingSpecularGlossinessWorkflow() const { return _flags & SPECULAR_GLOSSINESS_WORKFLOW; }
		constexpr bool IsOcclusionEnabled_Primary() const { return _flags & OCCLUSION_PRIMARY; }
		constexpr bool IsOcclusionEnabled_Secondary() const { return _flags & OCCLUSION_SECONDARY; }
		constexpr bool IsCustomShader() const { return customShaderID >= 0; }
		constexpr bool IsDoubleSided() const { return _flags & DOUBLE_SIDED; }
		constexpr bool IsOutlineEnabled() const { return _flags & OUTLINE; }
		constexpr bool IsPreferUncompressedTexturesEnabled() const { return _flags & PREFER_UNCOMPRESSED_TEXTURES; }
		constexpr bool IsVertexAODisabled() const { return _flags & DISABLE_VERTEXAO; }
		constexpr bool IsTextureStreamingDisabled() const { return _flags & DISABLE_TEXTURE_STREAMING; }
		constexpr bool IsCoplanarBlending() const { return _flags & COPLANAR_BLENDING; }

		constexpr void SetBaseColor(const XMFLOAT4& value) { SetDirty(); baseColor = value; }
		constexpr void SetSpecularColor(const XMFLOAT4& value) { SetDirty(); specularColor = value; }
		constexpr void SetEmissiveColor(const XMFLOAT4& value) { SetDirty(); emissiveColor = value; }
		constexpr void SetRoughness(float value) { SetDirty(); roughness = value; }
		constexpr void SetReflectance(float value) { SetDirty(); reflectance = value; }
		constexpr void SetMetalness(float value) { SetDirty(); metalness = value; }
		constexpr void SetEmissiveStrength(float value) { SetDirty(); emissiveColor.w = value; }
		constexpr void SetSaturation(float value) { SetDirty(); saturation = value; }
		constexpr void SetTransmissionAmount(float value) { SetDirty(); transmission = value; }
		constexpr void SetCloakAmount(float value) { SetDirty(); cloak = value; }
		constexpr void SetChromaticAberrationAmount(float value) { SetDirty(); chromatic_aberration = value; }
		constexpr void SetRefractionAmount(float value) { SetDirty(); refraction = value; }
		constexpr void SetNormalMapStrength(float value) { SetDirty(); normalMapStrength = value; }
		constexpr void SetParallaxOcclusionMapping(float value) { SetDirty(); parallaxOcclusionMapping = value; }
		constexpr void SetDisplacementMapping(float value) { SetDirty(); displacementMapping = value; }
		constexpr void SetSubsurfaceScatteringColor(XMFLOAT3 value)
		{
			SetDirty();
			subsurfaceScattering.x = value.x;
			subsurfaceScattering.y = value.y;
			subsurfaceScattering.z = value.z;
		}
		constexpr void SetSubsurfaceScatteringAmount(float value) { SetDirty(); subsurfaceScattering.w = value; }
		constexpr void SetOpacity(float value) { SetDirty(); baseColor.w = value; }
		constexpr void SetAlphaRef(float value) { SetDirty();  alphaRef = value; }
		constexpr void SetUseVertexColors(bool value) { SetDirty(); if (value) { _flags |= USE_VERTEXCOLORS; } else { _flags &= ~USE_VERTEXCOLORS; } }
		constexpr void SetUseWind(bool value) { SetDirty(); if (value) { _flags |= USE_WIND; } else { _flags &= ~USE_WIND; } }
		constexpr void SetUseSpecularGlossinessWorkflow(bool value) { SetDirty(); if (value) { _flags |= SPECULAR_GLOSSINESS_WORKFLOW; } else { _flags &= ~SPECULAR_GLOSSINESS_WORKFLOW; } }
		constexpr void SetSheenColor(const XMFLOAT3& value)
		{
			sheenColor = XMFLOAT4(value.x, value.y, value.z, sheenColor.w);
			SetDirty();
		}
		constexpr void SetExtinctionColor(const XMFLOAT4& value)
		{
			extinctionColor = XMFLOAT4(value.x, value.y, value.z, value.w);
			SetDirty();
		}
		constexpr void SetSheenRoughness(float value) { sheenRoughness = value; SetDirty(); }
		constexpr void SetClearcoatFactor(float value) { clearcoat = value; SetDirty(); }
		constexpr void SetClearcoatRoughness(float value) { clearcoatRoughness = value; SetDirty(); }
		constexpr void SetBlendWithTerrainHeight(float value) { blend_with_terrain_height = value; SetDirty(); }
		constexpr void SetCustomShaderID(int id) { customShaderID = id; }
		constexpr void DisableCustomShader() { customShaderID = -1; }
		constexpr void SetDoubleSided(bool value = true) { if (value) { _flags |= DOUBLE_SIDED; } else { _flags &= ~DOUBLE_SIDED; } }
		constexpr void SetOutlineEnabled(bool value = true) { if (value) { _flags |= OUTLINE; } else { _flags &= ~OUTLINE; } }
		constexpr void SetVertexAODisabled(bool value = true) { if (value) { _flags |= DISABLE_VERTEXAO; } else { _flags &= ~DISABLE_VERTEXAO; } }
		constexpr void SetTextureStreamingDisabled(bool value = true) { if (value) { _flags |= DISABLE_TEXTURE_STREAMING; } else { _flags &= ~DISABLE_TEXTURE_STREAMING; } }
		constexpr void SetCoplanarBlending(bool value = true) { if (value) { _flags |= COPLANAR_BLENDING; } else { _flags &= ~COPLANAR_BLENDING; } }
		constexpr void SetInteriorMappingScale(XMFLOAT3 value)
		{
			SetDirty();
			interiorMappingScale.x = value.x;
			interiorMappingScale.y = value.y;
			interiorMappingScale.z = value.z;
		}
		constexpr void SetInteriorMappingOffset(XMFLOAT3 value)
		{
			SetDirty();
			interiorMappingOffset.x = value.x;
			interiorMappingOffset.y = value.y;
			interiorMappingOffset.z = value.z;
		}
		constexpr void SetInteriorMappingRotation(float value)
		{
			SetDirty();
			interiorMappingRotation = value;
		}

		constexpr bool IsCapsuleShadowDisabled() const { return _flags & DISABLE_CAPSULE_SHADOW; }
		constexpr void SetCapsuleShadowDisabled(bool value = true) { if (value) { _flags |= DISABLE_CAPSULE_SHADOW; } else { _flags &= ~DISABLE_CAPSULE_SHADOW; } }

		constexpr void SetMeshBlend(float value) { mesh_blend = value; SetDirty(); }
		constexpr float GetMeshBlend() const { return mesh_blend; }

		void SetPreferUncompressedTexturesEnabled(bool value = true) { if (value) { _flags |= PREFER_UNCOMPRESSED_TEXTURES; } else { _flags &= ~PREFER_UNCOMPRESSED_TEXTURES; } CreateRenderData(true); }

		// The MaterialComponent will be written to ShaderMaterial (a struct that is optimized for GPU use)
		void WriteShaderMaterial(ShaderMaterial* dest) const;
		void WriteShaderTextureSlot(ShaderMaterial* dest, int slot, int descriptor);

		// Retrieve the array of textures from the material
		void WriteTextures(const wi::graphics::GPUResource** dest, int count) const;

		// Returns the bitwise OR of all the wi::enums::FILTER flags applicable to this material
		uint32_t GetFilterMask() const;

		wi::resourcemanager::Flags GetTextureSlotResourceFlags(TEXTURESLOT slot);

		// Create texture resources for GPU
		void CreateRenderData(bool force_recreate = false);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct alignas(16) RigidBodyPhysicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DISABLE_DEACTIVATION = 1 << 0,
			KINEMATIC = 1 << 1,
			START_DEACTIVATED = 1 << 2,
			REFRESH_PARAMETERS_REQUEST = 1 << 3,
			CHARACTER_PHYSICS = 1 << 4,
		};
		uint32_t _flags = EMPTY;

		enum CollisionShape : uint32_t
		{
			BOX,
			SPHERE,
			CAPSULE,
			CONVEX_HULL,
			TRIANGLE_MESH,
			CYLINDER,
			HEIGHTFIELD,
		};
		CollisionShape shape = BOX;
		float mass = 1.0f; // Set to 0 to make body static
		float friction = 0.2f;
		float restitution = 0.1f;
		float damping_linear = 0.05f;
		float damping_angular = 0.05f;
		float buoyancy = 1.2f;
		XMFLOAT3 local_offset = XMFLOAT3(0, 0, 0);

		// This will force LOD level for rigid body if it is a TRIANGLE_MESH shape:
		//	The geometry for LOD level will be taken from MeshComponent.
		//	The physics object will need to be recreated for it to take effect.
		uint32_t mesh_lod = 0;

		struct BoxParams
		{
			XMFLOAT3 halfextents = XMFLOAT3(1, 1, 1);
		} box;
		struct SphereParams
		{
			float radius = 1;
		} sphere;
		struct CapsuleParams
		{
			float radius = 1;
			float height = 1;
		} capsule; // also cylinder params

		// Vehicle physics will be enabled when Vehicle.type != None
		struct Vehicle
		{
			enum class Type
			{
				None,
				Car,
				Motorcycle,
			} type = Type::None;

			enum class CollisionMode
			{
				Ray,
				Sphere,
				Cylinder,
			} collision_mode = CollisionMode::Ray;

			float chassis_half_width = 0.9f;
			float chassis_half_height = 0.2f;
			float chassis_half_length = 2.0f;
			float front_wheel_offset = 0.0f;
			float rear_wheel_offset = 0.0f;
			float wheel_radius = 0.3f;
			float wheel_width = 0.1f;
			float max_engine_torque = 500.0f;
			float clutch_strength = 10.0f;
			float max_roll_angle = wi::math::DegreesToRadians(60.0f);
			float max_steering_angle = wi::math::DegreesToRadians(30.0f);

			struct Suspension
			{
				float min_length = 0.3f;
				float max_length = 0.5f;
				float frequency = 1.5f;
				float damping = 0.5f;
			};

			Suspension front_suspension;
			Suspension rear_suspension;

			struct Car
			{
				bool four_wheel_drive = false;
			} car;

			struct Motorcycle
			{
				float front_suspension_angle = wi::math::DegreesToRadians(30.0f);
				float front_brake_torque = 500.0f;
				float rear_brake_torque = 250.0f;

				// Non-serialized attributes:
				bool lean_control = true; // true: avoids fall over to the side
			} motorcycle;

			// These can be specified to drive wheel animation from physics engine state:
			wi::ecs::Entity wheel_entity_front_left = wi::ecs::INVALID_ENTITY;	// car, motorcycle
			wi::ecs::Entity wheel_entity_front_right = wi::ecs::INVALID_ENTITY;	// car
			wi::ecs::Entity wheel_entity_rear_left = wi::ecs::INVALID_ENTITY;	// car, motorcycle
			wi::ecs::Entity wheel_entity_rear_right = wi::ecs::INVALID_ENTITY;	// car

		} vehicle;

		struct Character
		{
			float maxSlopeAngle = wi::math::DegreesToRadians(50.0f);	// Maximum angle of slope that character can still walk on (radians).
			float gravityFactor = 1.0f;
		} character;

		// Non-serialized attributes:
		std::shared_ptr<void> physicsobject = nullptr; // You can set to null to recreate the physics object the next time phsyics system will be running.

		constexpr void SetDisableDeactivation(bool value) { if (value) { _flags |= DISABLE_DEACTIVATION; } else { _flags &= ~DISABLE_DEACTIVATION; } }
		constexpr void SetKinematic(bool value) { if (value) { _flags |= KINEMATIC; } else { _flags &= ~KINEMATIC; } }
		constexpr void SetStartDeactivated(bool value) { if (value) { _flags |= START_DEACTIVATED; } else { _flags &= ~START_DEACTIVATED; } }

		constexpr bool IsVehicle() const { return vehicle.type != Vehicle::Type::None; }
		constexpr bool IsCar() const { return vehicle.type == Vehicle::Type::Car; }
		constexpr bool IsMotorcycle() const { return vehicle.type == Vehicle::Type::Motorcycle; }
		constexpr bool IsDisableDeactivation() const { return _flags & DISABLE_DEACTIVATION; }
		constexpr bool IsKinematic() const { return _flags & KINEMATIC; }
		constexpr bool IsStartDeactivated() const { return _flags & START_DEACTIVATED; }

		constexpr void SetRefreshParametersNeeded(bool value = true) { if (value) { _flags |= REFRESH_PARAMETERS_REQUEST; } else { _flags &= ~REFRESH_PARAMETERS_REQUEST; } }
		constexpr bool IsRefreshParametersNeeded() const { return _flags & REFRESH_PARAMETERS_REQUEST; }

		constexpr void SetCharacterPhysics(bool value = true) { if (value) { _flags |= CHARACTER_PHYSICS; } else { _flags &= ~CHARACTER_PHYSICS; } }
		constexpr bool IsCharacterPhysics() const { return _flags & CHARACTER_PHYSICS; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct PhysicsConstraintComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			REFRESH_PARAMETERS_REQUEST = 1 << 0,
			DISABLE_SELF_COLLISION = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		// Note: the constraint axes are taken from the TransformComponent on the constraint's entity
		//	RIGHT axis means X axis in the default orientation
		//	UP axis means Y axis in the default orientation
		//
		//	The constraints are created in world space from the current TransformComponent orientation
		//	To issue recreation of the constraint, reset the physicsobject pointer of this structure
		//	To only refresh cosntraint settings without recreating the constraint, use SetRefreshParametersNeeded(true)
		enum class Type
		{
			Fixed,		// fixed in place completely
			Point,		// fixed to a point but can rotate around it
			Distance,	// point constraint within specified distance
			Hinge,		// rotation around a point on the UP axis of the contraint transform
			Cone,		// constrain to a cone shape specified by the cone angle (cone axis: UP)
			SixDOF,		// manual specification of axes movement and rotation limits
			SwingTwist,	// cone (UP axis) + rotational limits
			Slider,		// constrain on the RIGHT axis between limits
		} type = Type::Fixed;

		wi::ecs::Entity bodyA = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity bodyB = wi::ecs::INVALID_ENTITY;

		struct DistanceConstraintSettings
		{
			float min_distance = 0;
			float max_distance = 0;
		} distance_constraint;

		struct HingeConstraintSettings
		{
			float min_angle = -XM_PI;	// radians
			float max_angle = XM_PI;	// radians
			float target_angular_velocity = 0;	// motor
		} hinge_constraint;

		struct ConeConstraintSettings
		{
			float half_cone_angle = 0;	// radians
		} cone_constraint;

		struct SixDOFConstraintSettings
		{
			XMFLOAT3 minTranslationAxes = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			XMFLOAT3 maxTranslationAxes = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
			XMFLOAT3 minRotationAxes = XMFLOAT3(-XM_PI, -XM_PI, -XM_PI);
			XMFLOAT3 maxRotationAxes = XMFLOAT3(XM_PI, XM_PI, XM_PI);

			void SetFixedX() { minTranslationAxes.x = FLT_MAX; maxTranslationAxes.x = -FLT_MAX; }
			void SetFreeX() { minTranslationAxes.x = -FLT_MAX; maxTranslationAxes.x = FLT_MAX; }
			void SetFixedY() { minTranslationAxes.y = FLT_MAX; maxTranslationAxes.y = -FLT_MAX; }
			void SetFreeY() { minTranslationAxes.y = -FLT_MAX; maxTranslationAxes.y = FLT_MAX; }
			void SetFixedZ() { minTranslationAxes.z = FLT_MAX; maxTranslationAxes.z = -FLT_MAX; }
			void SetFreeZ() { minTranslationAxes.z = -FLT_MAX; maxTranslationAxes.z = FLT_MAX; }

			void SetFixedRotationX() { minRotationAxes.x = XM_PI; maxRotationAxes.x = -XM_PI; }
			void SetFreeRotationX() { minRotationAxes.x = -XM_PI; maxRotationAxes.x = XM_PI; }
			void SetFixedRotationY() { minRotationAxes.y = XM_PI; maxRotationAxes.y = -XM_PI; }
			void SetFreeRotationY() { minRotationAxes.y = -XM_PI; maxRotationAxes.y = XM_PI; }
			void SetFixedRotationZ() { minRotationAxes.z = XM_PI; maxRotationAxes.z = -XM_PI; }
			void SetFreeRotationZ() { minRotationAxes.z = -XM_PI; maxRotationAxes.z = XM_PI; }
		} six_dof;

		struct SwingTwistConstraintSettings
		{
			float normal_half_cone_angle = 0;	// radians
			float plane_half_cone_angle = 0;	// radians
			float min_twist_angle = 0;			// radians [-PI, PI]
			float max_twist_angle = 0;			// radians [-PI, PI]
		} swing_twist;

		struct SliderConstraintSettings
		{
			float min_limit = -FLT_MAX;
			float max_limit = FLT_MAX;
			float target_velocity = 0;	// motor
			float max_force = 0; // N
		} slider_constraint;

		float break_distance = FLT_MAX; // how much the constraint is allowed to be exerted before breaking, calculated as relative distance

		// Non-serialized attributes:
		std::shared_ptr<void> physicsobject = nullptr; // You can set to null to recreate the physics object the next time phsyics system will be running.

		// Request refreshing of constraint settings without recreating the constraint
		constexpr void SetRefreshParametersNeeded(bool value = true) { if (value) { _flags |= REFRESH_PARAMETERS_REQUEST; } else { _flags &= ~REFRESH_PARAMETERS_REQUEST; } }
		constexpr bool IsRefreshParametersNeeded() const { return _flags & REFRESH_PARAMETERS_REQUEST; }

		// Enable/disable collision between the two bodies that this constraint targets
		constexpr void SetDisableSelfCollision(bool value = true) { if (value) { _flags |= DISABLE_SELF_COLLISION; } else { _flags &= ~DISABLE_SELF_COLLISION; } }
		constexpr bool IsDisableSelfCollision() const { return _flags & DISABLE_SELF_COLLISION; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct alignas(16) MeshComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			RENDERABLE = 1 << 0,
			DOUBLE_SIDED = 1 << 1,
			DYNAMIC = 1 << 2,
			_DEPRECATED_TERRAIN = 1 << 3,
			_DEPRECATED_DIRTY_MORPH = 1 << 4,
			_DEPRECATED_DIRTY_BINDLESS = 1 << 5,
			TLAS_FORCE_DOUBLE_SIDED = 1 << 6,
			DOUBLE_SIDED_SHADOW = 1 << 7,
			BVH_ENABLED = 1 << 8,
			QUANTIZED_POSITIONS_DISABLED = 1 << 9,
		};
		// *uint32_t _flags is moved down for better struct padding...

		wi::vector<XMFLOAT3> vertex_positions;
		wi::vector<XMFLOAT3> vertex_normals;
		wi::vector<XMFLOAT4> vertex_tangents;
		wi::vector<XMFLOAT2> vertex_uvset_0;
		wi::vector<XMFLOAT2> vertex_uvset_1;
		wi::vector<XMUINT4> vertex_boneindices;
		wi::vector<XMFLOAT4> vertex_boneweights;
		wi::vector<XMUINT4> vertex_boneindices2;
		wi::vector<XMFLOAT4> vertex_boneweights2;
		wi::vector<XMFLOAT2> vertex_atlas;
		wi::vector<uint32_t> vertex_colors;
		wi::vector<uint8_t> vertex_windweights;
		wi::vector<uint32_t> indices;

		enum MESH_SUBSET_FLAGS
		{
			MESH_SUBSET_DOUBLESIDED = 1 << 0,
		};

		struct MeshSubset
		{
			std::string surfaceName; // custom identifier for user, not used by engine
			wi::ecs::Entity materialID = wi::ecs::INVALID_ENTITY;
			uint32_t indexOffset = 0;
			uint32_t indexCount = 0;

			// Non-serialized attributes:
			uint32_t materialIndex = 0;
			uint32_t flags = 0;

			constexpr bool IsDoubleSided() const { return flags & MESH_SUBSET_DOUBLESIDED; }
		};
		wi::vector<MeshSubset> subsets;

		wi::ecs::Entity armatureID = wi::ecs::INVALID_ENTITY;

		float tessellationFactor = 0.0f;
		uint32_t subsets_per_lod = 0; // this needs to be specified if there are multiple LOD levels

		struct MorphTarget
		{
			wi::vector<XMFLOAT3> vertex_positions;
			wi::vector<XMFLOAT3> vertex_normals;
			wi::vector<uint32_t> sparse_indices_positions; // optional, these can be used to target vertices indirectly
			wi::vector<uint32_t> sparse_indices_normals; // optional, these can be used to target vertices indirectly
			float weight = 0;

			// Non-serialized attributes:
			uint64_t offset_pos = ~0ull;
			uint64_t offset_nor = ~0ull;
		};
		wi::vector<MorphTarget> morph_targets;

		// Non-serialized attributes:
		wi::primitive::AABB aabb;
		wi::graphics::GPUBuffer generalBuffer; // index buffer + all static vertex buffers
		wi::graphics::GPUBuffer streamoutBuffer; // all dynamic vertex buffers
		wi::allocator::PageAllocator::Allocation generalBufferOffsetAllocation;
		wi::graphics::GPUBuffer generalBufferOffsetAllocationAlias;
		struct BufferView
		{
			uint64_t offset = ~0ull;
			uint64_t size = 0ull;
			int subresource_srv = -1;
			int descriptor_srv = -1;
			int subresource_uav = -1;
			int descriptor_uav = -1;

			constexpr bool IsValid() const
			{
				return offset != ~0ull;
			}
		};
		BufferView ib_provoke;
		BufferView ib_reorder;
		BufferView ib;
		BufferView vb_pos_wind;
		BufferView vb_nor;
		BufferView vb_tan;
		BufferView vb_uvs;
		BufferView vb_atl;
		BufferView vb_col;
		BufferView vb_bon;
		BufferView vb_mor;
		BufferView vb_clu;
		BufferView vb_bou;
		BufferView so_pos;
		BufferView so_nor;
		BufferView so_tan;
		BufferView so_pre;
		uint32_t geometryOffset = 0;
		uint32_t meshletCount = 0;
		uint32_t active_morph_count = 0;
		uint32_t morphGPUOffset = 0;
		XMFLOAT2 uv_range_min = XMFLOAT2(0, 0);
		XMFLOAT2 uv_range_max = XMFLOAT2(1, 1);

		wi::vector<wi::graphics::RaytracingAccelerationStructure> BLASes; // one BLAS per LOD

		wi::vector<wi::primitive::AABB> bvh_leaf_aabbs;
		wi::BVH bvh;

		struct SubsetClusterRange
		{
			uint32_t clusterOffset = 0;
			uint32_t clusterCount = 0;
		};
		wi::vector<SubsetClusterRange> cluster_ranges;

		RigidBodyPhysicsComponent precomputed_rigidbody_physics_shape; // you can precompute a physics shape here if you need without using a real rigid body component yet

		uint32_t _flags = RENDERABLE; // *this is serialized but put here for better struct padding

		enum BLAS_STATE
		{
			BLAS_STATE_NEEDS_REBUILD,
			BLAS_STATE_NEEDS_REFIT,
			BLAS_STATE_COMPLETE,
		};
		mutable BLAS_STATE BLAS_state = BLAS_STATE_NEEDS_REBUILD;

		constexpr void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		constexpr void SetDoubleSided(bool value) { if (value) { _flags |= DOUBLE_SIDED; } else { _flags &= ~DOUBLE_SIDED; } }
		constexpr void SetDoubleSidedShadow(bool value) { if (value) { _flags |= DOUBLE_SIDED_SHADOW; } else { _flags &= ~DOUBLE_SIDED_SHADOW; } }
		constexpr void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }

		// Enable disable CPU-side BVH acceleration structure
		//	true: BVH will be built immediately if it doesn't exist yet
		//	false: BVH will be deleted immediately if it exists
		void SetBVHEnabled(bool value) { if (value) { _flags |= BVH_ENABLED; if (!bvh.IsValid()) { BuildBVH(); } } else { _flags &= ~BVH_ENABLED; bvh = {}; bvh_leaf_aabbs.clear(); } }

		// Disable quantization of position GPU data. You can use this if you notice inaccuracy in positions.
		//	This should be enabled for connecting meshes like terrain chunks if their AABB is not consistent with each other
		constexpr void SetQuantizedPositionsDisabled(bool value) { if (value) { _flags |= QUANTIZED_POSITIONS_DISABLED; } else { _flags &= ~QUANTIZED_POSITIONS_DISABLED; } }

		constexpr bool IsRenderable() const { return _flags & RENDERABLE; }
		constexpr bool IsDoubleSided() const { return _flags & DOUBLE_SIDED; }
		constexpr bool IsDoubleSidedShadow() const { return _flags & DOUBLE_SIDED_SHADOW; }
		constexpr bool IsDynamic() const { return _flags & DYNAMIC; }
		constexpr bool IsBVHEnabled() const { return _flags & BVH_ENABLED; }
		constexpr bool IsQuantizedPositionsDisabled() const { return _flags & QUANTIZED_POSITIONS_DISABLED; }

		constexpr float GetTessellationFactor() const { return tessellationFactor; }
		constexpr bool IsSkinned() const { return armatureID != wi::ecs::INVALID_ENTITY; }

		inline wi::graphics::IndexBufferFormat GetIndexFormat() const { return wi::graphics::GetIndexBufferFormat((uint32_t)vertex_positions.size()); }
		inline size_t GetIndexStride() const { return GetIndexFormat() == wi::graphics::IndexBufferFormat::UINT32 ? sizeof(uint32_t) : sizeof(uint16_t); }

		inline wi::graphics::IndexBufferFormat GetProvokingIndexFormat() const { return wi::graphics::GetIndexBufferFormat((uint32_t)indices.size()); }
		inline size_t GetProvokingIndexStride() const { return GetProvokingIndexFormat() == wi::graphics::IndexBufferFormat::UINT32 ? sizeof(uint32_t) : sizeof(uint16_t); }

		uint32_t GetLODCount() const { return subsets_per_lod == 0 ? 1 : ((uint32_t)subsets.size() / subsets_per_lod); }
		void GetLODSubsetRange(uint32_t lod, uint32_t& first_subset, uint32_t& last_subset) const
		{
			first_subset = 0;
			last_subset = (uint32_t)subsets.size();
			if (subsets_per_lod > 0)
			{
				lod = std::min(lod, GetLODCount() - 1);
				first_subset = subsets_per_lod * lod;
				last_subset = first_subset + subsets_per_lod;
			}
		}

		size_t GetClusterCount() const;

		// Creates a new subset as a combination of the subsets of the first LOD, returns its index. This works if there are multiple LODs which are also contained in subsets array
		size_t CreateSubset();

		// Deletes a subset from all LODs
		void DeleteSubset(uint32_t subsetIndex);

		// Set a material to a subset in all LODs
		void SetSubsetMaterial(uint32_t subsetIndex, wi::ecs::Entity entity);

		// Get the material from the subsetIndex in the first LOD
		wi::ecs::Entity GetSubsetMaterial(uint32_t subsetIndex);

		// Deletes all GPU resources
		void DeleteRenderData();

		// Recreates GPU resources for index/vertex buffers
		void CreateRenderData();
		void CreateStreamoutRenderData();
		void CreateRaytracingRenderData();

		// Rebuilds CPU-side BVH acceleration structure
		void BuildBVH();

		size_t GetMemoryUsageCPU() const;
		size_t GetMemoryUsageGPU() const;
		size_t GetMemoryUsageBVH() const;

		enum COMPUTE_NORMALS
		{
			COMPUTE_NORMALS_HARD,		// hard face normals, can result in additional vertices generated
			COMPUTE_NORMALS_SMOOTH,		// smooth per vertex normals, this can remove/simplify geometry, but slow
			COMPUTE_NORMALS_SMOOTH_FAST	// average normals, vertex count will be unchanged, fast
		};
		void ComputeNormals(COMPUTE_NORMALS compute);
		void FlipCulling();
		void FlipNormals();
		void Recenter();
		void RecenterToBottom();
		wi::primitive::Sphere GetBoundingSphere() const;

		uint32_t GetBoneInfluenceDiv4() const
		{
			uint32_t influence_div4 = 0;
			if (!vertex_boneindices.empty())
			{
				influence_div4++;
			}
			if (!vertex_boneindices2.empty())
			{
				influence_div4++;
			}
			return influence_div4;
		}

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		struct Vertex_POS16
		{
			uint16_t x = 0;
			uint16_t y = 0;
			uint16_t z = 0;
			uint16_t w = 0;

			constexpr void FromFULL(const wi::primitive::AABB& aabb, XMFLOAT3 pos, uint8_t wind)
			{
				pos = wi::math::InverseLerp(aabb._min, aabb._max, pos); // UNORM remap
				x = uint16_t(pos.x * 65535.0f);
				y = uint16_t(pos.y * 65535.0f);
				z = uint16_t(pos.z * 65535.0f);
				w = uint16_t((float(wind) / 255.0f) * 65535.0f);
			}
			inline XMVECTOR LoadPOS(const wi::primitive::AABB& aabb) const
			{
				XMFLOAT3 v = GetPOS(aabb);
				return XMLoadFloat3(&v);
			}
			constexpr XMFLOAT3 GetPOS(const wi::primitive::AABB& aabb) const
			{
				XMFLOAT3 v = XMFLOAT3(
					float(x) / 65535.0f,
					float(y) / 65535.0f,
					float(z) / 65535.0f
				);
				return wi::math::Lerp(aabb._min, aabb._max, v);
			}
			constexpr uint8_t GetWind() const
			{
				return uint8_t((float(w) / 65535.0f) * 255);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R16G16B16A16_UNORM;
		};
		struct Vertex_POS32
		{
			float x = 0;
			float y = 0;
			float z = 0;

			constexpr void FromFULL(const XMFLOAT3& pos)
			{
				x = pos.x;
				y = pos.y;
				z = pos.z;
			}
			inline XMVECTOR LoadPOS() const
			{
				return XMVectorSet(x, y, z, 1);
			}
			constexpr XMFLOAT3 GetPOS() const
			{
				return XMFLOAT3(x, y, z);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R32G32B32_FLOAT;
		};
		struct Vertex_POS32W
		{
			float x = 0;
			float y = 0;
			float z = 0;
			float w = 0;

			constexpr void FromFULL(const XMFLOAT3& pos, uint8_t wind)
			{
				x = pos.x;
				y = pos.y;
				z = pos.z;
				w = float(wind) / 255.0f;
			}
			inline XMVECTOR LoadPOS() const
			{
				return XMVectorSet(x, y, z, 1);
			}
			constexpr XMFLOAT3 GetPOS() const
			{
				return XMFLOAT3(x, y, z);
			}
			constexpr uint8_t GetWind() const
			{
				return uint8_t(w * 255);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R32G32B32A32_FLOAT;
		};
		wi::graphics::Format position_format = Vertex_POS16::FORMAT; // CreateRenderData() will choose the appropriate format

		struct Vertex_TEX
		{
			uint16_t x = 0;
			uint16_t y = 0;

			constexpr void FromFULL(const XMFLOAT2& uv, const XMFLOAT2& uv_range_min = XMFLOAT2(0, 0), const XMFLOAT2& uv_range_max = XMFLOAT2(1, 1))
			{
				x = uint16_t(wi::math::InverseLerp(uv_range_min.x, uv_range_max.x, uv.x) * 65535.0f);
				y = uint16_t(wi::math::InverseLerp(uv_range_min.y, uv_range_max.y, uv.y) * 65535.0f);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R16G16_UNORM;
		};
		struct Vertex_TEX32
		{
			float x = 0;
			float y = 0;

			constexpr void FromFULL(const XMFLOAT2& uv, const XMFLOAT2& uv_range_min = XMFLOAT2(0, 0), const XMFLOAT2& uv_range_max = XMFLOAT2(1, 1))
			{
				x = wi::math::InverseLerp(uv_range_min.x, uv_range_max.x, uv.x);
				y = wi::math::InverseLerp(uv_range_min.y, uv_range_max.y, uv.y);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R32G32_FLOAT;
		};
		struct Vertex_UVS
		{
			Vertex_TEX uv0;
			Vertex_TEX uv1;
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R16G16B16A16_UNORM;
		};
		struct Vertex_UVS32
		{
			Vertex_TEX32 uv0;
			Vertex_TEX32 uv1;
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R32G32B32A32_FLOAT;
		};
		struct Vertex_BON
		{
			XMUINT4 packed = XMUINT4(0, 0, 0, 0);

			constexpr void FromFULL(const XMUINT4& boneIndices, const XMFLOAT4& boneWeights)
			{
				// Note:
				//	- Indices are packed at 20 bits which allow indexing >1 million bones per mesh
				//	- Weights are packed at 12 bits which allow 4096 distinct values, this was tweaked to
				//		retain good precision with a high bone count stanford bunny soft body simulation where regular 8 bit weights was not enough
				packed.x = (boneIndices.x & 0xFFFFF) | ((uint32_t(boneWeights.x * 4095) & 0xFFF) << 20u);
				packed.y = (boneIndices.y & 0xFFFFF) | ((uint32_t(boneWeights.y * 4095) & 0xFFF) << 20u);
				packed.z = (boneIndices.z & 0xFFFFF) | ((uint32_t(boneWeights.z * 4095) & 0xFFF) << 20u);
				packed.w = (boneIndices.w & 0xFFFFF) | ((uint32_t(boneWeights.w * 4095) & 0xFFF) << 20u);
			}
			constexpr XMUINT4 GetInd_FULL() const
			{
				return XMUINT4(packed.x & 0xFFFFF, packed.y & 0xFFFFF, packed.z & 0xFFFFF, packed.w & 0xFFFFF);
			}
			constexpr XMFLOAT4 GetWei_FULL() const
			{
				return XMFLOAT4(
					float(packed.x >> 20u) / 4095.0f,
					float(packed.y >> 20u) / 4095.0f,
					float(packed.z >> 20u) / 4095.0f,
					float(packed.w >> 20u) / 4095.0f
				);
			}
		};
		struct Vertex_COL
		{
			uint32_t color = 0;
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R8G8B8A8_UNORM;
		};
		struct Vertex_NOR
		{
			int8_t x = 0;
			int8_t y = 0;
			int8_t z = 0;
			int8_t w = 0;

			void FromFULL(const XMFLOAT3& nor)
			{
				XMVECTOR N = XMLoadFloat3(&nor);
				N = XMVector3Normalize(N);
				XMFLOAT3 n;
				XMStoreFloat3(&n, N);

				x = int8_t(n.x * 127.5f);
				y = int8_t(n.y * 127.5f);
				z = int8_t(n.z * 127.5f);
				w = 0;
			}
			inline XMFLOAT3 GetNOR() const
			{
				return XMFLOAT3(
					float(x) / 127.5f,
					float(y) / 127.5f,
					float(z) / 127.5f
				);
			}
			inline XMVECTOR LoadNOR() const
			{
				return XMVectorSet(
					float(x) / 127.5f,
					float(y) / 127.5f,
					float(z) / 127.5f,
					0
				);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R8G8B8A8_SNORM;
		};
		struct Vertex_TAN
		{
			int8_t x = 0;
			int8_t y = 0;
			int8_t z = 0;
			int8_t w = 0;

			void FromFULL(const XMFLOAT4& tan)
			{
				XMVECTOR T = XMLoadFloat4(&tan);
				T = XMVector3Normalize(T);
				XMFLOAT4 t;
				XMStoreFloat4(&t, T);
				t.w = tan.w;

				x = int8_t(t.x * 127.5f);
				y = int8_t(t.y * 127.5f);
				z = int8_t(t.z * 127.5f);
				w = int8_t(t.w * 127.5f);
			}
			inline XMFLOAT4 GetTAN() const
			{
				return XMFLOAT4(
					float(x) / 127.5f,
					float(y) / 127.5f,
					float(z) / 127.5f,
					float(w) / 127.5f
				);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R8G8B8A8_SNORM;
		};

	};

	struct alignas(16) ImpostorComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
		};
		uint32_t _flags = DIRTY;

		float swapInDistance = 100.0f;

		// Non-serialized attributes:
		mutable bool render_dirty = false;
		int textureIndex = -1;

		constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		constexpr bool IsDirty() const { return _flags & DIRTY; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct alignas(16) ObjectComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			RENDERABLE = 1 << 0,
			CAST_SHADOW = 1 << 1,
			DYNAMIC = 1 << 2,
			_DEPRECATED_IMPOSTOR_PLACEMENT = 1 << 3,
			REQUEST_PLANAR_REFLECTION = 1 << 4,
			LIGHTMAP_RENDER_REQUEST = 1 << 5,
			LIGHTMAP_DISABLE_BLOCK_COMPRESSION = 1 << 6,
			FOREGROUND = 1 << 7,
			NOT_VISIBLE_IN_MAIN_CAMERA = 1 << 8,
			NOT_VISIBLE_IN_REFLECTIONS = 1 << 9,
			WETMAP_ENABLED = 1 << 10,
		};
		uint32_t _flags = RENDERABLE | CAST_SHADOW;

		uint8_t userStencilRef = 0;
		wi::ecs::Entity meshID = wi::ecs::INVALID_ENTITY;
		uint32_t cascadeMask = 0; // which shadow cascades to skip from lowest detail to highest detail (0: skip none, 1: skip first, etc...)
		uint32_t filterMask = 0;
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 emissiveColor = XMFLOAT4(1, 1, 1, 1);
		float lod_bias = 0;
		float draw_distance = std::numeric_limits<float>::max(); // object will begin to fade out at this distance to camera
		uint32_t lightmapWidth = 0;
		uint32_t lightmapHeight = 0;
		wi::vector<uint8_t> lightmapTextureData;
		wi::vector<uint8_t> vertex_ao;
		float alphaRef = 1;
		XMFLOAT4 rimHighlightColor = XMFLOAT4(1, 1, 1, 0);
		float rimHighlightFalloff = 8;
		uint32_t sort_priority = 0; // increase to draw earlier (currently 4 bits will be used)

		// Non-serialized attributes:
		uint32_t filterMaskDynamic = 0;

		wi::graphics::Texture lightmap_render;
		wi::graphics::Texture lightmap;
		mutable uint32_t lightmapIterationCount = 0;
		int vb_ao_srv = -1;
		wi::graphics::GPUBuffer vb_ao;
		wi::graphics::GPUBuffer wetmap;

		XMFLOAT3 center = XMFLOAT3(0, 0, 0);
		float radius = 0;
		float fadeDistance = 0;

		uint16_t lod = 0;
		mutable bool wetmap_cleared = false;
		bool mesh_blend_required = false;

		// these will only be valid for a single frame:
		uint32_t mesh_index = ~0u;
		uint32_t sort_bits = 0;

		constexpr void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		constexpr void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		constexpr void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }
		constexpr void SetRequestPlanarReflection(bool value) { if (value) { _flags |= REQUEST_PLANAR_REFLECTION; } else { _flags &= ~REQUEST_PLANAR_REFLECTION; } }
		constexpr void SetLightmapRenderRequest(bool value) { if (value) { _flags |= LIGHTMAP_RENDER_REQUEST; } else { _flags &= ~LIGHTMAP_RENDER_REQUEST; } }
		constexpr void SetLightmapDisableBlockCompression(bool value) { if (value) { _flags |= LIGHTMAP_DISABLE_BLOCK_COMPRESSION; } else { _flags &= ~LIGHTMAP_DISABLE_BLOCK_COMPRESSION; } }
		// Foreground object will be rendered in front of regular objects
		constexpr void SetForeground(bool value) { if (value) { _flags |= FOREGROUND; } else { _flags &= ~FOREGROUND; } }
		// With this you can disable object rendering for main camera (DRAWSCENE_MAINCAMERA)
		constexpr void SetNotVisibleInMainCamera(bool value) { if (value) { _flags |= NOT_VISIBLE_IN_MAIN_CAMERA; } else { _flags &= ~NOT_VISIBLE_IN_MAIN_CAMERA; } }
		// With this you can disable object rendering for reflections
		constexpr void SetNotVisibleInReflections(bool value) { if (value) { _flags |= NOT_VISIBLE_IN_REFLECTIONS; } else { _flags &= ~NOT_VISIBLE_IN_REFLECTIONS; } }
		constexpr void SetWetmapEnabled(bool value) { if (value) { _flags |= WETMAP_ENABLED; } else { _flags &= ~WETMAP_ENABLED; } }

		constexpr bool IsRenderable() const { return (_flags & RENDERABLE) && (GetTransparency() < 0.99f); }
		constexpr bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		constexpr bool IsDynamic() const { return _flags & DYNAMIC; }
		constexpr bool IsRequestPlanarReflection() const { return _flags & REQUEST_PLANAR_REFLECTION; }
		constexpr bool IsLightmapRenderRequested() const { return _flags & LIGHTMAP_RENDER_REQUEST; }
		constexpr bool IsLightmapDisableBlockCompression() const { return _flags & LIGHTMAP_DISABLE_BLOCK_COMPRESSION; }
		constexpr bool IsForeground() const { return _flags & FOREGROUND; }
		constexpr bool IsNotVisibleInMainCamera() const { return _flags & NOT_VISIBLE_IN_MAIN_CAMERA; }
		constexpr bool IsNotVisibleInReflections() const { return _flags & NOT_VISIBLE_IN_REFLECTIONS; }
		constexpr bool IsWetmapEnabled() const { return _flags & WETMAP_ENABLED; }

		constexpr float GetTransparency() const { return 1 - color.w; }
		constexpr uint32_t GetFilterMask() const { return filterMask | filterMaskDynamic; }

		// User stencil value can be in range [0, 15]
		//	Values greater than 0 can be used to override userStencilRef of MaterialComponent
		constexpr void SetUserStencilRef(uint8_t value) { userStencilRef = value & 0x0F; }

		void ClearLightmap();
		void SaveLightmap(); // not thread safe if LIGHTMAP_BLOCK_COMPRESSION is enabled!
		void CompressLightmap(); // not thread safe if LIGHTMAP_BLOCK_COMPRESSION is enabled!

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		void CreateRenderData();
		void DeleteRenderData();
		struct Vertex_AO
		{
			uint8_t value = 0;
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R8_UNORM;
		};
	};

	struct alignas(16) SoftBodyPhysicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			SAFE_TO_REGISTER = 1 << 0,
			DISABLE_DEACTIVATION = 1 << 1,
			_DEPRECATED_FORCE_RESET = 1 << 2,
			WIND = 1 << 3,
		};
		uint32_t _flags = DISABLE_DEACTIVATION;

		float mass = 1.0f;
		float friction = 0.5f;
		float restitution = 0.0f;
		float pressure = 0.0f;
		float vertex_radius = 0.2f; // how much distance vertices keep from other physics bodies
		float detail = 1; // LOD target detail [0,1]
		uint32_t gpuBoneOffset = 0; // non-serialized, but better padding here
		wi::vector<uint32_t> physicsIndices; // physics vertex connectivity
		wi::vector<uint32_t> physicsToGraphicsVertexMapping; // maps graphics vertex index to physics vertex index of the same position
		wi::vector<float> weights; // weight per physics vertex controlling the mass. (0: disable weight (no physics, only animation), 1: default weight)

		// Non-serialized attributes:
		std::shared_ptr<void> physicsobject = nullptr; // You can set to null to recreate the physics object the next time phsyics system will be running.
		XMFLOAT4X4 worldMatrix = wi::math::IDENTITY_MATRIX;
		wi::vector<ShaderTransform> boneData; // simulated soft body nodes as bone matrices that can be fed into skinning
		wi::primitive::AABB aabb;

		constexpr void SetDisableDeactivation(bool value) { if (value) { _flags |= DISABLE_DEACTIVATION; } else { _flags &= ~DISABLE_DEACTIVATION; } }
		constexpr void SetWindEnabled(bool value) { if (value) { _flags |= WIND; } else { _flags &= ~WIND; } }

		constexpr bool IsDisableDeactivation() const { return _flags & DISABLE_DEACTIVATION; }
		constexpr bool IsWindEnabled() const { return _flags & WIND; }

		void Reset()
		{
			physicsIndices.clear();
			physicsToGraphicsVertexMapping.clear();
			physicsobject = {};
		}

		void SetDetail(float loddetail)
		{
			detail = loddetail;
			Reset();
		}

		// Create physics represenation of graphics mesh
		void CreateFromMesh(MeshComponent& mesh);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ArmatureComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;
		uint32_t gpuBoneOffset = 0; // non-serialized, but better padding here

		wi::vector<wi::ecs::Entity> boneCollection;
		wi::vector<XMFLOAT4X4> inverseBindMatrices;

		// Non-serialized attributes:
		wi::primitive::AABB aabb;
		wi::vector<ShaderTransform> boneData;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct LightComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			CAST_SHADOW = 1 << 0,
			VOLUMETRICS = 1 << 1,
			VISUALIZER = 1 << 2,
			LIGHTMAPONLY_STATIC = 1 << 3,
			VOLUMETRICCLOUDS = 1 << 4,
		};
		uint32_t _flags = EMPTY;

		enum LightType : uint32_t
		{
			DIRECTIONAL,
			POINT,
			SPOT,
			RECTANGLE,
			LIGHTTYPE_COUNT,
		};
		LightType type = POINT;

		XMFLOAT3 color = XMFLOAT3(1, 1, 1);
		float intensity = 1.0f; // Brightness of light in. The units that this is defined in depend on the type of light. Point and spot lights use luminous intensity in candela (lm/sr) while directional lights use illuminance in lux (lm/m2). https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
		float range = 10.0f;
		float outerConeAngle = XM_PIDIV4;
		float innerConeAngle = 0; // default value is 0, means only outer cone angle is used
		float radius = 0.025f;
		float length = 0; // point lights: line/capsule length; rect light: width
		float height = 0; // rect light only
		float volumetric_boost = 0; // increase the strength of volumetric fog only for this light

		wi::vector<float> cascade_distances = { 8,80,800 };
		wi::vector<std::string> lensFlareNames;

		int forced_shadow_resolution = -1; // -1: disabled, greater: fixed shadow map resolution

		wi::ecs::Entity cameraSource = wi::ecs::INVALID_ENTITY; // take texture from camera render

		// Non-serialized attributes:
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 direction = XMFLOAT3(0, 1, 0);
		mutable int occlusionquery = -1;
		XMFLOAT4 rotation = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT3 scale = XMFLOAT3(1, 1, 1);
		int maskTexDescriptor = -1;

		wi::vector<wi::Resource> lensFlareRimTextures;

		constexpr void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		constexpr void SetVolumetricsEnabled(bool value) { if (value) { _flags |= VOLUMETRICS; } else { _flags &= ~VOLUMETRICS; } }
		constexpr void SetVisualizerEnabled(bool value) { if (value) { _flags |= VISUALIZER; } else { _flags &= ~VISUALIZER; } }
		constexpr void SetStatic(bool value) { if (value) { _flags |= LIGHTMAPONLY_STATIC; } else { _flags &= ~LIGHTMAPONLY_STATIC; } }
		constexpr void SetVolumetricCloudsEnabled(bool value) { if (value) { _flags |= VOLUMETRICCLOUDS; } else { _flags &= ~VOLUMETRICCLOUDS; } }

		constexpr bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		constexpr bool IsVolumetricsEnabled() const { return _flags & VOLUMETRICS; }
		constexpr bool IsVisualizerEnabled() const { return _flags & VISUALIZER; }
		constexpr bool IsStatic() const { return _flags & LIGHTMAPONLY_STATIC; }
		constexpr bool IsVolumetricCloudsEnabled() const { return _flags & VOLUMETRICCLOUDS; }
		constexpr bool IsInactive() const { return intensity < 0.0001f || range < 0.0001f; }

		constexpr float GetRange() const
		{
			float retval = range;
			retval = std::max(0.001f, retval);
			retval = std::min(retval, 65504.0f); // clamp to 16-bit float max value
			return retval;
		}

		constexpr void SetType(LightType val) { type = val; }
		constexpr LightType GetType() const { return type; }

		// Set energy amount with non physical light units (from before version 0.70.0):
		constexpr void BackCompatSetEnergy(float energy)
		{
			switch (type)
			{
			case wi::scene::LightComponent::POINT:
				intensity = energy * 20;
				break;
			case wi::scene::LightComponent::SPOT:
				intensity = energy * 200;
				break;
			default:
				break;
			}
		}

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct alignas(16) CameraComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			CUSTOM_PROJECTION = 1 << 1,
			ORTHO = 1 << 2,
			CRT_FILTER = 1 << 3,
		};
		uint32_t _flags = EMPTY;

		float width = 0.0f;
		float height = 0.0f;
		float zNearP = 0.1f;
		float zFarP = 5000.0f;
		float fov = XM_PI / 3.0f;
		float focal_length = 1;
		float aperture_size = 0;
		XMFLOAT2 aperture_shape = XMFLOAT2(1, 1);
		float ortho_vertical_size = 1;

		// Non-serialized attributes:
		XMFLOAT3 Eye = XMFLOAT3(0, 0, 0);
		XMFLOAT3 At = XMFLOAT3(0, 0, 1);
		XMFLOAT3 Up = XMFLOAT3(0, 1, 0);
		XMFLOAT4X4 View, Projection, VP;
		wi::primitive::Frustum frustum;
		XMFLOAT4X4 InvView, InvProjection, InvVP;
		XMFLOAT3X3 rotationMatrix;
		XMFLOAT2 jitter;
		float padding = 0;
		XMFLOAT4 clipPlane = XMFLOAT4(0, 0, 0, 0); // default: no clip plane
		XMFLOAT4 clipPlaneOriginal = XMFLOAT4(0, 0, 0, 0); // not reversed clip plane
		wi::Canvas canvas;
		wi::graphics::Rect scissor;
		uint32_t sample_count = 1;
		int texture_primitiveID_index = -1;
		int texture_depth_index = -1;
		int texture_lineardepth_index = -1;
		int texture_velocity_index = -1;
		int texture_normal_index = -1;
		int texture_roughness_index = -1;
		int texture_reflection_index = -1;
		int texture_reflection_depth_index = -1;
		int texture_refraction_index = -1;
		int texture_waterriples_index = -1;
		int texture_ao_index = -1;
		int texture_ssr_index = -1;
		int texture_ssgi_index = -1;
		int texture_rtshadow_index = -1;
		int texture_rtdiffuse_index = -1;
		int texture_surfelgi_index = -1;
		int buffer_entitytiles_index = -1;
		int texture_vxgi_diffuse_index = -1;
		int texture_vxgi_specular_index = -1;
		int texture_reprojected_depth_index = -1;
		uint shadercamera_options = SHADERCAMERA_OPTION_NONE;

		struct RenderToTexture
		{
			XMUINT2 resolution = XMUINT2(0, 0);
			uint32_t sample_count = 1;
			wi::graphics::Texture rendertarget_MSAA;
			wi::graphics::Texture rendertarget_render;
			wi::graphics::Texture rendertarget_display;
			wi::graphics::Texture depthstencil;
			wi::graphics::Texture depthstencil_resolved;
			XMUINT2 tileCount = {};
			wi::graphics::GPUBuffer entityTiles;
			std::shared_ptr<void> visibility;
		} render_to_texture;

		void CreateOrtho(float newWidth, float newHeight, float newNear, float newFar, float newVerticalSize = 1);
		void CreatePerspective(float newWidth, float newHeight, float newNear, float newFar, float newFOV = XM_PI / 3.0f);
		void UpdateCamera();
		void TransformCamera(const XMMATRIX& W);
		void TransformCamera(const TransformComponent& transform) { TransformCamera(XMLoadFloat4x4(&transform.world)); }
		void Reflect(const XMFLOAT4& plane = XMFLOAT4(0, 1, 0, 0));

		inline XMVECTOR GetEye() const { return XMLoadFloat3(&Eye); }
		inline XMVECTOR GetAt() const { return XMLoadFloat3(&At); }
		inline XMVECTOR GetUp() const { return XMLoadFloat3(&Up); }
		inline XMVECTOR GetRight() const { return XMVector3Cross(GetUp(), GetAt()); }
		inline XMMATRIX GetView() const { return XMLoadFloat4x4(&View); }
		inline XMMATRIX GetInvView() const { return XMLoadFloat4x4(&InvView); }
		inline XMMATRIX GetProjection() const { return XMLoadFloat4x4(&Projection); }
		inline XMMATRIX GetInvProjection() const { return XMLoadFloat4x4(&InvProjection); }
		inline XMMATRIX GetViewProjection() const { return XMLoadFloat4x4(&VP); }
		inline XMMATRIX GetInvViewProjection() const { return XMLoadFloat4x4(&InvVP); }

		// Returns the vertical size of ortho projection that matches the perspective size at given distance
		float ComputeOrthoVerticalSizeFromPerspective(float dist);

		constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		constexpr void SetCustomProjectionEnabled(bool value = true) { if (value) { _flags |= CUSTOM_PROJECTION; } else { _flags &= ~CUSTOM_PROJECTION; } }
		constexpr void SetOrtho(bool value = true) { if (value) { _flags |= ORTHO; } else { _flags &= ~ORTHO; } SetDirty(); }
		constexpr void SetCRT(bool value = true) { if (value) { _flags |= CRT_FILTER; } else { _flags &= ~CRT_FILTER; } SetDirty(); }
		constexpr bool IsDirty() const { return _flags & DIRTY; }
		constexpr bool IsCustomProjectionEnabled() const { return _flags & CUSTOM_PROJECTION; }
		constexpr bool IsOrtho() const { return _flags & ORTHO; }
		constexpr bool IsCRT() const { return _flags & CRT_FILTER; }

		void Lerp(const CameraComponent& a, const CameraComponent& b, float t);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct EnvironmentProbeComponent
	{
		static constexpr uint32_t envmapMSAASampleCount = 8;
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			REALTIME = 1 << 1,
			MSAA = 1 << 2,
		};
		uint32_t _flags = DIRTY;
		uint32_t resolution = 128; // power of two
		std::string textureName; // if texture is coming from an asset

		// Non-serialized attributes:
		wi::graphics::Texture texture;
		wi::Resource resource; // if texture is coming from an asset
		XMFLOAT3 position;
		float range;
		XMFLOAT4X4 inverseMatrix;
		mutable bool render_dirty = false;

		constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; DeleteResource(); } else { _flags &= ~DIRTY; } }
		constexpr void SetRealTime(bool value) { if (value) { _flags |= REALTIME; } else { _flags &= ~REALTIME; } }
		constexpr void SetMSAA(bool value) { if (value) { _flags |= MSAA; } else { _flags &= ~MSAA; } SetDirty(); }

		constexpr bool IsDirty() const { return _flags & DIRTY; }
		constexpr bool IsRealTime() const { return _flags & REALTIME; }
		constexpr bool IsMSAA() const { return _flags & MSAA; }
		constexpr uint32_t GetSampleCount() const { return IsMSAA() ? envmapMSAASampleCount : 1; }

		size_t GetMemorySizeInBytes() const;

		void CreateRenderData();
		void DeleteResource();

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ForceFieldComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		enum class Type
		{
			Point,
			Plane,
		} type = Type::Point;

		float gravity = 0; // negative = deflector, positive = attractor
		float range = 0; // affection range

		// Non-serialized attributes:
		XMFLOAT3 position;
		XMFLOAT3 direction;

		constexpr float GetRange() const { return range; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct DecalComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			BASECOLOR_ONLY_ALPHA = 1 << 0,
		};
		uint32_t _flags = EMPTY;

		float slopeBlendPower = 0;

		// Set decal to only use alpha from base color texture. Useful for blending normalmap-only decals
		constexpr void SetBaseColorOnlyAlpha(bool value) { if (value) { _flags |= BASECOLOR_ONLY_ALPHA; } else { _flags &= ~BASECOLOR_ONLY_ALPHA; } }

		constexpr bool IsBaseColorOnlyAlpha() const { return _flags & BASECOLOR_ONLY_ALPHA; }

		// Non-serialized attributes:
		float emissive;
		XMFLOAT4 color;
		XMFLOAT3 front;
		float normal_strength;
		float displacement_strength;
		XMFLOAT3 position;
		float range;
		XMFLOAT4X4 world;
		XMFLOAT4 texMulAdd;

		wi::Resource texture;
		wi::Resource normal;
		wi::Resource surfacemap;
		wi::Resource displacementmap;

		constexpr float GetOpacity() const { return color.w; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct AnimationDataComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		wi::vector<float> keyframe_times;
		wi::vector<float> keyframe_data;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct AnimationComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			LOOPED = 1 << 1,
			ROOT_MOTION = 1 << 2,
			PING_PONG = 1 << 3,
		};
		uint32_t _flags = LOOPED;
		float start = 0;
		float end = 0;
		float timer = 0;
		float amount = 1;	// blend amount
		float speed = 1;

		struct AnimationChannel
		{
			enum FLAGS
			{
				EMPTY = 0,
			};
			uint32_t _flags = EMPTY;

			wi::ecs::Entity target = wi::ecs::INVALID_ENTITY;
			int samplerIndex = -1;
			int retargetIndex = -1;

			enum class Path
			{
				TRANSLATION,
				ROTATION,
				SCALE,
				WEIGHTS,

				LIGHT_COLOR,
				LIGHT_INTENSITY,
				LIGHT_RANGE,
				LIGHT_INNERCONE,
				LIGHT_OUTERCONE,
				// additional light paths can go here...
				_LIGHT_RANGE_END = LIGHT_COLOR + 1000,

				SOUND_PLAY,
				SOUND_STOP,
				SOUND_VOLUME,
				// additional sound paths can go here...
				_SOUND_RANGE_END = SOUND_PLAY + 1000,

				EMITTER_EMITCOUNT,
				// additional emitter paths can go here...
				_EMITTER_RANGE_END = EMITTER_EMITCOUNT + 1000,

				CAMERA_FOV,
				CAMERA_FOCAL_LENGTH,
				CAMERA_APERTURE_SIZE,
				CAMERA_APERTURE_SHAPE,
				// additional camera paths can go here...
				_CAMERA_RANGE_END = CAMERA_FOV + 1000,

				SCRIPT_PLAY,
				SCRIPT_STOP,
				// additional script paths can go here...
				_SCRIPT_RANGE_END = SCRIPT_PLAY + 1000,

				MATERIAL_COLOR,
				MATERIAL_EMISSIVE,
				MATERIAL_ROUGHNESS,
				MATERIAL_METALNESS,
				MATERIAL_REFLECTANCE,
				MATERIAL_TEXMULADD,
				// additional material paths can go here...
				_MATERIAL_RANGE_END = MATERIAL_COLOR + 1000,

				UNKNOWN,
			} path = Path::UNKNOWN;

			enum class PathDataType
			{
				Event,
				Float,
				Float2,
				Float3,
				Float4,
				Weights,

				Count,
			};
			PathDataType GetPathDataType() const;

			// Non-serialized attributes:
			mutable int next_event = 0;
		};
		struct AnimationSampler
		{
			enum FLAGS
			{
				EMPTY = 0,
			};
			uint32_t _flags = LOOPED;

			wi::ecs::Entity data = wi::ecs::INVALID_ENTITY;

			enum Mode
			{
				LINEAR,
				STEP,
				CUBICSPLINE,
				MODE_FORCE_UINT32 = 0xFFFFFFFF
			} mode = LINEAR;

			// The data is now not part of the sampler, so it can be shared. This is kept only for backwards compatibility with previous versions.
			AnimationDataComponent backwards_compatibility_data;

			// Non-serialized attributes:
			const void* scene = nullptr; // if animation data is in a different scene (if retargetting from a separate scene)
		};
		struct RetargetSourceData
		{
			wi::ecs::Entity source = wi::ecs::INVALID_ENTITY;
			XMFLOAT4X4 dstRelativeMatrix = wi::math::IDENTITY_MATRIX;
			XMFLOAT4X4 srcRelativeParentMatrix = wi::math::IDENTITY_MATRIX;
		};

		wi::vector<AnimationChannel> channels;
		wi::vector<AnimationSampler> samplers;
		wi::vector<RetargetSourceData> retargets;

		// Non-serialzied attributes:
		wi::vector<float> morph_weights_temp;
		float last_update_time = 0;

		// Root Motion
		XMFLOAT3 rootTranslationOffset;
		XMFLOAT4 rootRotationOffset;
		wi::ecs::Entity rootMotionBone;

		XMVECTOR rootPrevTranslation = XMVectorSet(-69, 420, 69, -420);
		XMVECTOR rootPrevRotation = XMVectorSet(-69, 420, 69, -420);
		inline static const XMVECTOR INVALID_VECTOR = XMVectorSet(-69, 420, 69, -420);
		float prevLocTimer;
		float prevRotTimer;
		// Root Motion

		constexpr bool IsPlaying() const { return _flags & PLAYING; }
		constexpr bool IsLooped() const { return _flags & LOOPED; }
		constexpr bool IsPingPong() const { return _flags & PING_PONG; }
		constexpr bool IsPlayingOnce() const { return (_flags & (LOOPED | PING_PONG)) == 0; }
		constexpr float GetLength() const { return end - start; }
		constexpr bool IsEnded() const { return timer >= end; }
		constexpr bool IsRootMotion() const { return _flags & ROOT_MOTION; }

		constexpr void Play() { _flags |= PLAYING; }
		constexpr void Pause() { _flags &= ~PLAYING; }
		constexpr void Stop() { Pause(); timer = 0.0f; last_update_time = timer; }
		constexpr void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; _flags &= ~PING_PONG; } else { _flags &= ~LOOPED; } }
		constexpr void SetPingPong(bool value = true) { if (value) { _flags |= PING_PONG; _flags &= ~LOOPED; } else { _flags &= ~PING_PONG; } }
		constexpr void SetPlayOnce() { _flags &= ~(LOOPED | PING_PONG); }

		constexpr void RootMotionOn() { _flags |= ROOT_MOTION; }
		constexpr void RootMotionOff() { _flags &= ~ROOT_MOTION; }
		constexpr XMFLOAT3 GetRootTranslation() const { return rootTranslationOffset; }
		constexpr XMFLOAT4 GetRootRotation() const { return rootRotationOffset; }
		constexpr wi::ecs::Entity GetRootMotionBone() const { return rootMotionBone; }
		constexpr void SetRootMotionBone(wi::ecs::Entity _rootMotionBone) { rootMotionBone = _rootMotionBone; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct WeatherComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			OCEAN_ENABLED = 1 << 0,
			_DEPRECATED_SIMPLE_SKY = 1 << 1,
			REALISTIC_SKY = 1 << 2,
			VOLUMETRIC_CLOUDS = 1 << 3,
			HEIGHT_FOG = 1 << 4,
			VOLUMETRIC_CLOUDS_CAST_SHADOW = 1 << 5,
			OVERRIDE_FOG_COLOR = 1 << 6,
			REALISTIC_SKY_AERIAL_PERSPECTIVE = 1 << 7,
			REALISTIC_SKY_HIGH_QUALITY = 1 << 8,
			REALISTIC_SKY_RECEIVE_SHADOW = 1 << 9,
			VOLUMETRIC_CLOUDS_RECEIVE_SHADOW = 1 << 10,
		};
		uint32_t _flags = EMPTY;

		constexpr bool IsOceanEnabled() const { return _flags & OCEAN_ENABLED; }
		constexpr bool IsRealisticSky() const { return _flags & REALISTIC_SKY; }
		constexpr bool IsVolumetricClouds() const { return _flags & VOLUMETRIC_CLOUDS; }
		constexpr bool IsHeightFog() const { return _flags & HEIGHT_FOG; }
		constexpr bool IsVolumetricCloudsCastShadow() const { return _flags & VOLUMETRIC_CLOUDS_CAST_SHADOW; }
		constexpr bool IsOverrideFogColor() const { return _flags & OVERRIDE_FOG_COLOR; }
		constexpr bool IsRealisticSkyAerialPerspective() const { return _flags & REALISTIC_SKY_AERIAL_PERSPECTIVE; }
		constexpr bool IsRealisticSkyHighQuality() const { return _flags & REALISTIC_SKY_HIGH_QUALITY; }
		constexpr bool IsRealisticSkyReceiveShadow() const { return _flags & REALISTIC_SKY_RECEIVE_SHADOW; }
		constexpr bool IsVolumetricCloudsReceiveShadow() const { return _flags & VOLUMETRIC_CLOUDS_RECEIVE_SHADOW; }

		constexpr void SetOceanEnabled(bool value = true) { if (value) { _flags |= OCEAN_ENABLED; } else { _flags &= ~OCEAN_ENABLED; } }
		constexpr void SetRealisticSky(bool value = true) { if (value) { _flags |= REALISTIC_SKY; } else { _flags &= ~REALISTIC_SKY; } }
		constexpr void SetVolumetricClouds(bool value = true) { if (value) { _flags |= VOLUMETRIC_CLOUDS; } else { _flags &= ~VOLUMETRIC_CLOUDS; } }
		constexpr void SetHeightFog(bool value = true) { if (value) { _flags |= HEIGHT_FOG; } else { _flags &= ~HEIGHT_FOG; } }
		constexpr void SetVolumetricCloudsCastShadow(bool value = true) { if (value) { _flags |= VOLUMETRIC_CLOUDS_CAST_SHADOW; } else { _flags &= ~VOLUMETRIC_CLOUDS_CAST_SHADOW; } }
		constexpr void SetOverrideFogColor(bool value = true) { if (value) { _flags |= OVERRIDE_FOG_COLOR; } else { _flags &= ~OVERRIDE_FOG_COLOR; } }
		constexpr void SetRealisticSkyAerialPerspective(bool value = true) { if (value) { _flags |= REALISTIC_SKY_AERIAL_PERSPECTIVE; } else { _flags &= ~REALISTIC_SKY_AERIAL_PERSPECTIVE; } }
		constexpr void SetRealisticSkyHighQuality(bool value = true) { if (value) { _flags |= REALISTIC_SKY_HIGH_QUALITY; } else { _flags &= ~REALISTIC_SKY_HIGH_QUALITY; } }
		constexpr void SetRealisticSkyReceiveShadow(bool value = true) { if (value) { _flags |= REALISTIC_SKY_RECEIVE_SHADOW; } else { _flags &= ~REALISTIC_SKY_RECEIVE_SHADOW; } }
		constexpr void SetVolumetricCloudsReceiveShadow(bool value = true) { if (value) { _flags |= VOLUMETRIC_CLOUDS_RECEIVE_SHADOW; } else { _flags &= ~VOLUMETRIC_CLOUDS_RECEIVE_SHADOW; } }

		XMFLOAT3 sunColor = XMFLOAT3(0, 0, 0);
		XMFLOAT3 sunDirection = XMFLOAT3(0, 1, 0);
		float skyExposure = 1;
		XMFLOAT3 horizon = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 zenith = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		float fogStart = 100;
		float fogDensity = 0;
		float fogHeightStart = 1;
		float fogHeightEnd = 3;
		XMFLOAT3 windDirection = XMFLOAT3(0, 0, 0);
		float windRandomness = 5;
		float windWaveSize = 1;
		float windSpeed = 1;
		float stars = 0.5f;
		XMFLOAT3 gravity = XMFLOAT3(0, -10, 0);
		float sky_rotation = 0; // horizontal rotation for skyMap texture (in radians)
		float rain_amount = 0;
		float rain_length = 0.04f;
		float rain_speed = 1;
		float rain_scale = 0.005f;
		float rain_splash_scale = 0.1f;
		XMFLOAT4 rain_color = XMFLOAT4(0.6f, 0.8f, 1, 0.5f);

		wi::Ocean::OceanParameters oceanParameters;
		AtmosphereParameters atmosphereParameters;
		VolumetricCloudParameters volumetricCloudParameters;

		std::string skyMapName;
		std::string colorGradingMapName;
		std::string volumetricCloudsWeatherMapFirstName;
		std::string volumetricCloudsWeatherMapSecondName;

		// Non-serialized attributes:
		wi::Resource skyMap;
		wi::Resource colorGradingMap;
		wi::Resource volumetricCloudsWeatherMapFirst;
		wi::Resource volumetricCloudsWeatherMapSecond;
		XMFLOAT4 stars_rotation_quaternion = XMFLOAT4(0, 0, 0, 1);
		uint32_t most_important_light_index = ~0u;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct SoundComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			LOOPED = 1 << 1,
			DISABLE_3D = 1 << 2,
		};
		uint32_t _flags = LOOPED;

		std::string filename;
		wi::Resource soundResource;
		wi::audio::SoundInstance soundinstance;
		float volume = 1;

		constexpr bool IsPlaying() const { return _flags & PLAYING; }
		constexpr bool IsLooped() const { return _flags & LOOPED; }
		constexpr bool IsDisable3D() const { return _flags & DISABLE_3D; }

		void Play();
		void Stop();
		void SetLooped(bool value = true);
		void SetDisable3D(bool value = true);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct VideoComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			LOOPED = 1 << 1,
		};
		uint32_t _flags = LOOPED;

		std::string filename;
		wi::Resource videoResource;
		wi::video::VideoInstance videoinstance;
		float currentTimer = 0; // The current playback timer is reflected in this

		constexpr bool IsPlaying() const { return _flags & PLAYING; }
		constexpr bool IsLooped() const { return _flags & LOOPED; }

		constexpr void Play() { _flags |= PLAYING; }
		constexpr void Pause() { _flags &= ~PLAYING; }
		void Stop() { Pause(); Seek(0); }
		constexpr void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }

		// Get total length of video in seconds
		float GetLength() const;

		// seek to timestamp (approximate)
		void Seek(float timerSeconds);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct InverseKinematicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DISABLED = 1 << 0,
		};
		uint32_t _flags = EMPTY;

		wi::ecs::Entity target = wi::ecs::INVALID_ENTITY; // which entity to follow (must have a transform component)
		uint32_t chain_length = 0; // recursive depth
		uint32_t iteration_count = 1; // computation step count. Increase this too for greater chain length

		// Non-serialized attributes:
		bool use_target_position = false;
		XMFLOAT3 target_position = XMFLOAT3(0, 0, 0);

		constexpr void SetDisabled(bool value = true) { if (value) { _flags |= DISABLED; } else { _flags &= ~DISABLED; } }
		constexpr bool IsDisabled() const { return _flags & DISABLED; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct SpringComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			RESET = 1 << 0,
			DISABLED = 1 << 1,
			_DEPRECATED_STRETCH_ENABLED = 1 << 2,
			GRAVITY_ENABLED = 1 << 3,
		};
		uint32_t _flags = RESET | GRAVITY_ENABLED;

		float stiffnessForce = 0.5f;
		float dragForce = 0.5f;
		float windForce = 0.5f;
		float hitRadius = 0;
		float gravityPower = 0.5f;
		XMFLOAT3 gravityDir = {};

		// Non-serialized attributes:
		XMFLOAT3 prevTail = {};
		XMFLOAT3 currentTail = {};
		XMFLOAT3 boneAxis = {};

		// These are maintained for top-down chained update by spring dependency system:
		wi::vector<SpringComponent*> children;
		wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
		TransformComponent* transform = nullptr;
		TransformComponent* parent_transform = nullptr;

		constexpr void Reset(bool value = true) { if (value) { _flags |= RESET; } else { _flags &= ~RESET; } }
		constexpr void SetDisabled(bool value = true) { if (value) { _flags |= DISABLED; } else { _flags &= ~DISABLED; } }
		constexpr void SetGravityEnabled(bool value) { if (value) { _flags |= GRAVITY_ENABLED; } else { _flags &= ~GRAVITY_ENABLED; } }

		constexpr bool IsResetting() const { return _flags & RESET; }
		constexpr bool IsDisabled() const { return _flags & DISABLED; }
		constexpr bool IsGravityEnabled() const { return _flags & GRAVITY_ENABLED; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct alignas(16) ColliderComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			CPU = 1 << 0,
			GPU = 1 << 1,
			CAPSULE_SHADOW = 1 << 2,
		};
		uint32_t _flags = CPU;

		constexpr void SetCPUEnabled(bool value = true) { if (value) { _flags |= CPU; } else { _flags &= ~CPU; } }
		constexpr void SetGPUEnabled(bool value = true) { if (value) { _flags |= GPU; } else { _flags &= ~GPU; } }
		constexpr void SetCapsuleShadowEnabled(bool value = true) { if (value) { _flags |= CAPSULE_SHADOW; } else { _flags &= ~CAPSULE_SHADOW; } }

		constexpr bool IsCPUEnabled() const { return _flags & CPU; }
		constexpr bool IsGPUEnabled() const { return _flags & GPU; }
		constexpr bool IsCapsuleShadowEnabled() const { return _flags & CAPSULE_SHADOW; }

		enum class Shape
		{
			Sphere,
			Capsule,
			Plane,
		};
		Shape shape;

		float radius = 0;
		XMFLOAT3 offset = {};
		XMFLOAT3 tail = {};

		// Non-serialized attributes:
		wi::primitive::Sphere sphere;
		wi::primitive::Capsule capsule;
		wi::primitive::Plane plane;
		uint32_t layerMask = ~0u;
		float dist = 0;
		uint32_t cpu_index = 0;
		uint32_t gpu_index = 0;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ScriptComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			PLAY_ONCE = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		std::string filename;

		// Non-serialized attributes:
		wi::vector<uint8_t> script; // compiled script binary data
		wi::Resource resource;
		size_t script_hash = 0;

		constexpr void Play() { _flags |= PLAYING; }
		constexpr void SetPlayOnce(bool once = true) { if (once) { _flags |= PLAY_ONCE; } else { _flags &= ~PLAY_ONCE; } }
		constexpr void Stop() { _flags &= ~PLAYING; }

		constexpr bool IsPlaying() const { return _flags & PLAYING; }
		constexpr bool IsPlayingOnlyOnce() const { return _flags & PLAY_ONCE; }

		void CreateFromFile(const std::string& filename);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ExpressionComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			FORCE_TALKING = 1 << 0,
			TALKING_ENDED = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		// Preset expressions can have common behaviours assigned:
		//	https://github.com/vrm-c/vrm-specification/blob/bd205a6c3839993f2729e4e7c3a74af89877cfce/specification/VRMC_vrm-1.0-beta/expressions.md#preset-expressions
		enum class Preset
		{
			// Emotions:
			Happy,		// Changed from joy
			Angry,		// anger
			Sad,		// Changed from sorrow
			Relaxed,	// Comfortable. Changed from fun
			Surprised,	// surprised. Added new in VRM 1.0

			// Lip sync procedural:
			//	Procedural: A value that can be automatically generated by the system.
			//	- Analyze microphone input, generate from text, etc.
			Aa,			// aa
			Ih,			// i
			Ou,			// u
			Ee,			// eh
			Oh,			// oh

			// Blink procedural:
			//	Procedural: A value that can be automatically generated by the system.
			//	- Randomly blink, etc.
			Blink,		// close both eyelids
			BlinkLeft,	// Close the left eyelid
			BlinkRight,	// Close right eyelid

			// Gaze procedural:
			//	Procedural: A value that can be automatically generated by the system.
			//	- The VRM LookAt will generate a value for the gaze point from time to time (see LookAt Expression Type).
			LookUp,		// For models where the line of sight moves with Expression instead of bone. See eye control.
			LookDown,	// For models where the line of sight moves with Expression instead of bone. See eye control.
			LookLeft,	// For models whose line of sight moves with Expression instead of bone. See eye control.
			LookRight,	// For models where the line of sight moves with Expression instead of bone. See eye control.

			// Other:
			Neutral,	// left for backwards compatibility.

			Count,
		};
		int presets[size_t(Preset::Count)] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

		enum class Override
		{
			None,
			Block,
			Blend,
		};

		float blink_frequency = 0.3f;	// number of blinks per second
		float blink_length = 0.1f;	// blink's completion time in seconds
		int blink_count = 2;
		float look_frequency = 0;	// number of lookAt changes per second
		float look_length = 0.6f;	// lookAt's completion time in seconds

		struct Expression
		{
			enum FLAGS
			{
				EMPTY = 0,
				DIRTY = 1 << 0,
				BINARY = 1 << 1,
			};
			uint32_t _flags = EMPTY;

			std::string name;
			float weight = 0;

			Preset preset = Preset::Count;
			Override override_mouth = Override::None;
			Override override_blink = Override::None;
			Override override_look = Override::None;

			struct MorphTargetBinding
			{
				wi::ecs::Entity meshID = wi::ecs::INVALID_ENTITY;
				int index = 0;
				float weight = 0;
			};
			wi::vector<MorphTargetBinding> morph_target_bindings;

			constexpr bool IsDirty() const { return _flags & DIRTY; }
			constexpr bool IsBinary() const { return _flags & BINARY; }

			constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
			constexpr void SetBinary(bool value = true) { if (value) { _flags |= BINARY; } else { _flags &= ~BINARY; } }

			// Set weight of expression (also sets dirty state if value is out of date)
			inline void SetWeight(float value)
			{
				if (std::abs(weight - value) > std::numeric_limits<float>::epsilon())
				{
					SetDirty(true);
				}
				weight = value;
			}
		};
		wi::vector<Expression> expressions;

		constexpr bool IsForceTalkingEnabled() const { return _flags & FORCE_TALKING; }

		// Force continuous talking animation, even if no voice is playing
		constexpr void SetForceTalkingEnabled(bool value = true) { if (value) { _flags |= FORCE_TALKING; } else { _flags &= ~FORCE_TALKING; _flags |= TALKING_ENDED; } }

		// Non-serialized attributes:
		float blink_timer = 0;
		float look_timer = 0;
		float look_weights[4] = {};
		Preset talking_phoneme = Preset::Aa;
		float talking_weight_prev_prev = 0;
		float talking_weight_prev = 0;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct HumanoidComponent
	{
		enum FLAGS
		{
			NONE = 0,
			LOOKAT = 1 << 0,
			RAGDOLL_PHYSICS = 1 << 1,
			DISABLE_INTERSECTION = 1 << 2,
			DISABLE_CAPSULE_SHADOW = 1 << 3,
		};
		uint32_t _flags = LOOKAT;

		// https://github.com/vrm-c/vrm-specification/blob/master/specification/VRMC_vrm-1.0-beta/humanoid.md#list-of-humanoid-bones
		enum class HumanoidBone
		{
			// Torso:
			Hips,			// Required
			Spine,			// Required
			Chest,
			UpperChest,
			Neck,

			// Head:
			Head,			// Required
			LeftEye,
			RightEye,
			Jaw,

			// Leg:
			LeftUpperLeg,	// Required
			LeftLowerLeg,	// Required
			LeftFoot,		// Required
			LeftToes,
			RightUpperLeg,	// Required
			RightLowerLeg,	// Required
			RightFoot,		// Required
			RightToes,

			// Arm:
			LeftShoulder,
			LeftUpperArm,	// Required
			LeftLowerArm,	// Required
			LeftHand,		// Required
			RightShoulder,
			RightUpperArm,	// Required
			RightLowerArm,	// Required
			RightHand,		// Required

			// Finger:
			LeftThumbMetacarpal,
			LeftThumbProximal,
			LeftThumbDistal,
			LeftIndexProximal,
			LeftIndexIntermediate,
			LeftIndexDistal,
			LeftMiddleProximal,
			LeftMiddleIntermediate,
			LeftMiddleDistal,
			LeftRingProximal,
			LeftRingIntermediate,
			LeftRingDistal,
			LeftLittleProximal,
			LeftLittleIntermediate,
			LeftLittleDistal,
			RightThumbMetacarpal,
			RightThumbProximal,
			RightThumbDistal,
			RightIndexIntermediate,
			RightIndexDistal,
			RightIndexProximal,
			RightMiddleProximal,
			RightMiddleIntermediate,
			RightMiddleDistal,
			RightRingProximal,
			RightRingIntermediate,
			RightRingDistal,
			RightLittleProximal,
			RightLittleIntermediate,
			RightLittleDistal,

			Count
		};
		wi::ecs::Entity bones[size_t(HumanoidBone::Count)] = {};

		constexpr bool IsLookAtEnabled() const { return _flags & LOOKAT; }
		constexpr bool IsRagdollPhysicsEnabled() const { return _flags & RAGDOLL_PHYSICS; }
		constexpr bool IsIntersectionDisabled() const { return _flags & DISABLE_INTERSECTION; }
		constexpr bool IsCapsuleShadowDisabled() const { return _flags & DISABLE_CAPSULE_SHADOW; }

		constexpr void SetLookAtEnabled(bool value = true) { if (value) { _flags |= LOOKAT; } else { _flags &= ~LOOKAT; } }
		constexpr void SetRagdollPhysicsEnabled(bool value = true) { if (value) { _flags |= RAGDOLL_PHYSICS; } else { _flags &= ~RAGDOLL_PHYSICS; } }
		constexpr void SetIntersectionDisabled(bool value = true) { if (value) { _flags |= DISABLE_INTERSECTION; } else { _flags &= ~DISABLE_INTERSECTION; } }
		constexpr void SetCapsuleShadowDisabled(bool value = true) { if (value) { _flags |= DISABLE_CAPSULE_SHADOW; } else { _flags &= ~DISABLE_CAPSULE_SHADOW; } }

		XMFLOAT2 head_rotation_max = XMFLOAT2(XM_PI / 3.0f, XM_PI / 6.0f);
		float head_rotation_speed = 0.1f;
		XMFLOAT2 eye_rotation_max = XMFLOAT2(XM_PI / 20.0f, XM_PI / 20.0f);
		float eye_rotation_speed = 0.1f;

		float ragdoll_fatness = 1.0f;
		float ragdoll_headsize = 1.0f;

		wi::ecs::Entity lookAtEntity = wi::ecs::INVALID_ENTITY; // lookAt can be fixed to specific entity

		float arm_spacing = 0;
		float leg_spacing = 0;

		// Non-serialized attributes:
		XMFLOAT3 lookAt = {}; // lookAt target pos, can be set by user
		XMFLOAT4 lookAtDeltaRotationState_Head = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT4 lookAtDeltaRotationState_LeftEye = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT4 lookAtDeltaRotationState_RightEye = XMFLOAT4(0, 0, 0, 1);
		std::shared_ptr<void> ragdoll = nullptr; // physics system implementation-specific object
		float default_facing = 0; // 0 = not yet computed, otherwise Z direction
		float knee_bending = 0; // 0 = not yet computed, otherwise Z direction

		// Things for ragdoll intersection tests:
		struct RagdollBodypart
		{
			HumanoidBone bone;
			wi::primitive::Capsule capsule;
		};
		wi::vector<RagdollBodypart> ragdoll_bodyparts;
		wi::primitive::AABB ragdoll_bounds;

		// Returns true if all the required bones are valid, false otherwise
		bool IsValid() const;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct MetadataComponent
	{
		enum FLAGS
		{
			NONE = 0,
		};
		uint32_t _flags = NONE;

		enum class Preset
		{
			Custom,
			Waypoint,
			Player,
			Enemy,
			NPC,
			Pickup,
			Vehicle,
		};
		Preset preset = Preset::Custom;

		// Ordered collection of values, but also accelerated lookup by string
		template<typename T>
		struct OrderedNamedValues
		{
			wi::unordered_map<std::string, size_t> lookup; // name -> value hash lookup
			wi::vector<std::string> names; // ordered names
			wi::vector<T> values; // ordered values

			void reserve(size_t capacity)
			{
				lookup.reserve(capacity);
				names.reserve(capacity);
				values.reserve(capacity);
			}
			size_t size() const
			{
				return lookup.size();
			}
			bool has(const std::string& name) const
			{
				return lookup.count(name) > 0;
			}
			T get(size_t index) const
			{
				if (index < values.size())
					return values[index];
				return T();
			}
			T get_name(size_t index) const
			{
				if (index < names.size())
					return names[index];
				return T();
			}
			T get(const std::string& name) const
			{
				if (has(name))
					return values[lookup.find(name)->second];
				return T();
			}
			void set(const std::string& name, const T& value)
			{
				if (has(name))
				{
					// modify existing entry:
					values[lookup[name]] = value;
					names[lookup[name]] = name;
				}
				else
				{
					// register new entry:
					lookup[name] = values.size();
					values.push_back(value);
					names.push_back(name);
				}
			}
			void erase(const std::string& name)
			{
				if (has(name))
				{
					values.erase(values.begin() + lookup[name]);
					names.erase(names.begin() + lookup[name]);
					lookup.erase(name);

					// reorder lookup
					for (size_t i = 0; i < names.size(); ++i)
					{
						lookup[names[i]] = i;
					}
				}
			}
		};

		OrderedNamedValues<bool> bool_values;
		OrderedNamedValues<int> int_values;
		OrderedNamedValues<float> float_values;
		OrderedNamedValues<std::string> string_values;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct CharacterComponent
	{
		enum FLAGS
		{
			NONE = 0,
			CHARACTER_TO_CHARACTER_COLLISION_DISABLED = 1 << 0,
			DEDICATED_SHADOW = 1 << 1,
		};
		uint32_t _flags = NONE;

		int health = 100;
		float width = 0.4f; // capsule radius
		float height = 1.5f; // capsule height
		float scale = 1.0f; // overall scaling

		// Non-serialized attributes:
		float ground_friction = 0.92f;
		float water_friction = 0.9f;
		float slope_threshold = 0.2f;
		float leaning_limit = 0.12f;
		float fixed_update_fps = 120;
		float gravity = -30;
		float water_vertical_offset = 0;
		float turning_speed = 10;
		XMFLOAT3 movement = XMFLOAT3(0, 0, 0);
		XMFLOAT3 velocity = XMFLOAT3(0, 0, 0);
		XMFLOAT3 inertia = XMFLOAT3(0, 0, 0);
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 position_prev = XMFLOAT3(0, 0, 0);
		XMFLOAT3 relative_offset = XMFLOAT3(0, 0, 0);
		bool active = true;
		float accumulator = 0; // fixed timestep accumulation
		float alpha = 0; // fixed timestep interpolation
		XMFLOAT3 facing = XMFLOAT3(0, 0, 1); // forward facing direction (smoothed)
		XMFLOAT3 facing_next = XMFLOAT3(0, 0, 1); // forward facing direction (immediate)
		float leaning = 0; // sideways leaning (smoothed)
		float leaning_next = 0; // sideways leaning (immediate)
		bool ground_intersect = false;
		bool wall_intersect = false;
		bool swimming = false;
		bool humanoid_checked = false;
		wi::ecs::Entity humanoidEntity = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity left_foot = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity right_foot = wi::ecs::INVALID_ENTITY;
		float foot_offset = 0;
		bool foot_placement_enabled = true;
		wi::PathQuery pathquery; // completed
		wi::vector<wi::ecs::Entity> animations;
		wi::ecs::Entity currentAnimation = wi::ecs::INVALID_ENTITY;
		float anim_timer = 0;
		float anim_amount = 1;
		bool reset_anim = true;
		bool anim_ended = true;
		XMFLOAT3 goal = XMFLOAT3(0, 0, 0);
		bool process_goal = false;
		struct PathfindingThreadContext
		{
			wi::jobsystem::context ctx;
			volatile long process_goal_completed = 0;
			wi::PathQuery pathquery_work; // working
			~PathfindingThreadContext()
			{
				wi::jobsystem::Wait(ctx);
			}
		};
		std::shared_ptr<PathfindingThreadContext> pathfinding_thread; // separate allocation, mustn't be reallocated while path finding thread is running
		const wi::VoxelGrid* voxelgrid = nullptr;
		float shake_horizontal = 0;
		float shake_vertical = 0;
		float shake_frequency = 0;
		float shake_decay = 0;
		float shake_timer = 0;

		// Apply movement to the character in the next update
		void Move(const XMFLOAT3& direction);
		// Apply movement relative to the character facing in the next update
		void Strafe(const XMFLOAT3& direction);
		// Apply upwards movement immediately
		void Jump(float amount);
		// Turn towards the direction smoothly
		void Turn(const XMFLOAT3& direction);
		// Lean sideways, negative values mean left, positive values mean right
		void Lean(float amount);
		// Apply shaking to the character
		//	horizontal, vertical: movement amount in directions
		//	frequency: speed of movement
		//	decay: speed of slowing down
		void Shake(float horizontal, float vertical = 0, float frequency = 100, float decay = 10);

		void AddAnimation(wi::ecs::Entity entity);
		void PlayAnimation(wi::ecs::Entity entity);
		void StopAnimation();
		void SetAnimationAmount(float amount);
		float GetAnimationAmount() const;
		float GetAnimationTimer() const;
		bool IsAnimationEnded() const;

		// Teleport character to position immediately
		void SetPosition(const XMFLOAT3& position);

		// Retrieve the current position without interpolation (this is the raw value from fixed timestep update)
		XMFLOAT3 GetPosition() const;

		// Retrieve the current position with interpolation (this is the position that is rendered)
		XMFLOAT3 GetPositionInterpolated() const;

		// Sets velocity immediately
		void SetVelocity(const XMFLOAT3& position);

		// Gets the current velocity
		XMFLOAT3 GetVelocity() const;

		// Returns the capsule representing the character, that is also used in collisions
		wi::primitive::Capsule GetCapsule() const;

		// Set the facing direction immediately
		void SetFacing(const XMFLOAT3& value);

		// Get the immediate facing direction
		XMFLOAT3 GetFacing() const;

		// Get the smoothed facing direction
		XMFLOAT3 GetFacingSmoothed() const;

		// Get the immediate leaning direction
		float GetLeaning() const;

		// Get the smoothed leaning direction
		float GetLeaningSmoothed() const;

		// Returns whether the character is currently stading on ground or not
		bool IsGrounded() const;

		// Returns whether the character is currently stading on ground or not
		bool IsWallIntersect() const;

		// Returns whether the character is currently swimming or not
		bool IsSwimming() const;

		// Enable/disable foot placement with inverse kinematics
		void SetFootPlacementEnabled(bool value);

		// Returns whether foot placement with inverse kinematics is currently enabled or not
		bool IsFootPlacementEnabled() const;

		// Set the goal for path finding, it will be processed the next time the scene is updated.
		//	You can get the results by accessing the pathquery object of the character.
		void SetPathGoal(const XMFLOAT3& goal, const wi::VoxelGrid* voxelgrid);

		// Enable/disable the character's processing
		void SetActive(bool value);

		// Returns whether character processing is active or not
		bool IsActive() const;

		bool IsCharacterToCharacterCollisionDisabled() const { return _flags & CHARACTER_TO_CHARACTER_COLLISION_DISABLED; }
		void SetCharacterToCharacterCollisionDisabled(bool value = true) { if (value) { _flags |= CHARACTER_TO_CHARACTER_COLLISION_DISABLED; } else { _flags &= ~CHARACTER_TO_CHARACTER_COLLISION_DISABLED; } }

		bool IsDedicatedShadow() const { return _flags & DEDICATED_SHADOW; }
		void SetDedicatedShadow(bool value = true) { if (value) { _flags |= DEDICATED_SHADOW; } else { _flags &= ~DEDICATED_SHADOW; } }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct SplineComponent
	{
		enum FLAGS
		{
			NONE = 0,
			DRAW_ALIGNED = 1 << 0,
			LOOPED = 1 << 1,
			DIRTY = 1 << 2,
		};
		uint32_t _flags = NONE;

		float width = 1; // overall width multiplier for all nodes (affects mesh generation)
		float rotation = 0; // rotation of nodes in radians around the spline axis (affects mesh generation)
		int mesh_generation_subdivision = 0; // increase this above 0 to request mesh generation
		int mesh_generation_vertical_subdivision = 0; // can create vertically subdivided mesh (corridoor, tunnel, etc. with this)
		float terrain_modifier_amount = 0; // increase above 0 to affect terrain generation (greater values increase the strength of the terrain trying to match the spline plane)
		float terrain_pushdown = 0; // push down the terrain below the spline plane (or above if negative value)
		float terrain_texture_falloff = 0.8f; // texture blend falloff for terrain modifier spline

		wi::vector<wi::ecs::Entity> spline_node_entities;

		// Non-serialized attributes:
		wi::vector<TransformComponent> spline_node_transforms;
		wi::vector<float> precomputed_node_distances;
		float precomputed_total_distance = 0;
		float prev_width = 1;
		float prev_rotation = 0;
		int prev_mesh_generation_subdivision = 0;
		int prev_mesh_generation_vertical_subdivision = 0;
		int prev_mesh_generation_nodes = 0;
		mutable float prev_terrain_modifier_amount = 0;
		mutable float prev_terrain_pushdown = 0;
		mutable float prev_terrain_texture_falloff = 0;
		mutable int prev_terrain_generation_nodes = 0;
		mutable bool dirty_terrain = false;
		bool prev_looped = false;
		wi::primitive::AABB aabb;
		wi::ecs::Entity materialEntity = wi::ecs::INVALID_ENTITY; // temp for terrain usage
		mutable wi::ecs::Entity materialEntity_terrainPrev = wi::ecs::INVALID_ENTITY; // temp for terrain usage

		// Evaluate an interpolated location on the spline at t which in range [0,1] on the spline
		//	the result matrix is oriented to look towards the spline direction and face upwards along the spline normal
		XMMATRIX EvaluateSplineAt(float t) const;

		// Get the closest point on the spline to a point
		XMVECTOR ClosestPointOnSpline(const XMVECTOR& P, int steps = 10) const;

		// Trace a point on the spline's plane:
		XMVECTOR TraceSplinePlane(const XMVECTOR& ORIGIN, const XMVECTOR& DIRECTION, int steps = 10) const;

		// Compute the boounding box of the spline iteratively
		wi::primitive::AABB ComputeAABB(int steps = 10) const;

		// Precompute the spline node distances that will be used at spline evaluation calls
		void PrecomputeSplineNodeDistances();

		// By default the spline is drawn as camera facing, this can be used to set it to be drawn aligned to segment rotations:
		bool IsDrawAligned() const { return _flags & DRAW_ALIGNED; }
		void SetDrawAligned(bool value = true) { if (value) { _flags |= DRAW_ALIGNED; } else { _flags &= ~DRAW_ALIGNED; } }

		bool IsLooped() const { return _flags & LOOPED; }
		void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }

		bool IsDirty() const { return _flags & DIRTY; }
		void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};
}
