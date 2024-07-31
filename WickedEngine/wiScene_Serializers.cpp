#include "wiScene.h"
#include "wiResourceManager.h"
#include "wiArchive.h"
#include "wiRandom.h"
#include "wiHelper.h"
#include "wiBacklog.h"
#include "wiTimer.h"
#include "wiVector.h"
#include "shaders/ShaderInterop_DDGI.h"

using namespace wi::ecs;

namespace wi::scene
{
	struct DEPRECATED_PreviousFrameTransformComponent
	{
		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri) { /*this never serialized any data*/ }
	};

	void NameComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> name;
		}
		else
		{
			archive << name;
		}
	}
	void LayerComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> layerMask;
		}
		else
		{
			archive << layerMask;
		}
	}
	void TransformComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> scale_local;
			archive >> rotation_local;
			archive >> translation_local;

			SetDirty();
			UpdateTransform();
		}
		else
		{
			archive << _flags; // maybe not needed just for dirtiness, but later might come handy if we have more persistent flags
			archive << scale_local;
			archive << rotation_local;
			archive << translation_local;
		}
	}
	void HierarchyComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		SerializeEntity(archive, parentID, seri);

		if (archive.IsReadMode())
		{
			archive >> layerMask_bind;
			if (archive.GetVersion() < 36)
			{
				XMFLOAT4X4 world_parent_inverse_bind;
				archive >> world_parent_inverse_bind;
			}
		}
		else
		{
			archive << layerMask_bind;
		}
	}
	void MaterialComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> (uint8_t&)engineStencilRef;
			archive >> userStencilRef;
			archive >> (uint8_t&)userBlendMode;
			archive >> baseColor;
			if (archive.GetVersion() >= 25)
			{
				archive >> emissiveColor;
			}
			archive >> texMulAdd;
			archive >> roughness;
			if (archive.GetVersion() < 35)
			{
				roughness = roughness == 0 ? 0 : std::sqrt(roughness);
			}
			archive >> reflectance;
			archive >> metalness;
			if (archive.GetVersion() < 25)
			{
				archive >> emissiveColor.w;
			}
			archive >> refraction;
			if (archive.GetVersion() < 52)
			{
				archive >> subsurfaceScattering.w;
			}
			archive >> normalMapStrength;
			archive >> parallaxOcclusionMapping;
			archive >> alphaRef;
			archive >> texAnimDirection;
			archive >> texAnimFrameRate;
			archive >> texAnimElapsedTime;

			archive >> textures[BASECOLORMAP].name;
			archive >> textures[SURFACEMAP].name;
			archive >> textures[NORMALMAP].name;
			archive >> textures[DISPLACEMENTMAP].name;

			if (archive.GetVersion() >= 24)
			{
				archive >> textures[EMISSIVEMAP].name;
			}

			if (archive.GetVersion() >= 28)
			{
				archive >> textures[OCCLUSIONMAP].name;

				archive >> textures[BASECOLORMAP].uvset;
				archive >> textures[SURFACEMAP].uvset;
				archive >> textures[NORMALMAP].uvset;
				archive >> textures[DISPLACEMENTMAP].uvset;
				archive >> textures[EMISSIVEMAP].uvset;
				archive >> textures[OCCLUSIONMAP].uvset;

				archive >> displacementMapping;
			}

			if (archive.GetVersion() >= 48)
			{
				archive >> (uint8_t&)shadingRate;
			}

			if (archive.GetVersion() >= 50)
			{
				archive >> (uint32_t&)shaderType;
				archive >> customShaderID;
			}
			else
			{
				if (_flags & _DEPRECATED_WATER)
				{
					shaderType = SHADERTYPE_WATER;
				}
				if (_flags & _DEPRECATED_PLANAR_REFLECTION)
				{
					shaderType = SHADERTYPE_PBR_PLANARREFLECTION;
				}
				if (parallaxOcclusionMapping > 0)
				{
					shaderType = SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING;
				}
			}

			if (archive.GetVersion() >= 52 && archive.GetVersion() < 54)
			{
				uint32_t subsurfaceProfile;
				archive >> subsurfaceProfile;
			}

			if (archive.GetVersion() >= 54)
			{
				archive >> subsurfaceScattering;
			}

			if (archive.GetVersion() >= 56)
			{
				archive >> specularColor;
			}

			if (archive.GetVersion() >= 59)
			{
				archive >> transmission;
				archive >> textures[TRANSMISSIONMAP].name;
				archive >> textures[TRANSMISSIONMAP].uvset;
			}

			if (archive.GetVersion() >= 61)
			{
				archive >> sheenColor;
				archive >> sheenRoughness;
				archive >> textures[SHEENCOLORMAP].name;
				archive >> textures[SHEENROUGHNESSMAP].name;
				archive >> textures[SHEENCOLORMAP].uvset;
				archive >> textures[SHEENROUGHNESSMAP].uvset;

				archive >> clearcoat;
				archive >> clearcoatRoughness;
				archive >> textures[CLEARCOATMAP].name;
				archive >> textures[CLEARCOATROUGHNESSMAP].name;
				archive >> textures[CLEARCOATNORMALMAP].name;
				archive >> textures[CLEARCOATMAP].uvset;
				archive >> textures[CLEARCOATROUGHNESSMAP].uvset;
				archive >> textures[CLEARCOATNORMALMAP].uvset;
			}

			if (archive.GetVersion() >= 68)
			{
				archive >> textures[SPECULARMAP].name;
				archive >> textures[SPECULARMAP].uvset;
			}

			if (seri.GetVersion() >= 1)
			{
				archive >> userdata;
			}

			if (seri.GetVersion() >= 2)
			{
				archive >> anisotropy_strength;
				archive >> anisotropy_rotation;
				archive >> textures[ANISOTROPYMAP].name;
				archive >> textures[ANISOTROPYMAP].uvset;
			}
			else
			{
				anisotropy_strength = parallaxOcclusionMapping; // old version fix
			}

			if (seri.GetVersion() >= 3)
			{
				archive >> textures[TRANSPARENCYMAP].name;
				archive >> textures[TRANSPARENCYMAP].uvset;
			}
			if (seri.GetVersion() >= 4)
			{
				archive >> blend_with_terrain_height;
			}
			if (seri.GetVersion() >= 5)
			{
				archive >> extinctionColor;
			}

			for (auto& x : textures)
			{
				if (!x.name.empty())
				{
					x.name = dir + x.name;
				}
			}

			wi::jobsystem::Execute(seri.ctx, [&](wi::jobsystem::JobArgs args) {
				CreateRenderData();
			});
		}
		else
		{
			for (auto& x : textures)
			{
				seri.RegisterResource(x.name);
			}

			archive << _flags;
			archive << (uint8_t)engineStencilRef;
			archive << userStencilRef;
			archive << (uint8_t)userBlendMode;
			archive << baseColor;
			if (archive.GetVersion() >= 25)
			{
				archive << emissiveColor;
			}
			archive << texMulAdd;
			archive << roughness;
			archive << reflectance;
			archive << metalness;
			if (archive.GetVersion() < 25)
			{
				archive << emissiveColor.w;
			}
			archive << refraction;
			if (archive.GetVersion() < 52)
			{
				float subsurfaceScattering = 0;
				archive << subsurfaceScattering;
			}
			archive << normalMapStrength;
			archive << parallaxOcclusionMapping;
			archive << alphaRef;
			archive << texAnimDirection;
			archive << texAnimFrameRate;
			archive << texAnimElapsedTime;

			archive << wi::helper::GetPathRelative(dir, textures[BASECOLORMAP].name);
			archive << wi::helper::GetPathRelative(dir, textures[SURFACEMAP].name);
			archive << wi::helper::GetPathRelative(dir, textures[NORMALMAP].name);
			archive << wi::helper::GetPathRelative(dir, textures[DISPLACEMENTMAP].name);

			if (archive.GetVersion() >= 24)
			{
				archive << wi::helper::GetPathRelative(dir, textures[EMISSIVEMAP].name);
			}

			if (archive.GetVersion() >= 28)
			{
				archive << wi::helper::GetPathRelative(dir, textures[OCCLUSIONMAP].name);

				archive << textures[BASECOLORMAP].uvset;
				archive << textures[SURFACEMAP].uvset;
				archive << textures[NORMALMAP].uvset;
				archive << textures[DISPLACEMENTMAP].uvset;
				archive << textures[EMISSIVEMAP].uvset;
				archive << textures[OCCLUSIONMAP].uvset;

				archive << displacementMapping;
			}

			if (archive.GetVersion() >= 48)
			{
				archive << (uint8_t)shadingRate;
			}

			if (archive.GetVersion() >= 50)
			{
				archive << (uint32_t)shaderType;
				archive << customShaderID;
			}

			if (archive.GetVersion() >= 54)
			{
				archive << subsurfaceScattering;
			}

			if (archive.GetVersion() >= 56)
			{
				archive << specularColor;
			}

			if (archive.GetVersion() >= 59)
			{
				archive << transmission;
				archive << wi::helper::GetPathRelative(dir, textures[TRANSMISSIONMAP].name);
				archive << textures[TRANSMISSIONMAP].uvset;
			}

			if (archive.GetVersion() >= 61)
			{
				archive << sheenColor;
				archive << sheenRoughness;
				archive << wi::helper::GetPathRelative(dir, textures[SHEENCOLORMAP].name);
				archive << wi::helper::GetPathRelative(dir, textures[SHEENROUGHNESSMAP].name);
				archive << textures[SHEENCOLORMAP].uvset;
				archive << textures[SHEENROUGHNESSMAP].uvset;

				archive << clearcoat;
				archive << clearcoatRoughness;
				archive << wi::helper::GetPathRelative(dir, textures[CLEARCOATMAP].name);
				archive << wi::helper::GetPathRelative(dir, textures[CLEARCOATROUGHNESSMAP].name);
				archive << wi::helper::GetPathRelative(dir, textures[CLEARCOATNORMALMAP].name);
				archive << textures[CLEARCOATMAP].uvset;
				archive << textures[CLEARCOATROUGHNESSMAP].uvset;
				archive << textures[CLEARCOATNORMALMAP].uvset;
			}

			if (archive.GetVersion() >= 68)
			{
				archive << wi::helper::GetPathRelative(dir, textures[SPECULARMAP].name);
				archive << textures[SPECULARMAP].uvset;
			}

			if (seri.GetVersion() >= 1)
			{
				archive << userdata;
			}

			if (seri.GetVersion() >= 2)
			{
				archive << anisotropy_strength;
				archive << anisotropy_rotation;
				archive << wi::helper::GetPathRelative(dir, textures[ANISOTROPYMAP].name);
				archive << textures[ANISOTROPYMAP].uvset;
			}

			if (seri.GetVersion() >= 3)
			{
				archive << wi::helper::GetPathRelative(dir, textures[TRANSPARENCYMAP].name);
				archive << textures[TRANSPARENCYMAP].uvset;
			}
			if (seri.GetVersion() >= 4)
			{
				archive << blend_with_terrain_height;
			}
			if (seri.GetVersion() >= 5)
			{
				archive << extinctionColor;
			}
		}
	}
	void MeshComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> vertex_positions;
			archive >> vertex_normals;
			archive >> vertex_uvset_0;
			archive >> vertex_boneindices;
			archive >> vertex_boneweights;
			archive >> vertex_atlas;
			archive >> vertex_colors;
			archive >> indices;

			size_t subsetCount;
			archive >> subsetCount;
			subsets.resize(subsetCount);
			for (size_t i = 0; i < subsetCount; ++i)
			{
				SerializeEntity(archive, subsets[i].materialID, seri);
				archive >> subsets[i].indexOffset;
				archive >> subsets[i].indexCount;
			}

			archive >> tessellationFactor;
			SerializeEntity(archive, armatureID, seri);

			if (archive.GetVersion() >= 28)
			{
				archive >> vertex_uvset_1;
			}

			if (archive.GetVersion() >= 41 && archive.GetVersion() < 79)
			{
				// These are no longer used:
				Entity terrain_material1;
				Entity terrain_material2;
				Entity terrain_material3;
				SerializeEntity(archive, terrain_material1, seri);
				SerializeEntity(archive, terrain_material2, seri);
				SerializeEntity(archive, terrain_material3, seri);
			}

			if (archive.GetVersion() >= 43)
			{
				archive >> vertex_windweights;
			}

			if (archive.GetVersion() >= 51)
			{
				archive >> vertex_tangents;
			}

			if (archive.GetVersion() >= 53)
			{
			    size_t targetCount;
			    archive >> targetCount;
			    morph_targets.resize(targetCount);
			    for (size_t i = 0; i < targetCount; ++i)
			    {
					archive >> morph_targets[i].vertex_positions;
					archive >> morph_targets[i].vertex_normals;
					archive >> morph_targets[i].weight;
					if (seri.GetVersion() >= 1)
					{
						archive >> morph_targets[i].sparse_indices_positions;
					}
					if (seri.GetVersion() >= 2)
					{
						archive >> morph_targets[i].sparse_indices_normals;
					}
			    }
			}

			if (archive.GetVersion() >= 76)
			{
				archive >> subsets_per_lod;
			}

			if (seri.GetVersion() >= 3)
			{
				archive >> vertex_boneindices2;
				archive >> vertex_boneweights2;
			}

			wi::jobsystem::Execute(seri.ctx, [&](wi::jobsystem::JobArgs args) {
				CreateRenderData();

				if (IsBVHEnabled())
				{
					BuildBVH();
				}
			});
		}
		else
		{
			archive << _flags;
			archive << vertex_positions;
			archive << vertex_normals;
			archive << vertex_uvset_0;
			archive << vertex_boneindices;
			archive << vertex_boneweights;
			archive << vertex_atlas;
			archive << vertex_colors;
			archive << indices;

			archive << subsets.size();
			for (size_t i = 0; i < subsets.size(); ++i)
			{
				SerializeEntity(archive, subsets[i].materialID, seri);
				archive << subsets[i].indexOffset;
				archive << subsets[i].indexCount;
			}

			archive << tessellationFactor;
			SerializeEntity(archive, armatureID, seri);

			if (archive.GetVersion() >= 28)
			{
				archive << vertex_uvset_1;
			}

			if (archive.GetVersion() >= 41 && archive.GetVersion() < 79)
			{
				// These are no longer used:
				Entity terrain_material1;
				Entity terrain_material2;
				Entity terrain_material3;
				SerializeEntity(archive, terrain_material1, seri);
				SerializeEntity(archive, terrain_material2, seri);
				SerializeEntity(archive, terrain_material3, seri);
			}

			if (archive.GetVersion() >= 43)
			{
				archive << vertex_windweights;
			}

			if (archive.GetVersion() >= 51)
			{
				archive << vertex_tangents;
			}

			if (archive.GetVersion() >= 53)
			{
			    archive << morph_targets.size();
			    for (size_t i = 0; i < morph_targets.size(); ++i)
			    {
					archive << morph_targets[i].vertex_positions;
					archive << morph_targets[i].vertex_normals;
					archive << morph_targets[i].weight;
					if (seri.GetVersion() >= 1)
					{
						archive << morph_targets[i].sparse_indices_positions;
					}
					if (seri.GetVersion() >= 2)
					{
						archive << morph_targets[i].sparse_indices_normals;
					}
			    }
			}

			if (archive.GetVersion() >= 76)
			{
				archive << subsets_per_lod;
			}

			if (seri.GetVersion() >= 3)
			{
				archive << vertex_boneindices2;
				archive << vertex_boneweights2;
			}

		}
	}
	void ImpostorComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> swapInDistance;

			SetDirty();
		}
		else
		{
			archive << _flags;
			archive << swapInDistance;
		}
	}
	void ObjectComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			SerializeEntity(archive, meshID, seri);
			archive >> cascadeMask;
			if (seri.GetVersion() >= 1)
			{
				archive >> filterMask;
			}
			else
			{
				uint32_t tmp;
				archive >> tmp;
			}
			archive >> color;

			if (archive.GetVersion() >= 23)
			{
				archive >> lightmapWidth;
				archive >> lightmapHeight;
				archive >> lightmapTextureData;
			}
			if (archive.GetVersion() >= 31)
			{
				archive >> userStencilRef;
			}
			if (archive.GetVersion() >= 60)
			{
				archive >> emissiveColor;
			}
			if (archive.GetVersion() >= 76)
			{
				archive >> lod_distance_multiplier;
			}
			if (archive.GetVersion() >= 80)
			{
				archive >> draw_distance;
			}
			if (seri.GetVersion() >= 2)
			{
				archive >> sort_priority;
			}
			if (seri.GetVersion() >= 3)
			{
				archive >> vertex_ao;
			}

			wi::jobsystem::Execute(seri.ctx, [&](wi::jobsystem::JobArgs args) {
				CreateRenderData();
			});
		}
		else
		{
			archive << _flags;
			SerializeEntity(archive, meshID, seri);
			archive << cascadeMask;
			if (seri.GetVersion() >= 1)
			{
				archive << filterMask;
			}
			archive << color;

			if (archive.GetVersion() >= 23)
			{
				archive << lightmapWidth;
				archive << lightmapHeight;
				archive << lightmapTextureData;
			}
			if (archive.GetVersion() >= 31)
			{
				archive << userStencilRef;
			}
			if (archive.GetVersion() >= 60)
			{
				archive << emissiveColor;
			}
			if (archive.GetVersion() >= 76)
			{
				archive << lod_distance_multiplier;
			}
			if (archive.GetVersion() >= 80)
			{
				archive << draw_distance;
			}
			if (seri.GetVersion() >= 2)
			{
				archive << sort_priority;
			}
			if (seri.GetVersion() >= 3)
			{
				archive << vertex_ao;
			}
		}
	}
	void RigidBodyPhysicsComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> (uint32_t&)shape;
			archive >> mass;
			archive >> friction;
			archive >> restitution;
			archive >> damping_linear;

			if (archive.GetVersion() >= 57)
			{
				archive >> damping_angular;
			}
			else
			{
				// these were previously untested:
				friction = 0.5f;
				restitution = 0.0f;
				damping_linear = 0;
			}

			if (archive.GetVersion() >= 58)
			{
				archive >> box.halfextents;
				archive >> sphere.radius;
				archive >> capsule.height;
				archive >> capsule.radius;
			}

			if (seri.GetVersion() >= 1)
			{
				archive >> mesh_lod;
			}
			if (seri.GetVersion() >= 2)
			{
				archive >> local_offset;
			}
			if (shape == CollisionShape::CYLINDER && seri.GetVersion() < 3)
			{
				// convert old assumption that was used in Bullet Physics:
				capsule.height = box.halfextents.y;
				capsule.radius = box.halfextents.x;
			}
			if (seri.GetVersion() >= 4)
			{
				archive >> buoyancy;
			}
		}
		else
		{
			archive << _flags;
			archive << (uint32_t&)shape;
			archive << mass;
			archive << friction;
			archive << restitution;
			archive << damping_linear;

			if (archive.GetVersion() >= 57)
			{
				archive << damping_angular;
			}

			if (archive.GetVersion() >= 58)
			{
				archive << box.halfextents;
				archive << sphere.radius;
				archive << capsule.height;
				archive << capsule.radius;
			}

			if (seri.GetVersion() >= 1)
			{
				archive << mesh_lod;
			}
			if (seri.GetVersion() >= 2)
			{
				archive << local_offset;
			}
			if (seri.GetVersion() >= 4)
			{
				archive << buoyancy;
			}
		}
	}
	void SoftBodyPhysicsComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		wi::vector<uint32_t> graphicsToPhysicsVertexMapping;

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> mass;
			archive >> friction;
			archive >> physicsToGraphicsVertexMapping;
			archive >> graphicsToPhysicsVertexMapping;
			archive >> weights;

			if (archive.GetVersion() >= 29 && archive.GetVersion() < 34)
			{
				wi::vector<XMFLOAT3> temp;
				archive >> temp;
			}

			if (archive.GetVersion() >= 57)
			{
				archive >> restitution;
			}
			else
			{
				// these were previously untested:
				friction = 0.5f;
			}

			if (seri.GetVersion() >= 1)
			{
				archive >> vertex_radius;
				archive >> detail;
			}
			else
			{
				SetWindEnabled(true);
			}

			if (seri.GetVersion() >= 2)
			{
				archive >> pressure;
			}
			else
			{
				// Convert weights from per-physics to per-graphics vertex:
				//	Per graphics vertex is better, because regenerating soft body with different detail will keep the pinning
				wi::vector<float> weights2(graphicsToPhysicsVertexMapping.size());
				for (size_t i = 0; i < weights2.size(); ++i)
				{
					weights2[i] = weights[graphicsToPhysicsVertexMapping[i]];
				}
				std::swap(weights, weights2);
			}

			if (seri.GetVersion() >= 3)
			{
				archive >> physicsIndices;
			}

			_flags &= ~SAFE_TO_REGISTER;
		}
		else
		{
			archive << _flags;
			archive << mass;
			archive << friction;
			archive << physicsToGraphicsVertexMapping;
			archive << graphicsToPhysicsVertexMapping;
			archive << weights;

			if (archive.GetVersion() >= 57)
			{
				archive << restitution;
			}

			if (seri.GetVersion() >= 1)
			{
				archive << vertex_radius;
				archive << detail;
			}
			if (seri.GetVersion() >= 2)
			{
				archive << pressure;
			}

			if (seri.GetVersion() >= 3)
			{
				archive << physicsIndices;
			}
		}
	}
	void ArmatureComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;

			size_t boneCount;
			archive >> boneCount;
			boneCollection.resize(boneCount);
			for (size_t i = 0; i < boneCount; ++i)
			{
				Entity boneID;
				SerializeEntity(archive, boneID, seri);
				boneCollection[i] = boneID;
			}

			archive >> inverseBindMatrices;

			if (archive.GetVersion() < 26)
			{
				XMFLOAT4X4 remapMatrix;
				archive >> remapMatrix; // no longer used
			}
			if (archive.GetVersion() == 26)
			{
				Entity rootBoneID;
				SerializeEntity(archive, rootBoneID, seri);
			}
		}
		else
		{
			archive << _flags;

			archive << boneCollection.size();
			for (size_t i = 0; i < boneCollection.size(); ++i)
			{
				Entity boneID = boneCollection[i];
				SerializeEntity(archive, boneID, seri);
			}

			archive << inverseBindMatrices;
			if (archive.GetVersion() == 26)
			{
				Entity rootBoneID;
				SerializeEntity(archive, rootBoneID, seri);
			}
		}
	}
	void LightComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> color;
			archive >> (uint32_t&)type;
			if (type > SPOT)
			{
				type = POINT; // fallback from old area light
			}
			archive >> intensity;
			archive >> range;
			archive >> outerConeAngle;
			if (archive.GetVersion() < 55)
			{
				float shadowBias;
				float radius;
				float width;
				float height;
				archive >> shadowBias;
				archive >> radius;
				archive >> width;
				archive >> height;
			}

			archive >> lensFlareNames;

			if (archive.GetVersion() >= 81)
			{
				archive >> forced_shadow_resolution;
			}

			if (archive.GetVersion() >= 82)
			{
				archive >> innerConeAngle;
			}

			if (archive.GetVersion() < 83)
			{
				// Conversion from old light units to physical light units:
				if (type != DIRECTIONAL)
				{
					BackCompatSetEnergy(intensity);
				}
				// Conversion from FOV to cone angle:
				outerConeAngle *= 0.5f;
				innerConeAngle *= 0.5f;
			}

			if (seri.GetVersion() >= 1)
			{
				archive >> cascade_distances;
			}
			if (seri.GetVersion() >= 2)
			{
				archive >> radius;
				archive >> length;
			}

			wi::jobsystem::Execute(seri.ctx, [&](wi::jobsystem::JobArgs args) {
				lensFlareRimTextures.resize(lensFlareNames.size());
				for (size_t i = 0; i < lensFlareNames.size(); ++i)
				{
					if (!lensFlareNames[i].empty())
					{
						lensFlareNames[i] = dir + lensFlareNames[i];
						lensFlareRimTextures[i] = wi::resourcemanager::Load(lensFlareNames[i]);
					}
				}
			});
		}
		else
		{
			for (auto& x : lensFlareNames)
			{
				seri.RegisterResource(x);
			}

			archive << _flags;
			archive << color;
			archive << (uint32_t)type;
			archive << intensity;
			archive << range;
			archive << outerConeAngle;
			if (archive.GetVersion() < 55)
			{
				float shadowBias = 0;
				float radius = 0;
				float width = 0;
				float height = 0;
				archive << shadowBias;
				archive << radius;
				archive << width;
				archive << height;
			}

			// If detecting an absolute path in textures, remove it and convert to relative:
			wi::vector<std::string> lensFlareNamesRelative = lensFlareNames;
			if (!dir.empty())
			{
				for (size_t i = 0; i < lensFlareNamesRelative.size(); ++i)
				{
					wi::helper::MakePathRelative(dir, lensFlareNamesRelative[i]);
				}
			}
			archive << lensFlareNamesRelative;

			if (archive.GetVersion() >= 81)
			{
				archive << forced_shadow_resolution;
			}

			if (archive.GetVersion() >= 82)
			{
				archive << innerConeAngle;
			}

			if (seri.GetVersion() >= 1)
			{
				archive << cascade_distances;
			}
			if (seri.GetVersion() >= 2)
			{
				archive << radius;
				archive << length;
			}
		}
	}
	void CameraComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> width;
			archive >> height;
			archive >> zNearP;
			archive >> zFarP;
			archive >> fov;

			if (archive.GetVersion() >= 65)
			{
				archive >> focal_length;
				archive >> aperture_size;
				archive >> aperture_shape;
			}

			SetDirty();
		}
		else
		{
			archive << _flags;
			archive << width;
			archive << height;
			archive << zNearP;
			archive << zFarP;
			archive << fov;

			if (archive.GetVersion() >= 65)
			{
				archive << focal_length;
				archive << aperture_size;
				archive << aperture_shape;
			}
		}
	}
	void EnvironmentProbeComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			SetDirty();

			if (seri.GetVersion() >= 1)
			{
				archive >> resolution;
				archive >> textureName;

				if (!textureName.empty())
				{
					textureName = dir + textureName;
					CreateRenderData();
				}
			}
		}
		else
		{
			seri.RegisterResource(textureName);

			archive << _flags;

			if (seri.GetVersion() >= 1)
			{
				archive << resolution;
				archive << wi::helper::GetPathRelative(dir, textureName);
			}
		}
	}
	void ForceFieldComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			uint32_t value;
			archive >> value;
			if (seri.GetVersion() < 1)
			{
				if (value == 200)
					value = 0;
				if (value == 201)
					value = 1;
			}
			type = (Type)value;
			archive >> gravity;
			archive >> range;
		}
		else
		{
			archive << _flags;
			archive << (uint32_t)type;
			archive << gravity;
			archive << range;
		}
	}
	void DecalComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;

			if (seri.GetVersion() >= 1)
			{
				archive >> slopeBlendPower;
			}
		}
		else
		{
			archive << _flags;

			if (seri.GetVersion() >= 1)
			{
				archive << slopeBlendPower;
			}
		}
	}
	void AnimationComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> start;
			archive >> end;
			archive >> timer;

			if (archive.GetVersion() >= 44)
			{
				archive >> amount;
			}
			if (archive.GetVersion() >= 47)
			{
				archive >> speed;
			}

			size_t channelCount;
			archive >> channelCount;
			channels.resize(channelCount);
			for (size_t i = 0; i < channelCount; ++i)
			{
				archive >> channels[i]._flags;
				archive >> (uint32_t&)channels[i].path;
				SerializeEntity(archive, channels[i].target, seri);
				archive >> channels[i].samplerIndex;
				if (seri.GetVersion() >= 1)
				{
					archive >> channels[i].retargetIndex;
				}
			}

			size_t samplerCount;
			archive >> samplerCount;
			samplers.resize(samplerCount);
			for (size_t i = 0; i < samplerCount; ++i)
			{
				archive >> samplers[i]._flags;
				archive >> (uint32_t&)samplers[i].mode;
				if (archive.GetVersion() < 46)
				{
					archive >> samplers[i].backwards_compatibility_data.keyframe_times;
					archive >> samplers[i].backwards_compatibility_data.keyframe_data;
				}
				if (archive.GetVersion() >= 46)
				{
					SerializeEntity(archive, samplers[i].data, seri);
				}
			}

			if (seri.GetVersion() >= 1)
			{
				size_t retargetCount;
				archive >> retargetCount;
				retargets.resize(retargetCount);
				for (size_t i = 0; i < retargetCount; ++i)
				{
					SerializeEntity(archive, retargets[i].source, seri);
					archive >> retargets[i].dstRelativeMatrix;
					archive >> retargets[i].srcRelativeParentMatrix;
				}
			}

		}
		else
		{
			archive << _flags;
			archive << start;
			archive << end;
			archive << timer;

			if (archive.GetVersion() >= 44)
			{
				archive << amount;
			}
			if (archive.GetVersion() >= 47)
			{
				archive << speed;
			}

			archive << channels.size();
			for (size_t i = 0; i < channels.size(); ++i)
			{
				archive << channels[i]._flags;
				archive << (uint32_t&)channels[i].path;
				SerializeEntity(archive, channels[i].target, seri);
				archive << channels[i].samplerIndex;
				if (seri.GetVersion() >= 1)
				{
					archive << channels[i].retargetIndex;
				}
			}

			archive << samplers.size();
			for (size_t i = 0; i < samplers.size(); ++i)
			{
				archive << samplers[i]._flags;
				archive << samplers[i].mode;
				SerializeEntity(archive, samplers[i].data, seri);
			}

			if (seri.GetVersion() >= 1)
			{
				archive << retargets.size();
				for (size_t i = 0; i < retargets.size(); ++i)
				{
					SerializeEntity(archive, retargets[i].source, seri);
					archive << retargets[i].dstRelativeMatrix;
					archive << retargets[i].srcRelativeParentMatrix;
				}
			}
		}


		if (seri.GetVersion() >= 2)
		{
			// Root Bone Name
			SerializeEntity(archive, rootMotionBone, seri);
		}
	}
	void AnimationDataComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> keyframe_times;
			archive >> keyframe_data;
		}
		else
		{
			archive << _flags;
			archive << keyframe_times;
			archive << keyframe_data;
		}
	}
	void WeatherComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		std::string dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> sunDirection;
			archive >> sunColor;
			archive >> horizon;
			archive >> zenith;
			archive >> ambient;
			archive >> fogStart;
			archive >> fogDensity;
			if (seri.GetVersion() < 3)
			{
				fogDensity = (1.0f / fogDensity);
			}
			if (archive.GetVersion() < 86)
			{
				float fogHeightSky;
				float cloudiness;
				float cloudScale;
				float cloudSpeed;
				archive >> fogHeightSky;
				archive >> cloudiness;
				archive >> cloudScale;
				archive >> cloudSpeed;
			}
			archive >> windDirection;
			archive >> windRandomness;
			archive >> windWaveSize;

			archive >> oceanParameters.dmap_dim;
			archive >> oceanParameters.patch_length;
			archive >> oceanParameters.time_scale;
			archive >> oceanParameters.wave_amplitude;
			archive >> oceanParameters.wind_dir;
			archive >> oceanParameters.wind_speed;
			archive >> oceanParameters.wind_dependency;
			archive >> oceanParameters.choppy_scale;
			if (archive.GetVersion() < 67)
			{
				XMFLOAT3 waterColor;
				archive >> waterColor;
				oceanParameters.waterColor.x = waterColor.x;
				oceanParameters.waterColor.y = waterColor.y;
				oceanParameters.waterColor.z = waterColor.z;
			}
			else
			{
				archive >> oceanParameters.waterColor;
			}
			archive >> oceanParameters.waterHeight;
			archive >> oceanParameters.surfaceDetail;
			archive >> oceanParameters.surfaceDisplacementTolerance;

			if (archive.GetVersion() >= 32)
			{
				archive >> skyMapName;
				if (!skyMapName.empty())
				{
					skyMapName = dir + skyMapName;
					skyMap = wi::resourcemanager::Load(skyMapName);
				}
			}
			if (archive.GetVersion() >= 40)
			{
				archive >> windSpeed;
			}
			if (archive.GetVersion() >= 62)
			{
				archive >> colorGradingMapName;
				if (!colorGradingMapName.empty())
				{
					colorGradingMapName = dir + colorGradingMapName;
					colorGradingMap = wi::resourcemanager::Load(colorGradingMapName, wi::resourcemanager::Flags::IMPORT_COLORGRADINGLUT);
				}
			}

			if (archive.GetVersion() >= 66)
			{
				archive >> skyExposure;

				archive >> atmosphereParameters.bottomRadius;
				archive >> atmosphereParameters.topRadius;
				archive >> atmosphereParameters.planetCenter;
				archive >> atmosphereParameters.rayleighDensityExpScale;
				archive >> atmosphereParameters.rayleighScattering;
				archive >> atmosphereParameters.mieDensityExpScale;
				archive >> atmosphereParameters.mieScattering;
				archive >> atmosphereParameters.mieExtinction;
				archive >> atmosphereParameters.mieAbsorption;
				archive >> atmosphereParameters.absorptionDensity0LayerWidth;
				archive >> atmosphereParameters.absorptionDensity0ConstantTerm;
				archive >> atmosphereParameters.absorptionDensity0LinearTerm;
				archive >> atmosphereParameters.absorptionDensity1ConstantTerm;
				archive >> atmosphereParameters.absorptionDensity1LinearTerm;
				archive >> atmosphereParameters.absorptionExtinction;
				archive >> atmosphereParameters.groundAlbedo;
			}

			if (archive.GetVersion() >= 70)
			{
				archive >> volumetricCloudParameters.layerFirst.albedo;
				archive >> volumetricCloudParameters.ambientGroundMultiplier;
				archive >> volumetricCloudParameters.layerFirst.extinctionCoefficient;
				archive >> volumetricCloudParameters.beerPowder;
				archive >> volumetricCloudParameters.beerPowderPower;
				archive >> volumetricCloudParameters.phaseG;
				archive >> volumetricCloudParameters.phaseG2;
				archive >> volumetricCloudParameters.phaseBlend;
				archive >> volumetricCloudParameters.multiScatteringScattering;
				archive >> volumetricCloudParameters.multiScatteringExtinction;
				archive >> volumetricCloudParameters.multiScatteringEccentricity;
				archive >> volumetricCloudParameters.shadowStepLength;
				archive >> volumetricCloudParameters.horizonBlendAmount;
				archive >> volumetricCloudParameters.horizonBlendPower;
				archive >> volumetricCloudParameters.layerFirst.rainAmount;
				archive >> volumetricCloudParameters.cloudStartHeight;
				archive >> volumetricCloudParameters.cloudThickness;
				archive >> volumetricCloudParameters.layerFirst.skewAlongWindDirection;
				archive >> volumetricCloudParameters.layerFirst.totalNoiseScale;
				archive >> volumetricCloudParameters.layerFirst.detailScale;
				archive >> volumetricCloudParameters.layerFirst.weatherScale;
				archive >> volumetricCloudParameters.layerFirst.curlScale;
				if (archive.GetVersion() < 86)
				{
					float ShapeNoiseHeightGradientAmount;
					float ShapeNoiseMultiplier;
					XMFLOAT2 ShapeNoiseMinMax;
					float ShapeNoisePower;
					archive >> ShapeNoiseHeightGradientAmount;
					archive >> ShapeNoiseMultiplier;
					archive >> ShapeNoiseMinMax;
					archive >> ShapeNoisePower;
				}
				archive >> volumetricCloudParameters.layerFirst.detailNoiseModifier;
				archive >> volumetricCloudParameters.layerFirst.detailNoiseHeightFraction;
				archive >> volumetricCloudParameters.layerFirst.curlNoiseModifier;
				archive >> volumetricCloudParameters.layerFirst.coverageAmount;
				archive >> volumetricCloudParameters.layerFirst.coverageMinimum;
				archive >> volumetricCloudParameters.layerFirst.typeAmount;
				archive >> volumetricCloudParameters.layerFirst.typeMinimum;
				if (archive.GetVersion() < 88)
				{
					float AnvilAmount;
					float AnvilOverhangHeight;
					archive >> AnvilAmount;
					archive >> AnvilOverhangHeight;
				}
				archive >> volumetricCloudParameters.animationMultiplier;
				archive >> volumetricCloudParameters.layerFirst.windSpeed;
				archive >> volumetricCloudParameters.layerFirst.windAngle;
				archive >> volumetricCloudParameters.layerFirst.windUpAmount;
				archive >> volumetricCloudParameters.layerFirst.coverageWindSpeed;
				archive >> volumetricCloudParameters.layerFirst.coverageWindAngle;
				archive >> volumetricCloudParameters.layerFirst.gradientSmall;
				archive >> volumetricCloudParameters.layerFirst.gradientMedium;
				archive >> volumetricCloudParameters.layerFirst.gradientLarge;
				archive >> volumetricCloudParameters.maxStepCount;
				archive >> volumetricCloudParameters.maxMarchingDistance;
				archive >> volumetricCloudParameters.inverseDistanceStepCount;
				archive >> volumetricCloudParameters.renderDistance;
				archive >> volumetricCloudParameters.LODDistance;
				archive >> volumetricCloudParameters.LODMin;
				archive >> volumetricCloudParameters.bigStepMarch;
				archive >> volumetricCloudParameters.transmittanceThreshold;
				archive >> volumetricCloudParameters.shadowSampleCount;
				archive >> volumetricCloudParameters.groundContributionSampleCount;

				if (archive.GetVersion() < 86)
				{
					volumetricCloudParameters.horizonBlendAmount *= 0.00001f;
					volumetricCloudParameters.layerFirst.totalNoiseScale *= 0.0004f;
					volumetricCloudParameters.layerFirst.weatherScale *= 0.0004f;
					volumetricCloudParameters.layerFirst.coverageAmount /= 2.0f;
					volumetricCloudParameters.layerFirst.coverageMinimum = std::max(0.0f, volumetricCloudParameters.layerFirst.coverageMinimum - 1.0f);
				}
			}

			if (archive.GetVersion() >= 71)
			{
				archive >> fogHeightStart;
				archive >> fogHeightEnd;
			}

			if (archive.GetVersion() >= 77 && archive.GetVersion() < 86)
			{
				float cloud_shadow_amount;
				float cloud_shadow_scale;
				float cloud_shadow_speed;
				archive >> cloud_shadow_amount;
				archive >> cloud_shadow_scale;
				archive >> cloud_shadow_speed;
			}

			if (archive.GetVersion() >= 78)
			{
				archive >> stars;
			}

			if (archive.GetVersion() >= 86)
			{
				archive >> volumetricCloudsWeatherMapFirstName;
				if (!volumetricCloudsWeatherMapFirstName.empty())
				{
					volumetricCloudsWeatherMapFirstName = dir + volumetricCloudsWeatherMapFirstName;
					volumetricCloudsWeatherMapFirst = wi::resourcemanager::Load(volumetricCloudsWeatherMapFirstName);
				}
			}

			if (archive.GetVersion() >= 88)
			{
				archive >> volumetricCloudsWeatherMapSecondName;
				if (!volumetricCloudsWeatherMapSecondName.empty())
				{
					volumetricCloudsWeatherMapSecondName = dir + volumetricCloudsWeatherMapSecondName;
					volumetricCloudsWeatherMapSecond = wi::resourcemanager::Load(volumetricCloudsWeatherMapSecondName);
				}

				archive >> volumetricCloudParameters.layerFirst.curlNoiseHeightFraction;
				archive >> volumetricCloudParameters.layerFirst.skewAlongCoverageWindDirection;
				archive >> volumetricCloudParameters.layerFirst.rainMinimum;
				archive >> volumetricCloudParameters.layerFirst.anvilDeformationSmall;
				archive >> volumetricCloudParameters.layerFirst.anvilDeformationMedium;
				archive >> volumetricCloudParameters.layerFirst.anvilDeformationLarge;

				archive >> volumetricCloudParameters.layerSecond.albedo;
				archive >> volumetricCloudParameters.layerSecond.extinctionCoefficient;
				archive >> volumetricCloudParameters.layerSecond.skewAlongWindDirection;
				archive >> volumetricCloudParameters.layerSecond.totalNoiseScale;
				archive >> volumetricCloudParameters.layerSecond.curlScale;
				archive >> volumetricCloudParameters.layerSecond.curlNoiseHeightFraction;
				archive >> volumetricCloudParameters.layerSecond.curlNoiseModifier;
				archive >> volumetricCloudParameters.layerSecond.detailScale;
				archive >> volumetricCloudParameters.layerSecond.detailNoiseHeightFraction;
				archive >> volumetricCloudParameters.layerSecond.detailNoiseModifier;
				archive >> volumetricCloudParameters.layerSecond.skewAlongCoverageWindDirection;
				archive >> volumetricCloudParameters.layerSecond.weatherScale;
				archive >> volumetricCloudParameters.layerSecond.coverageAmount;
				archive >> volumetricCloudParameters.layerSecond.coverageMinimum;
				archive >> volumetricCloudParameters.layerSecond.typeAmount;
				archive >> volumetricCloudParameters.layerSecond.typeMinimum;
				archive >> volumetricCloudParameters.layerSecond.rainAmount;
				archive >> volumetricCloudParameters.layerSecond.rainMinimum;
				archive >> volumetricCloudParameters.layerSecond.gradientSmall;
				archive >> volumetricCloudParameters.layerSecond.gradientMedium;
				archive >> volumetricCloudParameters.layerSecond.gradientLarge;
				archive >> volumetricCloudParameters.layerSecond.anvilDeformationSmall;
				archive >> volumetricCloudParameters.layerSecond.anvilDeformationMedium;
				archive >> volumetricCloudParameters.layerSecond.anvilDeformationLarge;
				archive >> volumetricCloudParameters.layerSecond.windSpeed;
				archive >> volumetricCloudParameters.layerSecond.windAngle;
				archive >> volumetricCloudParameters.layerSecond.windUpAmount;
				archive >> volumetricCloudParameters.layerSecond.coverageWindSpeed;
				archive >> volumetricCloudParameters.layerSecond.coverageWindAngle;
			}

			if (seri.GetVersion() >= 1)
			{
				archive >> gravity;
			}

			if (seri.GetVersion() >= 2)
			{
				archive >> atmosphereParameters.rayMarchMinMaxSPP;
				archive >> atmosphereParameters.distanceSPPMaxInv;
				archive >> atmosphereParameters.aerialPerspectiveScale;
			}
			if (seri.GetVersion() >= 4)
			{
				archive >> sky_rotation;
			}
			if (seri.GetVersion() >= 5)
			{
				archive >> rain_amount;
				archive >> rain_length;
				archive >> rain_speed;
				archive >> rain_scale;
				archive >> rain_splash_scale;
				archive >> rain_color;
			}
			if (seri.GetVersion() >= 6)
			{
				archive >> oceanParameters.extinctionColor;
			}
		}
		else
		{
			seri.RegisterResource(skyMapName);
			seri.RegisterResource(colorGradingMapName);
			seri.RegisterResource(volumetricCloudsWeatherMapFirstName);
			seri.RegisterResource(volumetricCloudsWeatherMapSecondName);

			archive << _flags;
			archive << sunDirection;
			archive << sunColor;
			archive << horizon;
			archive << zenith;
			archive << ambient;
			archive << fogStart;
			archive << fogDensity;
			archive << windDirection;
			archive << windRandomness;
			archive << windWaveSize;

			archive << oceanParameters.dmap_dim;
			archive << oceanParameters.patch_length;
			archive << oceanParameters.time_scale;
			archive << oceanParameters.wave_amplitude;
			archive << oceanParameters.wind_dir;
			archive << oceanParameters.wind_speed;
			archive << oceanParameters.wind_dependency;
			archive << oceanParameters.choppy_scale;
			archive << oceanParameters.waterColor;
			archive << oceanParameters.waterHeight;
			archive << oceanParameters.surfaceDetail;
			archive << oceanParameters.surfaceDisplacementTolerance;

			if (archive.GetVersion() >= 32)
			{
				archive << wi::helper::GetPathRelative(dir, skyMapName);
			}
			if (archive.GetVersion() >= 40)
			{
				archive << windSpeed;
			}
			if (archive.GetVersion() >= 62)
			{
				archive << wi::helper::GetPathRelative(dir, colorGradingMapName);
			}

			if (archive.GetVersion() >= 66)
			{
				archive << skyExposure;

				archive << atmosphereParameters.bottomRadius;
				archive << atmosphereParameters.topRadius;
				archive << atmosphereParameters.planetCenter;
				archive << atmosphereParameters.rayleighDensityExpScale;
				archive << atmosphereParameters.rayleighScattering;
				archive << atmosphereParameters.mieDensityExpScale;
				archive << atmosphereParameters.mieScattering;
				archive << atmosphereParameters.mieExtinction;
				archive << atmosphereParameters.mieAbsorption;
				archive << atmosphereParameters.absorptionDensity0LayerWidth;
				archive << atmosphereParameters.absorptionDensity0ConstantTerm;
				archive << atmosphereParameters.absorptionDensity0LinearTerm;
				archive << atmosphereParameters.absorptionDensity1ConstantTerm;
				archive << atmosphereParameters.absorptionDensity1LinearTerm;
				archive << atmosphereParameters.absorptionExtinction;
				archive << atmosphereParameters.groundAlbedo;
			}

			if (archive.GetVersion() >= 70)
			{
				archive << volumetricCloudParameters.layerFirst.albedo;
				archive << volumetricCloudParameters.ambientGroundMultiplier;
				archive << volumetricCloudParameters.layerFirst.extinctionCoefficient;
				archive << volumetricCloudParameters.beerPowder;
				archive << volumetricCloudParameters.beerPowderPower;
				archive << volumetricCloudParameters.phaseG;
				archive << volumetricCloudParameters.phaseG2;
				archive << volumetricCloudParameters.phaseBlend;
				archive << volumetricCloudParameters.multiScatteringScattering;
				archive << volumetricCloudParameters.multiScatteringExtinction;
				archive << volumetricCloudParameters.multiScatteringEccentricity;
				archive << volumetricCloudParameters.shadowStepLength;
				archive << volumetricCloudParameters.horizonBlendAmount;
				archive << volumetricCloudParameters.horizonBlendPower;
				archive << volumetricCloudParameters.layerFirst.rainAmount;
				archive << volumetricCloudParameters.cloudStartHeight;
				archive << volumetricCloudParameters.cloudThickness;
				archive << volumetricCloudParameters.layerFirst.skewAlongWindDirection;
				archive << volumetricCloudParameters.layerFirst.totalNoiseScale;
				archive << volumetricCloudParameters.layerFirst.detailScale;
				archive << volumetricCloudParameters.layerFirst.weatherScale;
				archive << volumetricCloudParameters.layerFirst.curlScale;
				if (archive.GetVersion() < 86)
				{
					float ShapeNoiseHeightGradientAmount = 0.0f;
					float ShapeNoiseMultiplier = 0.0f;
					XMFLOAT2 ShapeNoiseMinMax = XMFLOAT2(0.0f, 0.0f);
					float ShapeNoisePower = 0.0f;
					archive << ShapeNoiseHeightGradientAmount;
					archive << ShapeNoiseMultiplier;
					archive << ShapeNoiseMinMax;
					archive << ShapeNoisePower;
				}
				archive << volumetricCloudParameters.layerFirst.detailNoiseModifier;
				archive << volumetricCloudParameters.layerFirst.detailNoiseHeightFraction;
				archive << volumetricCloudParameters.layerFirst.curlNoiseModifier;
				archive << volumetricCloudParameters.layerFirst.coverageAmount;
				archive << volumetricCloudParameters.layerFirst.coverageMinimum;
				archive << volumetricCloudParameters.layerFirst.typeAmount;
				archive << volumetricCloudParameters.layerFirst.typeMinimum;
				if (archive.GetVersion() < 88)
				{
					float AnvilAmount = 0.0f;
					float AnvilOverhangHeight = 0.0f;
					archive << AnvilAmount;
					archive << AnvilOverhangHeight;
				}
				archive << volumetricCloudParameters.animationMultiplier;
				archive << volumetricCloudParameters.layerFirst.windSpeed;
				archive << volumetricCloudParameters.layerFirst.windAngle;
				archive << volumetricCloudParameters.layerFirst.windUpAmount;
				archive << volumetricCloudParameters.layerFirst.coverageWindSpeed;
				archive << volumetricCloudParameters.layerFirst.coverageWindAngle;
				archive << volumetricCloudParameters.layerFirst.gradientSmall;
				archive << volumetricCloudParameters.layerFirst.gradientMedium;
				archive << volumetricCloudParameters.layerFirst.gradientLarge;
				archive << volumetricCloudParameters.maxStepCount;
				archive << volumetricCloudParameters.maxMarchingDistance;
				archive << volumetricCloudParameters.inverseDistanceStepCount;
				archive << volumetricCloudParameters.renderDistance;
				archive << volumetricCloudParameters.LODDistance;
				archive << volumetricCloudParameters.LODMin;
				archive << volumetricCloudParameters.bigStepMarch;
				archive << volumetricCloudParameters.transmittanceThreshold;
				archive << volumetricCloudParameters.shadowSampleCount;
				archive << volumetricCloudParameters.groundContributionSampleCount;
			}

			if (archive.GetVersion() >= 71)
			{
				archive << fogHeightStart;
				archive << fogHeightEnd;
			}

			if (archive.GetVersion() >= 78)
			{
				archive << stars;
			}

			if (archive.GetVersion() >= 86)
			{
				archive << wi::helper::GetPathRelative(dir, volumetricCloudsWeatherMapFirstName);
			}

			if (archive.GetVersion() >= 88)
			{
				archive << wi::helper::GetPathRelative(dir, volumetricCloudsWeatherMapSecondName);

				archive << volumetricCloudParameters.layerFirst.curlNoiseHeightFraction;
				archive << volumetricCloudParameters.layerFirst.skewAlongCoverageWindDirection;
				archive << volumetricCloudParameters.layerFirst.rainMinimum;
				archive << volumetricCloudParameters.layerFirst.anvilDeformationSmall;
				archive << volumetricCloudParameters.layerFirst.anvilDeformationMedium;
				archive << volumetricCloudParameters.layerFirst.anvilDeformationLarge;

				archive << volumetricCloudParameters.layerSecond.albedo;
				archive << volumetricCloudParameters.layerSecond.extinctionCoefficient;
				archive << volumetricCloudParameters.layerSecond.skewAlongWindDirection;
				archive << volumetricCloudParameters.layerSecond.totalNoiseScale;
				archive << volumetricCloudParameters.layerSecond.curlScale;
				archive << volumetricCloudParameters.layerSecond.curlNoiseHeightFraction;
				archive << volumetricCloudParameters.layerSecond.curlNoiseModifier;
				archive << volumetricCloudParameters.layerSecond.detailScale;
				archive << volumetricCloudParameters.layerSecond.detailNoiseHeightFraction;
				archive << volumetricCloudParameters.layerSecond.detailNoiseModifier;
				archive << volumetricCloudParameters.layerSecond.skewAlongCoverageWindDirection;
				archive << volumetricCloudParameters.layerSecond.weatherScale;
				archive << volumetricCloudParameters.layerSecond.coverageAmount;
				archive << volumetricCloudParameters.layerSecond.coverageMinimum;
				archive << volumetricCloudParameters.layerSecond.typeAmount;
				archive << volumetricCloudParameters.layerSecond.typeMinimum;
				archive << volumetricCloudParameters.layerSecond.rainAmount;
				archive << volumetricCloudParameters.layerSecond.rainMinimum;
				archive << volumetricCloudParameters.layerSecond.gradientSmall;
				archive << volumetricCloudParameters.layerSecond.gradientMedium;
				archive << volumetricCloudParameters.layerSecond.gradientLarge;
				archive << volumetricCloudParameters.layerSecond.anvilDeformationSmall;
				archive << volumetricCloudParameters.layerSecond.anvilDeformationMedium;
				archive << volumetricCloudParameters.layerSecond.anvilDeformationLarge;
				archive << volumetricCloudParameters.layerSecond.windSpeed;
				archive << volumetricCloudParameters.layerSecond.windAngle;
				archive << volumetricCloudParameters.layerSecond.windUpAmount;
				archive << volumetricCloudParameters.layerSecond.coverageWindSpeed;
				archive << volumetricCloudParameters.layerSecond.coverageWindAngle;
			}

			if (seri.GetVersion() >= 1)
			{
				archive << gravity;
			}

			if (seri.GetVersion() >= 2)
			{
				archive << atmosphereParameters.rayMarchMinMaxSPP;
				archive << atmosphereParameters.distanceSPPMaxInv;
				archive << atmosphereParameters.aerialPerspectiveScale;
			}
			if (seri.GetVersion() >= 4)
			{
				archive << sky_rotation;
			}
			if (seri.GetVersion() >= 5)
			{
				archive << rain_amount;
				archive << rain_length;
				archive << rain_speed;
				archive << rain_scale;
				archive << rain_splash_scale;
				archive << rain_color;
			}
			if (seri.GetVersion() >= 6)
			{
				archive << oceanParameters.extinctionColor;
			}
		}
	}
	void SoundComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> filename;
			archive >> volume;
			archive >> (uint32_t&)soundinstance.type;

			if (seri.GetVersion() >= 1)
			{
				archive >> soundinstance.begin;
				archive >> soundinstance.length;
				archive >> soundinstance.loop_begin;
				archive >> soundinstance.loop_length;
			}

			wi::jobsystem::Execute(seri.ctx, [&](wi::jobsystem::JobArgs args) {
				if (!filename.empty())
				{
					filename = dir + filename;
					soundResource = wi::resourcemanager::Load(filename);
					// Note: sound instance can't be created yet, as soundResource is not necessarily ready at this point
					//	Consider when multiple threads are loading the same sound, one thread will be loading the data,
					//	the others return early with the resource that will be containing the data once it has been loaded.
				}
			});
		}
		else
		{
			seri.RegisterResource(filename);

			archive << _flags;
			archive << wi::helper::GetPathRelative(dir, filename);
			archive << volume;
			archive << soundinstance.type;

			if (seri.GetVersion() >= 1)
			{
				archive << soundinstance.begin;
				archive << soundinstance.length;
				archive << soundinstance.loop_begin;
				archive << soundinstance.loop_length;
			}
		}
	}
	void VideoComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> filename;

			wi::jobsystem::Execute(seri.ctx, [&](wi::jobsystem::JobArgs args) {
				if (!filename.empty())
				{
					filename = dir + filename;
					videoResource = wi::resourcemanager::Load(filename);
					wi::video::CreateVideoInstance(&videoResource.GetVideo(), &videoinstance);
				}
			});
		}
		else
		{
			seri.RegisterResource(filename);

			archive << _flags;
			archive << wi::helper::GetPathRelative(dir, filename);
		}
	}
	void InverseKinematicsComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		SerializeEntity(archive, target, seri);

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> chain_length;
			archive >> iteration_count;
		}
		else
		{
			archive << _flags;
			archive << chain_length;
			archive << iteration_count;
		}
	}
	void SpringComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> stiffnessForce;
			archive >> dragForce;
			archive >> windForce;

			if (seri.GetVersion() >= 1)
			{
				archive >> hitRadius;
				archive >> gravityPower;
				archive >> gravityDir;
			}

			Reset();
		}
		else
		{
			archive << _flags;
			archive << stiffnessForce;
			archive << dragForce;
			archive << windForce;

			if (seri.GetVersion() >= 1)
			{
				archive << hitRadius;
				archive << gravityPower;
				archive << gravityDir;
			}
		}
	}
	void ColliderComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			int ishape = 0;
			archive >> ishape;
			shape = (Shape)ishape;
			if (seri.GetVersion() < 1)
			{
				Entity transformID;
				SerializeEntity(archive, transformID, seri);
			}
			archive >> radius;
			archive >> offset;
			archive >> tail;

			if (seri.GetVersion() < 2)
			{
				SetCPUEnabled(true);
				SetGPUEnabled(true);
			}
		}
		else
		{
			archive << _flags;
			archive << (int)shape;
			archive << radius;
			archive << offset;
			archive << tail;
		}
	}
	void ScriptComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> filename;

			if (IsPlayingOnlyOnce())
			{
				Play();
			}

			wi::jobsystem::Execute(seri.ctx, [&](wi::jobsystem::JobArgs args) {
				CreateFromFile(dir + filename);
				});
		}
		else
		{
			seri.RegisterResource(filename);

			archive << _flags;
			archive << wi::helper::GetPathRelative(dir, filename);
		}
	}
	void ExpressionComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			for (int& index : presets)
			{
				archive >> index;
			}
			archive >> blink_frequency;
			archive >> blink_length;
			archive >> blink_count;
			archive >> look_frequency;
			archive >> look_length;

			size_t expression_count = 0;
			archive >> expression_count;
			expressions.resize(expression_count);
			for (size_t expression_index = 0; expression_index < expression_count; ++expression_index)
			{
				Expression& expression = expressions[expression_index];
				archive >> expression.name;
				if (expression.preset == ExpressionComponent::Preset::Count && expression.name.compare("Surprised") == 0)
				{
					// Vroid was not exporting Suprised expression properly and it was not handled at import for some models:
					expression.preset = ExpressionComponent::Preset::Surprised;
					presets[size_t(ExpressionComponent::Preset::Surprised)] = int(expression_index);
				}
				archive >> expression.weight;

				uint32_t value = 0;
				archive >> value;
				expression.preset = (Preset)value;

				archive >> value;
				expression.override_mouth = (Override)value;
				archive >> value;
				expression.override_blink = (Override)value;
				archive >> value;
				expression.override_look = (Override)value;

				size_t count = 0;
				archive >> count;
				expression.morph_target_bindings.resize(count);
				for (size_t i = 0; i < count; ++i)
				{
					SerializeEntity(archive, expression.morph_target_bindings[i].meshID, seri);
					archive >> expression.morph_target_bindings[i].index;
					archive >> expression.morph_target_bindings[i].weight;
				}

				expression.SetDirty();
			}
		}
		else
		{
			archive << _flags;
			for (int index : presets)
			{
				archive << index;
			}
			archive << blink_frequency;
			archive << blink_length;
			archive << blink_count;
			archive << look_frequency;
			archive << look_length;

			archive << expressions.size();
			for (size_t expression_index = 0; expression_index < expressions.size(); ++expression_index)
			{
				Expression& expression = expressions[expression_index];
				archive << expression.name;
				archive << expression.weight;

				archive << (uint32_t)expression.preset;
				archive << (uint32_t)expression.override_mouth;
				archive << (uint32_t)expression.override_blink;
				archive << (uint32_t)expression.override_look;

				archive << expression.morph_target_bindings.size();
				for (size_t i = 0; i < expression.morph_target_bindings.size(); ++i)
				{
					SerializeEntity(archive, expression.morph_target_bindings[i].meshID, seri);
					archive << expression.morph_target_bindings[i].index;
					archive << expression.morph_target_bindings[i].weight;
				}
			}
		}
	}
	void HumanoidComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		XMFLOAT3 default_look_direction = XMFLOAT3(0, 0, 1);
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> default_look_direction;
			archive >> head_rotation_max;
			archive >> head_rotation_speed;
			archive >> eye_rotation_max;
			archive >> eye_rotation_speed;

			for (auto& entity : bones)
			{
				SerializeEntity(archive, entity, seri);
			}

			if (seri.GetVersion() >= 1)
			{
				archive >> ragdoll_fatness;
				archive >> ragdoll_headsize;
			}
		}
		else
		{
			archive << _flags;
			archive << default_look_direction;
			archive << head_rotation_max;
			archive << head_rotation_speed;
			archive << eye_rotation_max;
			archive << eye_rotation_speed;

			for (auto& entity : bones)
			{
				SerializeEntity(archive, entity, seri);
			}

			if (seri.GetVersion() >= 1)
			{
				archive << ragdoll_fatness;
				archive << ragdoll_headsize;
			}
		}
	}
	void MetadataComponent::Serialize(wi::Archive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;

			int intpreset = 0;
			archive >> intpreset;
			preset = (Preset)intpreset;

			size_t count = 0;

			archive >> count;
			bool_values.reserve(count);
			for (size_t i = 0; i < count; ++i)
			{
				std::string name;
				archive >> name;
				bool value;
				archive >> value;
				bool_values.set(name, value);
			}

			archive >> count;
			int_values.reserve(count);
			for (size_t i = 0; i < count; ++i)
			{
				std::string name;
				archive >> name;
				int value;
				archive >> value;
				int_values.set(name, value);
			}

			archive >> count;
			float_values.reserve(count);
			for (size_t i = 0; i < count; ++i)
			{
				std::string name;
				archive >> name;
				float value;
				archive >> value;
				float_values.set(name, value);
			}

			archive >> count;
			string_values.reserve(count);
			for (size_t i = 0; i < count; ++i)
			{
				std::string name;
				archive >> name;
				std::string value;
				archive >> value;
				string_values.set(name, value);
			}
		}
		else
		{
			archive << _flags;

			archive << (int)preset;

			archive << bool_values.lookup.size();
			for (auto& x : bool_values.lookup)
			{
				archive << x.first;
				archive << bool_values.get(x.first);
			}

			archive << int_values.lookup.size();
			for (auto& x : int_values.lookup)
			{
				archive << x.first;
				archive << int_values.get(x.first);
			}

			archive << float_values.lookup.size();
			for (auto& x : float_values.lookup)
			{
				archive << x.first;
				archive << float_values.get(x.first);
			}

			archive << string_values.lookup.size();
			for (auto& x : string_values.lookup)
			{
				archive << x.first;
				archive << string_values.get(x.first);
			}
		}
	}

	void Scene::Serialize(wi::Archive& archive)
	{
		wi::Timer timer;

		if (archive.IsReadMode())
		{
			uint32_t reserved;
			archive >> reserved;
		}
		else
		{
			uint32_t reserved = 0;
			archive << reserved;
		}

		// Manage jump position to jump to resource serialization WRITE area:
		size_t jump_before = 0;
		size_t jump_after = 0;
		size_t original_pos = 0;
		if (archive.GetVersion() >= 90)
		{
			if (archive.IsReadMode())
			{
				archive >> jump_before;
				archive >> jump_after;
				original_pos = archive.GetPos();
				archive.Jump(jump_before); // jump before resourcemanager::Serialize_WRITE
			}
			else
			{
				jump_before = archive.WriteUnknownJumpPosition();
				jump_after = archive.WriteUnknownJumpPosition();
			}
		}

		// Keeping this alive to keep serialized resources alive until entity serialization ends:
		wi::resourcemanager::ResourceSerializer resource_seri;
		if (archive.IsReadMode() && archive.GetVersion() >= 63)
		{
			wi::resourcemanager::Serialize_READ(archive, resource_seri);
			if (archive.GetVersion() >= 90)
			{
				// After resource serialization, jump back to entity serialization area:
				archive.Jump(original_pos);
			}
		}

		// With this we will ensure that serialized entities are unique and persistent across the scene:
		EntitySerializer seri;
		seri.ctx.priority = wi::jobsystem::Priority::Low; // serialization tasks will be low priority to not block rendering if scene loading is asynchronous

		if(archive.GetVersion() >= 84)
		{
			// New scene serialization path with component library:
			componentLibrary.Serialize(archive, seri);
		}
		else
		{
			// Old serialization path with hard coded component types:
			names.Serialize(archive, seri);
			layers.Serialize(archive, seri);
			transforms.Serialize(archive, seri);
			if (archive.GetVersion() < 75)
			{
				ComponentManager<DEPRECATED_PreviousFrameTransformComponent> prev_transforms;
				prev_transforms.Serialize(archive, seri);
			}
			hierarchy.Serialize(archive, seri);
			materials.Serialize(archive, seri);
			meshes.Serialize(archive, seri);
			impostors.Serialize(archive, seri);
			objects.Serialize(archive, seri);
			ComponentManager<wi::primitive::AABB> aabbs_tmp; // no longer needed from serializer
			aabbs_tmp.Serialize(archive, seri);
			rigidbodies.Serialize(archive, seri);
			softbodies.Serialize(archive, seri);
			armatures.Serialize(archive, seri);
			lights.Serialize(archive, seri);
			aabbs_tmp.Serialize(archive, seri);
			cameras.Serialize(archive, seri);
			probes.Serialize(archive, seri);
			aabbs_tmp.Serialize(archive, seri);
			forces.Serialize(archive, seri);
			decals.Serialize(archive, seri);
			aabbs_tmp.Serialize(archive, seri);
			animations.Serialize(archive, seri);
			emitters.Serialize(archive, seri);
			hairs.Serialize(archive, seri);
			weathers.Serialize(archive, seri);
			if (archive.GetVersion() >= 30)
			{
				sounds.Serialize(archive, seri);
			}
			if (archive.GetVersion() >= 37)
			{
				inverse_kinematics.Serialize(archive, seri);
			}
			if (archive.GetVersion() >= 38)
			{
				springs.Serialize(archive, seri);
			}
			if (archive.GetVersion() >= 46)
			{
				animation_datas.Serialize(archive, seri);
			}

			if (archive.GetVersion() < 46)
			{
				// Fixing the animation import from archive that didn't have separate animation data components:
				for (size_t i = 0; i < animations.GetCount(); ++i)
				{
					AnimationComponent& animation = animations[i];
					for (const AnimationComponent::AnimationChannel& channel : animation.channels)
					{
						assert(channel.samplerIndex < (int)animation.samplers.size());
						AnimationComponent::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
						if (sampler.data == INVALID_ENTITY)
						{
							// backwards-compatibility mode
							sampler.data = CreateEntity();
							animation_datas.Create(sampler.data) = sampler.backwards_compatibility_data;
							sampler.backwards_compatibility_data.keyframe_times.clear();
							sampler.backwards_compatibility_data.keyframe_data.clear();
						}
					}
				}
			}
		}

		// Additional data serializations:
		if (archive.GetVersion() >= 85)
		{
			ddgi.Serialize(archive);
		}

		wi::jobsystem::Wait(seri.ctx); // This is needed before emitter material fixup that is below, because material CreateRenderDatas might be pending!

		// Fixup old emittedparticle distortion basecolor slot -> normalmap slot
		if (archive.GetVersion() < 89)
		{
			for (size_t i = 0; i < emitters.GetCount(); ++i)
			{
				if (emitters[i].shaderType != EmittedParticleSystem::PARTICLESHADERTYPE::SOFT_DISTORTION)
					continue;
				Entity entity = emitters.GetEntity(i);
				MaterialComponent* material = materials.GetComponent(entity);
				if (material != nullptr)
				{
					material->textures[NORMALMAP] = std::move(material->textures[BASECOLORMAP]);
					material->CreateRenderData(true);
				}
			}
		}

		if (archive.GetVersion() >= 90)
		{
			if (archive.IsReadMode())
			{
				archive.Jump(jump_after); // jump after resourcemanager::Serialize_WRITE
			}
			else
			{
				archive.PatchUnknownJumpPosition(jump_before);
				wi::resourcemanager::Serialize_WRITE(archive, seri.resource_registration);
				archive.PatchUnknownJumpPosition(jump_after);
			}
		}

		char text[64] = {};
		snprintf(text, arraysize(text), "Scene::Serialize took %.2f seconds", timer.elapsed_seconds());
		wi::backlog::post(text);
	}

	void Scene::DDGI::Serialize(wi::Archive& archive)
	{
		using namespace wi::graphics;
		GraphicsDevice* device = GetDevice();

		if (archive.IsReadMode())
		{
			archive >> frame_index;
			archive >> grid_dimensions;

			if (archive.GetVersion() >= 87)
			{
				archive >> grid_min;
				archive >> grid_max;
				archive >> smooth_backface;
			}

			wi::vector<uint8_t> data;

			// color texture:
			archive >> data;
			if(!data.empty())
			{
				TextureDesc desc;
				desc.bind_flags = BindFlag::SHADER_RESOURCE;
				desc.width = DDGI_COLOR_TEXELS * grid_dimensions.x * grid_dimensions.y;
				desc.height = DDGI_COLOR_TEXELS * grid_dimensions.z;
				desc.format = Format::BC6H_UF16;
				const uint32_t num_blocks_x = desc.width / GetFormatBlockSize(desc.format);
				const uint32_t num_blocks_y = desc.height / GetFormatBlockSize(desc.format);
				if (data.size() == num_blocks_x * num_blocks_y * GetFormatStride(desc.format))
				{
					SubresourceData initdata;
					initdata.data_ptr = data.data();
					initdata.row_pitch = num_blocks_x * GetFormatStride(desc.format);

					device->CreateTexture(&desc, &initdata, &color_texture);
					device->SetName(&color_texture, "ddgi.color_texture[serialized]");
				}
				else
				{
					wi::backlog::post("The serialized DDGI irradiance data structure is different from current version, discarding irradiance data.", wi::backlog::LogLevel::Warning);
				}
			}

			// depth texture:
			archive >> data;
			if(!data.empty())
			{
				TextureDesc desc;
				desc.width = DDGI_DEPTH_TEXELS * grid_dimensions.x * grid_dimensions.y;
				desc.height = DDGI_DEPTH_TEXELS * grid_dimensions.z;
				desc.format = Format::R16G16_FLOAT;
				desc.bind_flags = BindFlag::SHADER_RESOURCE;

				SubresourceData initdata;
				initdata.data_ptr = data.data();
				initdata.row_pitch = desc.width * GetFormatStride(desc.format);

				device->CreateTexture(&desc, &initdata, &depth_texture);
				device->SetName(&depth_texture, "ddgi.depth_texture[serialized]");
			}

			// offset texture:
			archive >> data;
			if(!data.empty())
			{
				TextureDesc desc;
				desc.type = TextureDesc::Type::TEXTURE_3D;
				desc.width = grid_dimensions.x;
				desc.height = grid_dimensions.z;
				desc.depth = grid_dimensions.y;
				desc.format = Format::R10G10B10A2_UNORM;
				desc.bind_flags = BindFlag::SHADER_RESOURCE;

				const size_t required_size = ComputeTextureMemorySizeInBytes(desc);
				if (data.size() == required_size)
				{
					SubresourceData initdata;
					initdata.data_ptr = data.data();
					initdata.row_pitch = desc.width * GetFormatStride(desc.format);
					initdata.slice_pitch = initdata.row_pitch * desc.height;

					device->CreateTexture(&desc, &initdata, &offset_texture);
					device->SetName(&offset_texture, "ddgi.offset_texture[serialized]");
				}
				else
				{
					wi::backlog::post("The serialized DDGI probe offset structure is different from current version, discarding probe offset data.", wi::backlog::LogLevel::Warning);
				}
			}
		}
		else
		{
			archive << frame_index;
			archive << grid_dimensions;

			if (archive.GetVersion() >= 87)
			{
				archive << grid_min;
				archive << grid_max;
				archive << smooth_backface;
			}

			wi::vector<uint8_t> data;
			if (color_texture.IsValid())
			{
				bool success = wi::helper::saveTextureToMemory(color_texture, data);
				assert(success);
			}
			archive << data;

			data.clear();
			if (depth_texture.IsValid())
			{
				bool success = wi::helper::saveTextureToMemory(depth_texture, data);
				assert(success);
			}
			archive << data;

			data.clear();
			if (offset_texture.IsValid())
			{
				bool success = wi::helper::saveTextureToMemory(offset_texture, data);
				assert(success);
			}
			archive << data;
		}
	}

	Entity Entity_Serialize_Internal(
		Scene& scene,
		wi::Archive& archive,
		EntitySerializer& seri,
		Entity entity,
		Scene::EntitySerializeFlags flags
	)
	{
		SerializeEntity(archive, entity, seri);

		bool restore_remap = seri.allow_remap;
		if (has_flag(flags, Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES))
		{
			seri.allow_remap = false;
		}
		
		if (archive.GetVersion() >= 84)
		{
			// New entity serialization path with component library:
			scene.componentLibrary.Entity_Serialize(entity, archive, seri);

			if (archive.IsReadMode())
			{
				// Wait the job system, because from this point, component managers could be resized
				//	due to more serialization tasks in recursive operation
				//	The pointers must not be invalidated while serialization jobs are not finished
				wi::jobsystem::Wait(seri.ctx);

				if (archive.GetVersion() >= 72 && has_flag(flags, Scene::EntitySerializeFlags::RECURSIVE))
				{
					// serialize children:
					seri.allow_remap = restore_remap;
					size_t childCount = 0;
					archive >> childCount;
					for (size_t i = 0; i < childCount; ++i)
					{
						Entity child = Entity_Serialize_Internal(scene, archive, seri, INVALID_ENTITY, flags);
						if (child != INVALID_ENTITY)
						{
							HierarchyComponent* hier = scene.hierarchy.GetComponent(child);
							if (hier != nullptr)
							{
								hier->parentID = entity;
							}
						}
					}
				}
			}
			else
			{
				// Wait the job system, because from this point, component managers could be resized
				//	due to more serialization tasks in recursive operation
				//	The pointers must not be invalidated while serialization jobs are not finished
				wi::jobsystem::Wait(seri.ctx);

				if (archive.GetVersion() >= 72 && has_flag(flags, Scene::EntitySerializeFlags::RECURSIVE))
				{
					// Recursive serialization for all children:
					seri.allow_remap = restore_remap;
					wi::vector<Entity> children;
					for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
					{
						const HierarchyComponent& hier = scene.hierarchy[i];
						if (hier.parentID == entity)
						{
							Entity child = scene.hierarchy.GetEntity(i);
							children.push_back(child);
						}
					}
					archive << children.size();
					for (Entity child : children)
					{
						Entity_Serialize_Internal(scene, archive, seri, child, flags);
					}
				}
			}

		}
		else
		{
			// Old entity serialization path code for hard coded component types:
			if (archive.IsReadMode())
			{
				// Check for each components if it exists, and if yes, READ it:
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.names.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.layers.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.transforms.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				if(archive.GetVersion() < 75)
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						ComponentManager<DEPRECATED_PreviousFrameTransformComponent> prev_transforms;
						auto& component = prev_transforms.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.hierarchy.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.materials.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.meshes.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.impostors.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.objects.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto component = wi::primitive::AABB(); // no longer needed to be serialized
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.rigidbodies.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.softbodies.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.armatures.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.lights.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto component = wi::primitive::AABB(); // no longer needed to be serialized
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.cameras.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.probes.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto component = wi::primitive::AABB(); // no longer needed to be serialized
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.forces.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.decals.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto component = wi::primitive::AABB(); // no longer needed to be serialized
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.animations.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.emitters.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.hairs.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.weathers.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				if (archive.GetVersion() >= 30)
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.sounds.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				if (archive.GetVersion() >= 37)
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.inverse_kinematics.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				if (archive.GetVersion() >= 38)
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.springs.Create(entity);
						component.Serialize(archive, seri);
					}
				}
				if (archive.GetVersion() >= 46)
				{
					bool component_exists;
					archive >> component_exists;
					if (component_exists)
					{
						auto& component = scene.animation_datas.Create(entity);
						component.Serialize(archive, seri);
					}
				}

				// Wait the job system, because from this point, component managers could be resized
				//	due to more serialization tasks in recursive operation
				//	The pointers must not be invalidated while serialization jobs are not finished
				wi::jobsystem::Wait(seri.ctx);

				if (archive.GetVersion() >= 72 && has_flag(flags, Scene::EntitySerializeFlags::RECURSIVE))
				{
					// serialize children:
					seri.allow_remap = restore_remap;
					size_t childCount = 0;
					archive >> childCount;
					for (size_t i = 0; i < childCount; ++i)
					{
						Entity child = Entity_Serialize_Internal(scene, archive, seri, INVALID_ENTITY, flags);
						if (child != INVALID_ENTITY)
						{
							HierarchyComponent* hier = scene.hierarchy.GetComponent(child);
							if (hier != nullptr)
							{
								hier->parentID = entity;
							}
						}
					}
				}
			}
			else
			{
				// Find existing components one-by-one and WRITE them out:
				{
					auto component = scene.names.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.layers.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.transforms.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.hierarchy.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.materials.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.meshes.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.impostors.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.objects.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					archive << false; // aabb no longer needed to be serialized
				}
				{
					auto component = scene.rigidbodies.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.softbodies.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.armatures.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.lights.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					archive << false; // aabb no longer needed to be serialized
				}
				{
					auto component = scene.cameras.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.probes.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					archive << false; // aabb no longer needed to be serialized
				}
				{
					auto component = scene.forces.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.decals.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					archive << false; // aabb no longer needed to be serialized
				}
				{
					auto component = scene.animations.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.emitters.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.hairs.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				{
					auto component = scene.weathers.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				if(archive.GetVersion() >= 30)
				{
					auto component = scene.sounds.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				if (archive.GetVersion() >= 37)
				{
					auto component = scene.inverse_kinematics.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				if (archive.GetVersion() >= 38)
				{
					auto component = scene.springs.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}
				if (archive.GetVersion() >= 46)
				{
					auto component = scene.animation_datas.GetComponent(entity);
					if (component != nullptr)
					{
						archive << true;
						component->Serialize(archive, seri);
					}
					else
					{
						archive << false;
					}
				}

				// Wait the job system, because from this point, component managers could be resized
				//	due to more serialization tasks in recursive operation
				//	The pointers must not be invalidated while serialization jobs are not finished
				wi::jobsystem::Wait(seri.ctx);

				if (archive.GetVersion() >= 72 && has_flag(flags, Scene::EntitySerializeFlags::RECURSIVE))
				{
					// Recursive serialization for all children:
					seri.allow_remap = restore_remap;
					wi::vector<Entity> children;
					for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
					{
						const HierarchyComponent& hier = scene.hierarchy[i];
						if (hier.parentID == entity)
						{
							Entity child = scene.hierarchy.GetEntity(i);
							children.push_back(child);
						}
					}
					archive << children.size();
					for (Entity child : children)
					{
						Entity_Serialize_Internal(scene, archive, seri, child, flags);
					}
				}
			}
		}

		seri.allow_remap = restore_remap;
		return entity;
	}

	Entity Scene::Entity_Serialize(
		wi::Archive& archive,
		EntitySerializer& seri,
		Entity entity,
		EntitySerializeFlags flags
	)
	{
		// Manage jump position to jump to resource serialization WRITE area:
		size_t jump_before = 0;
		size_t jump_after = 0;
		size_t original_pos = 0;
		if (archive.GetVersion() >= 90)
		{
			if (archive.IsReadMode())
			{
				archive >> jump_before;
				archive >> jump_after;
				original_pos = archive.GetPos();
				archive.Jump(jump_before); // jump before resourcemanager::Serialize_WRITE
			}
			else
			{
				jump_before = archive.WriteUnknownJumpPosition();
				jump_after = archive.WriteUnknownJumpPosition();
			}
		}

		// Keeping this alive to keep serialized resources alive until entity serialization ends:
		wi::resourcemanager::ResourceSerializer resource_seri;
		if (archive.IsReadMode() && archive.GetVersion() >= 90)
		{
			wi::resourcemanager::Serialize_READ(archive, resource_seri);
			// After resource serialization, jump back to entity serialization area:
			archive.Jump(original_pos); // jump back to entity serialize
		}

		Entity ret = Entity_Serialize_Internal(
			*this,
			archive,
			seri,
			entity,
			flags
		);

		if (archive.GetVersion() >= 90)
		{
			if (archive.IsReadMode())
			{
				archive.Jump(jump_after); // jump after resourcemanager::Serialize_WRITE
			}
			else
			{
				archive.PatchUnknownJumpPosition(jump_before);
				wi::resourcemanager::Serialize_WRITE(archive, seri.resource_registration);
				archive.PatchUnknownJumpPosition(jump_after);
			}
		}

		return ret;
	}

}
