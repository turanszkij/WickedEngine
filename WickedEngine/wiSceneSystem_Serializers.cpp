#include "wiSceneSystem.h"
#include "wiResourceManager.h"
#include "wiArchive.h"

namespace wiSceneSystem
{

	void NameComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			std::string tmp;
			archive >> tmp;
			*this = tmp;
		}
		else
		{
			std::string tmp = name;
			archive << tmp;
		}
	}
	void LayerComponent::Serialize(wiArchive& archive)
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
	void TransformComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> scale_local;
			archive >> rotation_local;
			archive >> translation_local;

			SetDirty();
		}
		else
		{
			archive << _flags; // maybe not needed just for dirtiness, but later might come handy if we have more persistent flags
			archive << scale_local;
			archive << rotation_local;
			archive << translation_local;
		}
	}
	void PreviousFrameTransformComponent::Serialize(wiArchive& archive)
	{
		// NOTHING! We just need a serialize function for this to be able serialize with ComponentManager!
		//	This structure has no persistent state!
	}
	void HierarchyComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> parentID;
			archive >> layerMask_bind;
			archive >> world_parent_inverse_bind;
		}
		else
		{
			archive << parentID;
			archive << layerMask_bind;
			archive << world_parent_inverse_bind;
		}
	}
	void MaterialComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> (uint8_t&)engineStencilRef;
			archive >> userStencilRef;
			archive >> (uint8_t&)blendFlag;
			archive >> baseColor;
			archive >> texMulAdd;
			archive >> roughness;
			archive >> reflectance;
			archive >> metalness;
			archive >> emissive;
			archive >> refractionIndex;
			archive >> subsurfaceScattering;
			archive >> normalMapStrength;
			archive >> parallaxOcclusionMapping;
			archive >> alphaRef;
			archive >> texAnimDirection;
			archive >> texAnimFrameRate;
			archive >> texAnimSleep;

			archive >> baseColorMapName;
			archive >> surfaceMapName;
			archive >> normalMapName;
			archive >> displacementMapName;

			SetDirty();
			if (!baseColorMapName.empty())
			{
				baseColorMap = (wiGraphicsTypes::Texture2D*)wiResourceManager::GetGlobal()->add(baseColorMapName);
			}
			if (!surfaceMapName.empty())
			{
				surfaceMap = (wiGraphicsTypes::Texture2D*)wiResourceManager::GetGlobal()->add(surfaceMapName);
			}
			if (!normalMapName.empty())
			{
				normalMap = (wiGraphicsTypes::Texture2D*)wiResourceManager::GetGlobal()->add(normalMapName);
			}
			if (!displacementMapName.empty())
			{
				displacementMap = (wiGraphicsTypes::Texture2D*)wiResourceManager::GetGlobal()->add(displacementMapName);
			}

		}
		else
		{
			archive << _flags;
			archive << (uint8_t)engineStencilRef;
			archive << userStencilRef;
			archive << (uint8_t)blendFlag;
			archive << baseColor;
			archive << texMulAdd;
			archive << roughness;
			archive << reflectance;
			archive << metalness;
			archive << emissive;
			archive << refractionIndex;
			archive << subsurfaceScattering;
			archive << normalMapStrength;
			archive << parallaxOcclusionMapping;
			archive << alphaRef;
			archive << texAnimDirection;
			archive << texAnimFrameRate;
			archive << texAnimSleep;

			archive << baseColorMapName;
			archive << surfaceMapName;
			archive << normalMapName;
			archive << displacementMapName;
		}
	}
	void MeshComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> vertex_positions;
			archive >> vertex_normals;
			archive >> vertex_texcoords;
			archive >> vertex_boneindices;
			archive >> vertex_boneweights;
			archive >> indices;

			size_t subsetCount;
			archive >> subsetCount;
			subsets.resize(subsetCount);
			for (size_t i = 0; i < subsetCount; ++i)
			{
				archive >> subsets[i].materialID;
				archive >> subsets[i].indexOffset;
				archive >> subsets[i].indexCount;
			}

			archive >> tessellationFactor;
			archive >> impostorDistance;
			archive >> armatureID;

			CreateRenderData();
		}
		else
		{
			archive << _flags;
			archive << vertex_positions;
			archive << vertex_normals;
			archive << vertex_texcoords;
			archive << vertex_boneindices;
			archive << vertex_boneweights;
			archive << indices;

			archive << subsets.size();
			for (size_t i = 0; i < subsets.size(); ++i)
			{
				archive << subsets[i].materialID;
				archive << subsets[i].indexOffset;
				archive << subsets[i].indexCount;
			}

			archive << tessellationFactor;
			archive << impostorDistance;
			archive << armatureID;

		}
	}
	void ObjectComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> meshID;
			archive >> cascadeMask;
			archive >> rendertypeMask;
			archive >> color;
		}
		else
		{
			archive << _flags;
			archive << meshID;
			archive << cascadeMask;
			archive << rendertypeMask;
			archive << color;
		}
	}
	void RigidBodyPhysicsComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> (uint32_t&)shape;
			archive >> kinematic;
			archive >> mass;
			archive >> friction;
			archive >> restitution;
			archive >> damping;
		}
		else
		{
			archive << _flags;
			archive << (uint32_t&)shape;
			archive << kinematic;
			archive << mass;
			archive << friction;
			archive << restitution;
			archive << damping;
		}
	}
	void SoftBodyPhysicsComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> mass;
			archive >> friction;
			archive >> physicsvertices;
			archive >> physicsindices;
		}
		else
		{
			archive << _flags;
			archive << mass;
			archive << friction;
			archive << physicsvertices;
			archive << physicsindices;
		}
	}
	void ArmatureComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> boneCollection;
			archive >> inverseBindMatrices;
			archive >> remapMatrix;
		}
		else
		{
			archive << _flags;
			archive << boneCollection;
			archive << inverseBindMatrices;
			archive << remapMatrix;
		}
	}
	void LightComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> color;
			archive >> (uint32_t&)type;
			archive >> energy;
			archive >> range;
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
			archive << range;
			archive << fov;
			archive << shadowBias;
			archive << radius;
			archive << width;
			archive << height;

			archive << lensFlareNames;
		}
	}
	void CameraComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> width;
			archive >> height;
			archive >> zNearP;
			archive >> zFarP;
			archive >> fov;
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
	void EnvironmentProbeComponent::Serialize(wiArchive& archive)
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
	void ForceFieldComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> type;
			archive >> gravity;
			archive >> range;
		}
		else
		{
			archive << _flags;
			archive << type;
			archive << gravity;
			archive << range;
		}
	}
	void DecalComponent::Serialize(wiArchive& archive)
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
	void AnimationComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> length;

			size_t channelCount;
			archive >> channelCount;
			channels.resize(channelCount);
			for (size_t i = 0; i < channelCount; ++i)
			{
				archive >> channels[i]._flags;
				archive >> channels[i].target;
				archive >> (uint32_t&)channels[i].type;
				archive >> (uint32_t&)channels[i].mode;
				archive >> channels[i].keyframe_times;
				archive >> channels[i].keyframe_data;
			}
		}
		else
		{
			archive << _flags;
			archive << length;

			archive << channels.size();
			for (size_t i = 0; i < channels.size(); ++i)
			{
				archive << channels[i]._flags;
				archive << channels[i].target;
				archive << (uint32_t&)channels[i].type;
				archive << (uint32_t&)channels[i].mode;
				archive << channels[i].keyframe_times;
				archive << channels[i].keyframe_data;
			}
		}
	}

	void Scene::Serialize(wiArchive& archive)
	{
		names.Serialize(archive);
		layers.Serialize(archive);
		transforms.Serialize(archive);
		prev_transforms.Serialize(archive);
		hierarchy.Serialize(archive);
		materials.Serialize(archive);
		meshes.Serialize(archive);
		objects.Serialize(archive);
		aabb_objects.Serialize(archive);
		rigidbodies.Serialize(archive);
		softbodies.Serialize(archive);
		armatures.Serialize(archive);
		lights.Serialize(archive);
		aabb_lights.Serialize(archive);
		cameras.Serialize(archive);
		probes.Serialize(archive);
		aabb_probes.Serialize(archive);
		forces.Serialize(archive);
		decals.Serialize(archive);
		aabb_decals.Serialize(archive);
		animations.Serialize(archive);
		emitters.Serialize(archive);
		hairs.Serialize(archive);

		if (archive.IsReadMode())
		{
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
		}
		else
		{
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
		}

	}

}
