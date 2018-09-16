#include "wiSceneSystem.h"

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

			archive >> baseColorMapName;
			archive >> surfaceMapName;
			archive >> normalMapName;
			archive >> displacementMapName;

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
		}
		else
		{
		}
	}
	void ObjectComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void RigidBodyPhysicsComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void SoftBodyPhysicsComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void ArmatureComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void LightComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void CameraComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void EnvironmentProbeComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void ForceFieldComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void DecalComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
		}
	}
	void AnimationComponent::Serialize(wiArchive& archive)
	{
		if (archive.IsReadMode())
		{
		}
		else
		{
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
	}

}
