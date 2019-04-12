#include "wiSceneSystem.h"
#include "wiResourceManager.h"
#include "wiArchive.h"
#include "wiRandom.h"
#include "wiHelper.h"

using namespace wiECS;

namespace wiSceneSystem
{

	void NameComponent::Serialize(wiArchive& archive, uint32_t seed)
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
	void LayerComponent::Serialize(wiArchive& archive, uint32_t seed)
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
	void TransformComponent::Serialize(wiArchive& archive, uint32_t seed)
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
	void PreviousFrameTransformComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		// NOTHING! We just need a serialize function for this to be able serialize with ComponentManager!
		//	This structure has no persistent state!
	}
	void HierarchyComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		SerializeEntity(archive, parentID, seed);

		if (archive.IsReadMode())
		{
			archive >> layerMask_bind;
			archive >> world_parent_inverse_bind;
		}
		else
		{
			archive << layerMask_bind;
			archive << world_parent_inverse_bind;
		}
	}
	void MaterialComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		std::string dir = archive.GetSourceDirectory();

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
			archive >> reflectance;
			archive >> metalness;
			if (archive.GetVersion() < 25)
			{
				archive >> emissiveColor.w;
			}
			archive >> refractionIndex;
			archive >> subsurfaceScattering;
			archive >> normalMapStrength;
			archive >> parallaxOcclusionMapping;
			archive >> alphaRef;
			archive >> texAnimDirection;
			archive >> texAnimFrameRate;
			archive >> texAnimElapsedTime;

			archive >> baseColorMapName;
			archive >> surfaceMapName;
			archive >> normalMapName;
			archive >> displacementMapName;

			if (archive.GetVersion() >= 24)
			{
				archive >> emissiveMapName;
			}

			if (archive.GetVersion() >= 28)
			{
				archive >> occlusionMapName;

				archive >> uvset_baseColorMap;
				archive >> uvset_surfaceMap;
				archive >> uvset_normalMap;
				archive >> uvset_displacementMap;
				archive >> uvset_emissiveMap;
				archive >> uvset_occlusionMap;

				archive >> displacementMapping;
			}

			SetDirty();

			if (!baseColorMapName.empty())
			{
				baseColorMap = (wiGraphics::Texture2D*)wiResourceManager::GetGlobal().add(dir + baseColorMapName);
			}
			if (!surfaceMapName.empty())
			{
				surfaceMap = (wiGraphics::Texture2D*)wiResourceManager::GetGlobal().add(dir + surfaceMapName);
			}
			if (!normalMapName.empty())
			{
				normalMap = (wiGraphics::Texture2D*)wiResourceManager::GetGlobal().add(dir + normalMapName);
			}
			if (!displacementMapName.empty())
			{
				displacementMap = (wiGraphics::Texture2D*)wiResourceManager::GetGlobal().add(dir + displacementMapName);
			}
			if (!emissiveMapName.empty())
			{
				emissiveMap = (wiGraphics::Texture2D*)wiResourceManager::GetGlobal().add(dir + emissiveMapName);
			}
			if (!occlusionMapName.empty())
			{
				occlusionMap = (wiGraphics::Texture2D*)wiResourceManager::GetGlobal().add(dir + occlusionMapName);
			}

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
			archive << refractionIndex;
			archive << subsurfaceScattering;
			archive << normalMapStrength;
			archive << parallaxOcclusionMapping;
			archive << alphaRef;
			archive << texAnimDirection;
			archive << texAnimFrameRate;
			archive << texAnimElapsedTime;

			// If detecting an absolute path in textures, remove it and convert to relative:
			{
				size_t found = baseColorMapName.rfind(dir);
				if (found != std::string::npos)
				{
					baseColorMapName = baseColorMapName.substr(found + dir.length());
				}

				found = surfaceMapName.rfind(dir);
				if (found != std::string::npos)
				{
					surfaceMapName = surfaceMapName.substr(found + dir.length());
				}

				found = normalMapName.rfind(dir);
				if (found != std::string::npos)
				{
					normalMapName = normalMapName.substr(found + dir.length());
				}

				found = displacementMapName.rfind(dir);
				if (found != std::string::npos)
				{
					displacementMapName = displacementMapName.substr(found + dir.length());
				}

				found = emissiveMapName.rfind(dir);
				if (found != std::string::npos)
				{
					emissiveMapName = emissiveMapName.substr(found + dir.length());
				}
			}

			archive << baseColorMapName;
			archive << surfaceMapName;
			archive << normalMapName;
			archive << displacementMapName;

			if (archive.GetVersion() >= 24)
			{
				archive << emissiveMapName;
			}

			if (archive.GetVersion() >= 28)
			{
				archive << occlusionMapName;

				archive << uvset_baseColorMap;
				archive << uvset_surfaceMap;
				archive << uvset_normalMap;
				archive << uvset_displacementMap;
				archive << uvset_emissiveMap;
				archive << uvset_occlusionMap;

				archive << displacementMapping;
			}
		}
	}
	void MeshComponent::Serialize(wiArchive& archive, uint32_t seed)
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
				SerializeEntity(archive, subsets[i].materialID, seed);
				archive >> subsets[i].indexOffset;
				archive >> subsets[i].indexCount;
			}

			archive >> tessellationFactor;
			SerializeEntity(archive, armatureID, seed);

			if (archive.GetVersion() >= 28)
			{
				archive >> vertex_uvset_1;
			}

			CreateRenderData();
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
				SerializeEntity(archive, subsets[i].materialID, seed);
				archive << subsets[i].indexOffset;
				archive << subsets[i].indexCount;
			}

			archive << tessellationFactor;
			SerializeEntity(archive, armatureID, seed);

			if (archive.GetVersion() >= 28)
			{
				archive << vertex_uvset_1;
			}

		}
	}
	void ImpostorComponent::Serialize(wiArchive& archive, uint32_t seed)
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
	void ObjectComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			SerializeEntity(archive, meshID, seed);
			archive >> cascadeMask;
			archive >> rendertypeMask;
			archive >> color;

			if (archive.GetVersion() >= 23)
			{
				archive >> lightmapWidth;
				archive >> lightmapHeight;
				archive >> lightmapTextureData;
			}
		}
		else
		{
			archive << _flags;
			SerializeEntity(archive, meshID, seed);
			archive << cascadeMask;
			archive << rendertypeMask;
			archive << color;

			if (archive.GetVersion() >= 23)
			{
				archive << lightmapWidth;
				archive << lightmapHeight;
				archive << lightmapTextureData;
			}
		}
	}
	void RigidBodyPhysicsComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> (uint32_t&)shape;
			archive >> mass;
			archive >> friction;
			archive >> restitution;
			archive >> damping;
		}
		else
		{
			archive << _flags;
			archive << (uint32_t&)shape;
			archive << mass;
			archive << friction;
			archive << restitution;
			archive << damping;
		}
	}
	void SoftBodyPhysicsComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> mass;
			archive >> friction;
			archive >> physicsToGraphicsVertexMapping;
			archive >> graphicsToPhysicsVertexMapping;
			archive >> weights;

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
		}
	}
	void ArmatureComponent::Serialize(wiArchive& archive, uint32_t seed)
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
				SerializeEntity(archive, boneID, seed);
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
				SerializeEntity(archive, rootBoneID, seed);
			}
		}
		else
		{
			archive << _flags;

			archive << boneCollection.size();
			for (size_t i = 0; i < boneCollection.size(); ++i)
			{
				Entity boneID = boneCollection[i];
				SerializeEntity(archive, boneID, seed);
			}

			archive << inverseBindMatrices;
			if (archive.GetVersion() == 26)
			{
				Entity rootBoneID;
				SerializeEntity(archive, rootBoneID, seed);
			}
		}
	}
	void LightComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> color;
			archive >> (uint32_t&)type;
			archive >> energy;
			archive >> range_local;
			archive >> fov;
			archive >> shadowBias;
			archive >> radius;
			archive >> width;
			archive >> height;

			archive >> lensFlareNames;
		}
		else
		{
			archive << _flags;
			archive << color;
			archive << (uint32_t&)type;
			archive << energy;
			archive << range_local;
			archive << fov;
			archive << shadowBias;
			archive << radius;
			archive << width;
			archive << height;

			archive << lensFlareNames;
		}
	}
	void CameraComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> width;
			archive >> height;
			archive >> zNearP;
			archive >> zFarP;
			archive >> fov;

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
		}
	}
	void EnvironmentProbeComponent::Serialize(wiArchive& archive, uint32_t seed)
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
	void ForceFieldComponent::Serialize(wiArchive& archive, uint32_t seed)
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
	void DecalComponent::Serialize(wiArchive& archive, uint32_t seed)
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
	void AnimationComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> start;
			archive >> end;
			archive >> timer;

			size_t channelCount;
			archive >> channelCount;
			channels.resize(channelCount);
			for (size_t i = 0; i < channelCount; ++i)
			{
				archive >> channels[i]._flags;
				archive >> (uint32_t&)channels[i].path;
				SerializeEntity(archive, channels[i].target, seed);
				archive >> channels[i].samplerIndex;
			}

			size_t samplerCount;
			archive >> samplerCount;
			samplers.resize(samplerCount);
			for (size_t i = 0; i < samplerCount; ++i)
			{
				archive >> samplers[i]._flags;
				archive >> (uint32_t&)samplers[i].mode;
				archive >> samplers[i].keyframe_times;
				archive >> samplers[i].keyframe_data;
			}

		}
		else
		{
			archive << _flags;
			archive << start;
			archive << end;
			archive << timer;

			archive << channels.size();
			for (size_t i = 0; i < channels.size(); ++i)
			{
				archive << channels[i]._flags;
				archive << (uint32_t&)channels[i].path;
				SerializeEntity(archive, channels[i].target, seed);
				archive << channels[i].samplerIndex;
			}

			archive << samplers.size();
			for (size_t i = 0; i < samplers.size(); ++i)
			{
				archive << samplers[i]._flags;
				archive << samplers[i].mode;
				archive << samplers[i].keyframe_times;
				archive << samplers[i].keyframe_data;
			}
		}
	}
	void WeatherComponent::Serialize(wiArchive& archive, uint32_t seed)
	{
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
			archive >> fogHeight;
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
			archive >> oceanParameters.waterColor;
			archive >> oceanParameters.waterHeight;
			archive >> oceanParameters.surfaceDetail;
			archive >> oceanParameters.surfaceDisplacementTolerance;
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
			archive << fogHeight;
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
		}
	}

	void Scene::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			uint32_t entityCount;
			archive >> entityCount; // reserved
		}
		else
		{
			archive << CountEntities();
		}

		// With this we will ensure that serialized entities are unique and persistent across the scene:
		uint32_t seed = (uint32_t)wiRandom::getRandom(1, INT_MAX);

		names.Serialize(archive, seed);
		layers.Serialize(archive, seed);
		transforms.Serialize(archive, seed);
		prev_transforms.Serialize(archive, seed);
		hierarchy.Serialize(archive, seed);
		materials.Serialize(archive, seed);
		meshes.Serialize(archive, seed);
		impostors.Serialize(archive, seed);
		objects.Serialize(archive, seed);
		aabb_objects.Serialize(archive, seed);
		rigidbodies.Serialize(archive, seed);
		softbodies.Serialize(archive, seed);
		armatures.Serialize(archive, seed);
		lights.Serialize(archive, seed);
		aabb_lights.Serialize(archive, seed);
		cameras.Serialize(archive, seed);
		probes.Serialize(archive, seed);
		aabb_probes.Serialize(archive, seed);
		forces.Serialize(archive, seed);
		decals.Serialize(archive, seed);
		aabb_decals.Serialize(archive, seed);
		animations.Serialize(archive, seed);
		emitters.Serialize(archive, seed);
		hairs.Serialize(archive, seed);
		weathers.Serialize(archive, seed);

	}

	Entity Scene::Entity_Serialize(wiArchive& archive, Entity entity, uint32_t seed, bool propagateSeedDeep)
	{
		SerializeEntity(archive, entity, seed);

		if (archive.IsReadMode())
		{
			// Check for each components if it exists, and if yes, READ it:
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = names.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = layers.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = transforms.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = prev_transforms.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = hierarchy.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = materials.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = meshes.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = impostors.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = objects.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = aabb_objects.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = rigidbodies.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = softbodies.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = armatures.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = lights.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = aabb_lights.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = cameras.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = probes.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = aabb_probes.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = forces.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = decals.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = aabb_decals.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = animations.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = emitters.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = hairs.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
				}
			}
			{
				bool component_exists;
				archive >> component_exists;
				if (component_exists)
				{
					auto& component = weathers.Create(entity);
					component.Serialize(archive, propagateSeedDeep ? seed : 0);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
					component->Serialize(archive, seed);
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
