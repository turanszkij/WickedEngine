#include "wiScene.h"
#include "wiResourceManager.h"
#include "wiArchive.h"
#include "wiRandom.h"
#include "wiHelper.h"
#include "wiBackLog.h"

#include <chrono>
#include <string>

using namespace wiECS;

namespace wiScene
{

	void NameComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void LayerComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void TransformComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void PreviousFrameTransformComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
	{
		// NOTHING! We just need a serialize function for this to be able serialize with ComponentManager!
		//	This structure has no persistent state!
	}
	void HierarchyComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void MaterialComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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

			for (auto& x : textures)
			{
				if (!x.name.empty())
				{
					x.name = dir + x.name;
				}
			}

			wiJobSystem::Execute(seri.ctx, [&](wiJobArgs args) {
				CreateRenderData();
			});
		}
		else
		{
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

			for (auto& x : textures)
			{
				wiHelper::MakePathRelative(dir, x.name);
			}

			archive << textures[BASECOLORMAP].name;
			archive << textures[SURFACEMAP].name;
			archive << textures[NORMALMAP].name;
			archive << textures[DISPLACEMENTMAP].name;

			if (archive.GetVersion() >= 24)
			{
				archive << textures[EMISSIVEMAP].name;
			}

			if (archive.GetVersion() >= 28)
			{
				archive << textures[OCCLUSIONMAP].name;

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
				archive << textures[TRANSMISSIONMAP].name;
				archive << textures[TRANSMISSIONMAP].uvset;
			}

			if (archive.GetVersion() >= 61)
			{
				archive << sheenColor;
				archive << sheenRoughness;
				archive << textures[SHEENCOLORMAP].name;
				archive << textures[SHEENROUGHNESSMAP].name;
				archive << textures[SHEENCOLORMAP].uvset;
				archive << textures[SHEENROUGHNESSMAP].uvset;

				archive << clearcoat;
				archive << clearcoatRoughness;
				archive << textures[CLEARCOATMAP].name;
				archive << textures[CLEARCOATROUGHNESSMAP].name;
				archive << textures[CLEARCOATNORMALMAP].name;
				archive << textures[CLEARCOATMAP].uvset;
				archive << textures[CLEARCOATROUGHNESSMAP].uvset;
				archive << textures[CLEARCOATNORMALMAP].uvset;
			}

			if (archive.GetVersion() >= 68)
			{
				archive << textures[SPECULARMAP].name;
				archive << textures[SPECULARMAP].uvset;
			}
		}
	}
	void MeshComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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

			if (archive.GetVersion() >= 41)
			{
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
			    targets.resize(targetCount);
			    for (size_t i = 0; i < targetCount; ++i)
			    {
					archive >> targets[i].vertex_positions;
					archive >> targets[i].vertex_normals;
					archive >> targets[i].weight;
			    }
			}

			wiJobSystem::Execute(seri.ctx, [&](wiJobArgs args) {
				CreateRenderData();
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

			if (archive.GetVersion() >= 41)
			{
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
			    archive << targets.size();
			    for (size_t i = 0; i < targets.size(); ++i)
			    {
					archive << targets[i].vertex_positions;
					archive << targets[i].vertex_normals;
					archive << targets[i].weight;
			    }
			}

		}
	}
	void ImpostorComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void ObjectComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			SerializeEntity(archive, meshID, seri);
			archive >> cascadeMask;
			archive >> rendertypeMask;
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
		}
		else
		{
			archive << _flags;
			SerializeEntity(archive, meshID, seri);
			archive << cascadeMask;
			archive << rendertypeMask;
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
		}
	}
	void RigidBodyPhysicsComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
		}
	}
	void SoftBodyPhysicsComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
	{
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
				std::vector<XMFLOAT3> temp;
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
		}
	}
	void ArmatureComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void LightComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
			archive >> energy;
			archive >> range_local;
			archive >> fov;
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

			wiJobSystem::Execute(seri.ctx, [&](wiJobArgs args) {
				lensFlareRimTextures.resize(lensFlareNames.size());
				for (size_t i = 0; i < lensFlareNames.size(); ++i)
				{
					if (!lensFlareNames[i].empty())
					{
						lensFlareNames[i] = dir + lensFlareNames[i];
						lensFlareRimTextures[i] = wiResourceManager::Load(lensFlareNames[i], wiResourceManager::IMPORT_RETAIN_FILEDATA);
					}
				}
			});
		}
		else
		{
			archive << _flags;
			archive << color;
			archive << (uint32_t)type;
			archive << energy;
			archive << range_local;
			archive << fov;
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
			if (!dir.empty())
			{
				for (size_t i = 0; i < lensFlareNames.size(); ++i)
				{
					wiHelper::MakePathRelative(dir, lensFlareNames[i]);
				}
			}
			archive << lensFlareNames;
		}
	}
	void CameraComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void EnvironmentProbeComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;

			SetDirty();
		}
		else
		{
			archive << _flags;
		}
	}
	void ForceFieldComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> type;
			archive >> gravity;
			archive >> range_local;
		}
		else
		{
			archive << _flags;
			archive << type;
			archive << gravity;
			archive << range_local;
		}
	}
	void DecalComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
		}
		else
		{
			archive << _flags;
		}
	}
	void AnimationComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
			}

			archive << samplers.size();
			for (size_t i = 0; i < samplers.size(); ++i)
			{
				archive << samplers[i]._flags;
				archive << samplers[i].mode;
				SerializeEntity(archive, samplers[i].data, seri);
			}
		}
	}
	void AnimationDataComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void WeatherComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
			archive >> fogEnd;
			archive >> fogHeightSky;
			archive >> cloudiness;
			archive >> cloudScale;
			archive >> cloudSpeed;
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
					skyMap = wiResourceManager::Load(skyMapName, wiResourceManager::IMPORT_RETAIN_FILEDATA);
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
					colorGradingMap = wiResourceManager::Load(colorGradingMapName, wiResourceManager::IMPORT_COLORGRADINGLUT | wiResourceManager::IMPORT_RETAIN_FILEDATA);
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
				archive >> volumetricCloudParameters.Albedo;
				archive >> volumetricCloudParameters.CloudAmbientGroundMultiplier;
				archive >> volumetricCloudParameters.ExtinctionCoefficient;
				archive >> volumetricCloudParameters.BeerPowder;
				archive >> volumetricCloudParameters.BeerPowderPower;
				archive >> volumetricCloudParameters.PhaseG;
				archive >> volumetricCloudParameters.PhaseG2;
				archive >> volumetricCloudParameters.PhaseBlend;
				archive >> volumetricCloudParameters.MultiScatteringScattering;
				archive >> volumetricCloudParameters.MultiScatteringExtinction;
				archive >> volumetricCloudParameters.MultiScatteringEccentricity;
				archive >> volumetricCloudParameters.ShadowStepLength;
				archive >> volumetricCloudParameters.HorizonBlendAmount;
				archive >> volumetricCloudParameters.HorizonBlendPower;
				archive >> volumetricCloudParameters.WeatherDensityAmount;
				archive >> volumetricCloudParameters.CloudStartHeight;
				archive >> volumetricCloudParameters.CloudThickness;
				archive >> volumetricCloudParameters.SkewAlongWindDirection;
				archive >> volumetricCloudParameters.TotalNoiseScale;
				archive >> volumetricCloudParameters.DetailScale;
				archive >> volumetricCloudParameters.WeatherScale;
				archive >> volumetricCloudParameters.CurlScale;
				archive >> volumetricCloudParameters.ShapeNoiseHeightGradientAmount;
				archive >> volumetricCloudParameters.ShapeNoiseMultiplier;
				archive >> volumetricCloudParameters.ShapeNoiseMinMax;
				archive >> volumetricCloudParameters.ShapeNoisePower;
				archive >> volumetricCloudParameters.DetailNoiseModifier;
				archive >> volumetricCloudParameters.DetailNoiseHeightFraction;
				archive >> volumetricCloudParameters.CurlNoiseModifier;
				archive >> volumetricCloudParameters.CoverageAmount;
				archive >> volumetricCloudParameters.CoverageMinimum;
				archive >> volumetricCloudParameters.TypeAmount;
				archive >> volumetricCloudParameters.TypeOverall;
				archive >> volumetricCloudParameters.AnvilAmount;
				archive >> volumetricCloudParameters.AnvilOverhangHeight;
				archive >> volumetricCloudParameters.AnimationMultiplier;
				archive >> volumetricCloudParameters.WindSpeed;
				archive >> volumetricCloudParameters.WindAngle;
				archive >> volumetricCloudParameters.WindUpAmount;
				archive >> volumetricCloudParameters.CoverageWindSpeed;
				archive >> volumetricCloudParameters.CoverageWindAngle;
				archive >> volumetricCloudParameters.CloudGradientSmall;
				archive >> volumetricCloudParameters.CloudGradientMedium;
				archive >> volumetricCloudParameters.CloudGradientLarge;
				archive >> volumetricCloudParameters.MaxStepCount;
				archive >> volumetricCloudParameters.MaxMarchingDistance;
				archive >> volumetricCloudParameters.InverseDistanceStepCount;
				archive >> volumetricCloudParameters.RenderDistance;
				archive >> volumetricCloudParameters.LODDistance;
				archive >> volumetricCloudParameters.LODMin;
				archive >> volumetricCloudParameters.BigStepMarch;
				archive >> volumetricCloudParameters.TransmittanceThreshold;
				archive >> volumetricCloudParameters.ShadowSampleCount;
				archive >> volumetricCloudParameters.GroundContributionSampleCount;
			}

			if (archive.GetVersion() >= 71)
			{
				archive >> fogHeightStart;
				archive >> fogHeightEnd;
			}
		}
		else
		{
			archive << _flags;
			archive << sunDirection;
			archive << sunColor;
			archive << horizon;
			archive << zenith;
			archive << ambient;
			archive << fogStart;
			archive << fogEnd;
			archive << fogHeightSky;
			archive << cloudiness;
			archive << cloudScale;
			archive << cloudSpeed;
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

			wiHelper::MakePathRelative(dir, skyMapName);
			wiHelper::MakePathRelative(dir, colorGradingMapName);

			if (archive.GetVersion() >= 32)
			{
				archive << skyMapName;
			}
			if (archive.GetVersion() >= 40)
			{
				archive << windSpeed;
			}
			if (archive.GetVersion() >= 62)
			{
				archive << colorGradingMapName;
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
				archive << volumetricCloudParameters.Albedo;
				archive << volumetricCloudParameters.CloudAmbientGroundMultiplier;
				archive << volumetricCloudParameters.ExtinctionCoefficient;
				archive << volumetricCloudParameters.BeerPowder;
				archive << volumetricCloudParameters.BeerPowderPower;
				archive << volumetricCloudParameters.PhaseG;
				archive << volumetricCloudParameters.PhaseG2;
				archive << volumetricCloudParameters.PhaseBlend;
				archive << volumetricCloudParameters.MultiScatteringScattering;
				archive << volumetricCloudParameters.MultiScatteringExtinction;
				archive << volumetricCloudParameters.MultiScatteringEccentricity;
				archive << volumetricCloudParameters.ShadowStepLength;
				archive << volumetricCloudParameters.HorizonBlendAmount;
				archive << volumetricCloudParameters.HorizonBlendPower;
				archive << volumetricCloudParameters.WeatherDensityAmount;
				archive << volumetricCloudParameters.CloudStartHeight;
				archive << volumetricCloudParameters.CloudThickness;
				archive << volumetricCloudParameters.SkewAlongWindDirection;
				archive << volumetricCloudParameters.TotalNoiseScale;
				archive << volumetricCloudParameters.DetailScale;
				archive << volumetricCloudParameters.WeatherScale;
				archive << volumetricCloudParameters.CurlScale;
				archive << volumetricCloudParameters.ShapeNoiseHeightGradientAmount;
				archive << volumetricCloudParameters.ShapeNoiseMultiplier;
				archive << volumetricCloudParameters.ShapeNoiseMinMax;
				archive << volumetricCloudParameters.ShapeNoisePower;
				archive << volumetricCloudParameters.DetailNoiseModifier;
				archive << volumetricCloudParameters.DetailNoiseHeightFraction;
				archive << volumetricCloudParameters.CurlNoiseModifier;
				archive << volumetricCloudParameters.CoverageAmount;
				archive << volumetricCloudParameters.CoverageMinimum;
				archive << volumetricCloudParameters.TypeAmount;
				archive << volumetricCloudParameters.TypeOverall;
				archive << volumetricCloudParameters.AnvilAmount;
				archive << volumetricCloudParameters.AnvilOverhangHeight;
				archive << volumetricCloudParameters.AnimationMultiplier;
				archive << volumetricCloudParameters.WindSpeed;
				archive << volumetricCloudParameters.WindAngle;
				archive << volumetricCloudParameters.WindUpAmount;
				archive << volumetricCloudParameters.CoverageWindSpeed;
				archive << volumetricCloudParameters.CoverageWindAngle;
				archive << volumetricCloudParameters.CloudGradientSmall;
				archive << volumetricCloudParameters.CloudGradientMedium;
				archive << volumetricCloudParameters.CloudGradientLarge;
				archive << volumetricCloudParameters.MaxStepCount;
				archive << volumetricCloudParameters.MaxMarchingDistance;
				archive << volumetricCloudParameters.InverseDistanceStepCount;
				archive << volumetricCloudParameters.RenderDistance;
				archive << volumetricCloudParameters.LODDistance;
				archive << volumetricCloudParameters.LODMin;
				archive << volumetricCloudParameters.BigStepMarch;
				archive << volumetricCloudParameters.TransmittanceThreshold;
				archive << volumetricCloudParameters.ShadowSampleCount;
				archive << volumetricCloudParameters.GroundContributionSampleCount;
			}

			if (archive.GetVersion() >= 71)
			{
				archive << fogHeightStart;
				archive << fogHeightEnd;
			}
		}
	}
	void SoundComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> filename;
			archive >> volume;
			archive >> (uint32_t&)soundinstance.type;

			wiJobSystem::Execute(seri.ctx, [&](wiJobArgs args) {
				if (!filename.empty())
				{
					filename = dir + filename;
					soundResource = wiResourceManager::Load(filename, wiResourceManager::IMPORT_RETAIN_FILEDATA);
					wiAudio::CreateSoundInstance(&soundResource->sound, &soundinstance);
				}
			});
		}
		else
		{
			wiHelper::MakePathRelative(dir, filename);

			archive << _flags;
			archive << filename;
			archive << volume;
			archive << soundinstance.type;
		}
	}
	void InverseKinematicsComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
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
	void SpringComponent::Serialize(wiArchive& archive, EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> stiffness;
			archive >> damping;
			archive >> wind_affection;

			Reset();
		}
		else
		{
			archive << _flags;
			archive << stiffness;
			archive << damping;
			archive << wind_affection;
		}
	}

	void Scene::Serialize(wiArchive& archive)
	{
		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

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

		// Keeping this alive to keep serialized resources alive until entity serialization ends:
		wiResourceManager::ResourceSerializer resource_seri;
		if (archive.GetVersion() >= 63)
		{
			wiResourceManager::Serialize(archive, resource_seri);
		}

		// With this we will ensure that serialized entities are unique and persistent across the scene:
		EntitySerializer seri;

		names.Serialize(archive, seri);
		layers.Serialize(archive, seri);
		transforms.Serialize(archive, seri);
		prev_transforms.Serialize(archive, seri);
		hierarchy.Serialize(archive, seri);
		materials.Serialize(archive, seri);
		meshes.Serialize(archive, seri);
		impostors.Serialize(archive, seri);
		objects.Serialize(archive, seri);
		aabb_objects.Serialize(archive, seri);
		rigidbodies.Serialize(archive, seri);
		softbodies.Serialize(archive, seri);
		armatures.Serialize(archive, seri);
		lights.Serialize(archive, seri);
		aabb_lights.Serialize(archive, seri);
		cameras.Serialize(archive, seri);
		probes.Serialize(archive, seri);
		aabb_probes.Serialize(archive, seri);
		forces.Serialize(archive, seri);
		decals.Serialize(archive, seri);
		aabb_decals.Serialize(archive, seri);
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

		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
		double sec = time_span.count();
		wiBackLog::post((std::string("Scene serialize took ") + std::to_string(sec) + std::string(" sec")).c_str());
	}

	Entity Scene::Entity_Serialize(wiArchive& archive, Entity entity)
	{
		EntitySerializer seri;

		SerializeEntity(archive, entity, seri);

		// From this point, we will not remap the entities, 
		//	to retain internal entity references inside components:
		seri.allow_remap = false;

		if (archive.IsReadMode())
		{
			// Check for each components if it exists, and if yes, READ it:
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = names.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = layers.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = transforms.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = prev_transforms.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = hierarchy.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = materials.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = meshes.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = impostors.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = objects.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = aabb_objects.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = rigidbodies.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = softbodies.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = armatures.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = lights.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = aabb_lights.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = cameras.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = probes.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = aabb_probes.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = forces.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = decals.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = aabb_decals.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = animations.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = emitters.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = hairs.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = weathers.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			if (archive.GetVersion() >= 30)
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = sounds.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			if (archive.GetVersion() >= 37)
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = inverse_kinematics.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			if (archive.GetVersion() >= 38)
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = springs.Create(entity);
					component.Serialize(archive, seri);
				}
			}
			if (archive.GetVersion() >= 46)
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = animation_datas.Create(entity);
					component.Serialize(archive, seri);
				}
			}
		}
		else
		{
			// Find existing components one-by-one and WRITE them out:
			{
				auto component = names.GetComponent(entity);
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
				auto component = layers.GetComponent(entity);
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
				auto component = transforms.GetComponent(entity);
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
				auto component = prev_transforms.GetComponent(entity);
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
				auto component = hierarchy.GetComponent(entity);
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
				auto component = materials.GetComponent(entity);
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
				auto component = meshes.GetComponent(entity);
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
				auto component = impostors.GetComponent(entity);
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
				auto component = objects.GetComponent(entity);
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
				auto component = aabb_objects.GetComponent(entity);
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
				auto component = rigidbodies.GetComponent(entity);
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
				auto component = softbodies.GetComponent(entity);
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
				auto component = armatures.GetComponent(entity);
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
				auto component = lights.GetComponent(entity);
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
				auto component = aabb_lights.GetComponent(entity);
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
				auto component = cameras.GetComponent(entity);
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
				auto component = probes.GetComponent(entity);
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
				auto component = aabb_probes.GetComponent(entity);
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
				auto component = forces.GetComponent(entity);
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
				auto component = decals.GetComponent(entity);
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
				auto component = aabb_decals.GetComponent(entity);
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
				auto component = animations.GetComponent(entity);
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
				auto component = emitters.GetComponent(entity);
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
				auto component = hairs.GetComponent(entity);
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
				auto component = weathers.GetComponent(entity);
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
				auto component = sounds.GetComponent(entity);
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
				auto component = inverse_kinematics.GetComponent(entity);
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
				auto component = springs.GetComponent(entity);
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
				auto component = animation_datas.GetComponent(entity);
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

		}

		return entity;
	}

}
