#include "wiArchive.h"
#include "wiResourceManager.h"
#include "wiScene.h"
#include "wiInitializer.h"
#include "wiApplication.h"
#include "wiECS.h"
#include "wiRenderer.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <wiScene_Components.h>

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(wi::vector<XMFLOAT2>);
PYBIND11_MAKE_OPAQUE(wi::vector<XMFLOAT3>);
PYBIND11_MAKE_OPAQUE(wi::vector<XMFLOAT4>);
PYBIND11_MAKE_OPAQUE(wi::vector<XMUINT4>);
PYBIND11_MAKE_OPAQUE(wi::vector<uint8_t>);
PYBIND11_MAKE_OPAQUE(wi::vector<uint32_t>);
PYBIND11_MAKE_OPAQUE(wi::vector<wi::scene::MeshComponent::MeshSubset>);
PYBIND11_MAKE_OPAQUE(wi::scene::NameComponent);
PYBIND11_MAKE_OPAQUE(wi::scene::MeshComponent);
PYBIND11_MAKE_OPAQUE(wi::scene::MaterialComponent);
PYBIND11_MAKE_OPAQUE(wi::scene::TransformComponent);
PYBIND11_MAKE_OPAQUE(wi::scene::ObjectComponent);

static wi::Application app;

void init_wicked(py::module_& mod)
{
	//TODO we can probably initialize less stuff

	sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_VIDEO);
	if (!system) {
		throw sdl2::SDLError("Error creating SDL2 system");
	}

	sdl2::window_ptr_t window = sdl2::make_window("WickedBlenderExporter",
												  SDL_WINDOWPOS_CENTERED,
												  SDL_WINDOWPOS_CENTERED,
												  50, 50,
												  SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN);
	if (!window) {
		throw sdl2::SDLError("Error creating window");
	}

	app.SetWindow(window.get());
	wi::renderer::SetShaderSourcePath(WICKED_ROOT_DIR"/WickedEngine/shaders/");
	wi::renderer::SetShaderPath(WICKED_ROOT_DIR"/build/BlenderExporter/shaders/");

	wi::initializer::InitializeComponentsImmediate();
}

void export_primitives(py::module_& mod)
{
	py::class_<XMFLOAT2>(mod, "XMFLOAT2")
		.def(py::init<>())
		.def(py::init<float, float>())
		;
	py::class_<XMFLOAT3>(mod, "XMFLOAT3")
		.def(py::init<>())
		.def(py::init<float, float, float>())
		;
	py::class_<XMFLOAT4>(mod, "XMFLOAT4")
		.def(py::init<>())
		.def(py::init<float, float, float, float>())
		;
	py::class_<XMUINT4>(mod, "XMUINT4")
		.def(py::init<>())
		.def(py::init<uint32_t, uint32_t, uint32_t, uint32_t>())
		;

	py::bind_vector<wi::vector<XMFLOAT2>>(mod, "VectorXMFLOAT2");
	py::bind_vector<wi::vector<XMFLOAT3>>(mod, "VectorXMFLOAT3");
	py::bind_vector<wi::vector<XMFLOAT4>>(mod, "VectorXMFLOAT4");
	py::bind_vector<wi::vector<XMUINT4 >>(mod, "VectorXMUINT4");
	py::bind_vector<wi::vector<uint8_t >>(mod, "VectorU8");
	py::bind_vector<wi::vector<uint32_t>>(mod, "VectorU32");
	py::bind_vector<wi::vector<wi::scene::MeshComponent::MeshSubset>>(mod, "VectorMeshSubset");

}

void export_resourcemanager(py::module_& mod)
{
	py::enum_<wi::resourcemanager::Mode>(mod, "ResourceManagerMode")
		.value("DISCARD_FILEDATA_AFTER_LOAD", wi::resourcemanager::Mode::DISCARD_FILEDATA_AFTER_LOAD)
		.value("ALLOW_RETAIN_FILEDATA", wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA)
		.value("ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING", wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING)
		.export_values();

	mod.def("resourcemanager_SetMode", &wi::resourcemanager::SetMode);
}

void export_Archive(py::module_& mod)
{
	py::class_<wi::Archive>(mod, "Archive")
		.def(py::init<>())
		.def(py::init<const std::string&, bool>())
		.def("IsOpen", &wi::Archive::IsOpen)
		.def("Close", &wi::Archive::Close)
		.def("SaveFile", &wi::Archive::SaveFile)
		.def("SaveHeaderFile", &wi::Archive::SaveHeaderFile)
		;
}

void export_Scene(py::module_& mod)
{
	py::class_<wi::scene::Scene>(mod, "Scene")
		.def(py::init<>())
		//.def_readwrite("componentLibrary", &wi::scene::Scene::componentLibrary)
		.def("names", 		[](const wi::scene::Scene &_this){ return &_this.names; })
		.def("meshes", 		[](const wi::scene::Scene &_this){ return &_this.meshes; })
		.def("transforms", 	[](const wi::scene::Scene &_this){ return &_this.transforms; })
		.def("materials", 	[](const wi::scene::Scene &_this){ return &_this.materials; })
		.def("objects", 	[](const wi::scene::Scene &_this){ return &_this.objects; })
		.def("Serialize", &wi::scene::Scene::Serialize)
		.def("Entity_CreateTransform", &wi::scene::Scene::Entity_CreateTransform)
		.def("Entity_CreateMaterial", &wi::scene::Scene::Entity_CreateMaterial)
		.def("Entity_CreateObject", &wi::scene::Scene::Entity_CreateObject)
		.def("Entity_CreateMesh", &wi::scene::Scene::Entity_CreateMesh)
		.def("Entity_CreateLight", &wi::scene::Scene::Entity_CreateLight)
		.def("Entity_CreateForce", &wi::scene::Scene::Entity_CreateForce)
		.def("Entity_CreateEnvironmentProbe", &wi::scene::Scene::Entity_CreateEnvironmentProbe)
		.def("Entity_CreateDecal", &wi::scene::Scene::Entity_CreateDecal)
		.def("Entity_CreateCamera", &wi::scene::Scene::Entity_CreateCamera)
		.def("Entity_CreateEmitter", &wi::scene::Scene::Entity_CreateEmitter)
		.def("Entity_CreateHair", &wi::scene::Scene::Entity_CreateHair)
		.def("Entity_CreateSound", &wi::scene::Scene::Entity_CreateSound)
		.def("Entity_CreateVideo", &wi::scene::Scene::Entity_CreateVideo)
		.def("Entity_CreateCube", &wi::scene::Scene::Entity_CreateCube)
		.def("Entity_CreatePlane", &wi::scene::Scene::Entity_CreatePlane)
		.def("Entity_CreateSphere", &wi::scene::Scene::Entity_CreateSphere)
		.def("Component_Attach", &wi::scene::Scene::Component_Attach)
		.def("Component_Attach", [](wi::scene::Scene& _this, wi::ecs::Entity entity, wi::ecs::Entity parent) { return _this.Component_Attach(entity, parent); })
		.def("Component_Detach", &wi::scene::Scene::Component_Detach)
		.def("Component_DetachChildren", &wi::scene::Scene::Component_DetachChildren)
		.def("RunAnimationUpdateSystem", &wi::scene::Scene::RunAnimationUpdateSystem)
		.def("RunTransformUpdateSystem", &wi::scene::Scene::RunTransformUpdateSystem)
		.def("RunHierarchyUpdateSystem", &wi::scene::Scene::RunHierarchyUpdateSystem)
		.def("RunExpressionUpdateSystem", &wi::scene::Scene::RunExpressionUpdateSystem)
		.def("RunProceduralAnimationUpdateSystem", &wi::scene::Scene::RunProceduralAnimationUpdateSystem)
		.def("RunArmatureUpdateSystem", &wi::scene::Scene::RunArmatureUpdateSystem)
		.def("RunMeshUpdateSystem", &wi::scene::Scene::RunMeshUpdateSystem)
		.def("RunMaterialUpdateSystem", &wi::scene::Scene::RunMaterialUpdateSystem)
		.def("RunImpostorUpdateSystem", &wi::scene::Scene::RunImpostorUpdateSystem)
		.def("RunObjectUpdateSystem", &wi::scene::Scene::RunObjectUpdateSystem)
		.def("RunCameraUpdateSystem", &wi::scene::Scene::RunCameraUpdateSystem)
		.def("RunDecalUpdateSystem", &wi::scene::Scene::RunDecalUpdateSystem)
		.def("RunProbeUpdateSystem", &wi::scene::Scene::RunProbeUpdateSystem)
		.def("RunForceUpdateSystem", &wi::scene::Scene::RunForceUpdateSystem)
		.def("RunLightUpdateSystem", &wi::scene::Scene::RunLightUpdateSystem)
		.def("RunParticleUpdateSystem", &wi::scene::Scene::RunParticleUpdateSystem)
		.def("RunWeatherUpdateSystem", &wi::scene::Scene::RunWeatherUpdateSystem)
		.def("RunSoundUpdateSystem", &wi::scene::Scene::RunSoundUpdateSystem)
		.def("RunVideoUpdateSystem", &wi::scene::Scene::RunVideoUpdateSystem)
		.def("RunScriptUpdateSystem", &wi::scene::Scene::RunScriptUpdateSystem)
		.def("RunSpriteUpdateSystem", &wi::scene::Scene::RunSpriteUpdateSystem)
		.def("RunFontUpdateSystem", &wi::scene::Scene::RunFontUpdateSystem)
		;
}

template<typename T>
void register_component_manager(py::module_& mod, const char* name)
{
	typedef wi::ecs::ComponentManager<T> Manager;
	py::class_<Manager, std::unique_ptr<Manager, py::nodelete>>(mod, name)
		.def("Clear", &Manager::Clear)
		// .def("Copy", &Manager::Copy)
		// .def("Merge", &Manager::Merge)
		.def("Create", &Manager::Create)
		.def("Remove", &Manager::Remove)
		.def("MoveItem", &Manager::MoveItem)
		.def("Contains", &Manager::Contains)
		.def("GetComponent", [](Manager* _this, wi::ecs::Entity entity) { return static_cast<T*>(_this->GetComponent(entity)); })
		.def("GetIndex", &Manager::GetIndex)
		.def("GetCount", [](const Manager* _this) { return _this->GetCount(); })
		.def("GetEntity", &Manager::GetEntity)
		.def("GetEntityArray", &Manager::GetEntityArray)
		.def("GetComponentArray", &Manager::GetComponentArray)
		;
}

void export_ECS(py::module_& mod)
{
	using namespace wi::ecs;
	using namespace wi::scene;
	// py::class_<ComponentLibrary>(mod, "ComponentLibrary");

	// py::class_<Entity>(mod, "Entity");
	mod.def("CreateEntity", &CreateEntity);

	//maybe Components need to not handle memory by python
	// `std::unique_ptr<TComponent, py::nodelete>`
	py::class_<MeshComponent::MeshSubset>(mod, "MeshComponent_MeshSubset")
		.def(py::init<>())
		.def_readwrite("materialID", &MeshComponent::MeshSubset::materialID)
		.def_readwrite("indexOffset", &MeshComponent::MeshSubset::indexOffset)
		.def_readwrite("indexCount", &MeshComponent::MeshSubset::indexCount)
		.def_readwrite("materialIndex", &MeshComponent::MeshSubset::materialIndex)
		;

	py::class_<NameComponent, std::unique_ptr<NameComponent, py::nodelete>>(mod, "NameComponent")
		.def_readwrite("name", &NameComponent::name)
		;

	py::class_<MeshComponent, std::unique_ptr<MeshComponent, py::nodelete>>(mod, "MeshComponent")
		.def_readwrite("vertex_positions", &MeshComponent::vertex_positions)
		.def_readwrite("vertex_normals", &MeshComponent::vertex_normals)
		.def_readwrite("vertex_tangents", &MeshComponent::vertex_tangents)
		.def_readwrite("vertex_uvset_0", &MeshComponent::vertex_uvset_0)
		.def_readwrite("vertex_uvset_1", &MeshComponent::vertex_uvset_1)
		.def_readwrite("vertex_boneindices", &MeshComponent::vertex_boneindices)
		.def_readwrite("vertex_boneweights", &MeshComponent::vertex_boneweights)
		.def_readwrite("vertex_atlas", &MeshComponent::vertex_atlas)
		.def_readwrite("vertex_colors", &MeshComponent::vertex_colors)
		.def_readwrite("vertex_windweights", &MeshComponent::vertex_windweights)
		.def_readwrite("indices", &MeshComponent::indices)
		.def_readwrite("subsets", &MeshComponent::subsets)
		.def_readwrite("tessellationFactor", &MeshComponent::tessellationFactor)
		.def_readwrite("armatureID", &MeshComponent::armatureID)
		.def("ComputeNormals", &MeshComponent::ComputeNormals)
		.def("ComputeNormals", &MeshComponent::CreateRenderData)
		;

	py::enum_<MeshComponent::COMPUTE_NORMALS>(mod, "MeshComponentComputeNormals")
		.value("HARD", MeshComponent::COMPUTE_NORMALS::COMPUTE_NORMALS_HARD)
		.value("SMOOTH", MeshComponent::COMPUTE_NORMALS::COMPUTE_NORMALS_SMOOTH)
		.value("SMOOTH_FAST", MeshComponent::COMPUTE_NORMALS::COMPUTE_NORMALS_SMOOTH_FAST)
		.export_values()
		;

	py::class_<TransformComponent>(mod, "TransformComponent")
		.def(py::init<>())
		.def_readwrite("scale_local", &TransformComponent::scale_local)
		.def_readwrite("rotation_local", &TransformComponent::rotation_local)
		.def_readwrite("translation_local", &TransformComponent::translation_local)
		.def_readwrite("world", &TransformComponent::world)
		.def("GetPosition", &TransformComponent::GetPosition)
		.def("GetRotation", &TransformComponent::GetRotation)
		.def("GetScale", &TransformComponent::GetScale)
		.def("GetPositionV", &TransformComponent::GetPositionV)
		.def("GetRotationV", &TransformComponent::GetRotationV)
		.def("GetScaleV", &TransformComponent::GetScaleV)
		.def("GetLocalMatrix", &TransformComponent::GetLocalMatrix)
		.def("UpdateTransform", &TransformComponent::UpdateTransform)
		.def("UpdateTransform_Parented", &TransformComponent::UpdateTransform_Parented)
		.def("ApplyTransform", &TransformComponent::ApplyTransform)
		.def("ClearTransform", &TransformComponent::ClearTransform)
		// .def("Translate", &TransformComponent::Translate)
		// .def("Translate", &TransformComponent::Translate)
		.def("RotateRollPitchYaw", &TransformComponent::RotateRollPitchYaw)
		// .def("Rotate", &TransformComponent::Rotate)
		// .def("Rotate", &TransformComponent::Rotate)
		// .def("Scale", &TransformComponent::Scale)
		// .def("Scale", &TransformComponent::Scale)
		// .def("MatrixTransform", &TransformComponent::MatrixTransform)
		// .def("MatrixTransform", &TransformComponent::MatrixTransform)
		.def("Lerp", &TransformComponent::Lerp)
		.def("CatmullRom", &TransformComponent::CatmullRom)
		;


	py::class_<MaterialComponent, std::unique_ptr<MaterialComponent, py::nodelete>>(mod, "MaterialComponent")
		.def_readwrite("baseColor", &MaterialComponent::baseColor)
		;

	py::class_<ObjectComponent, std::unique_ptr<ObjectComponent, py::nodelete>>(mod, "ObjectComponent")
		.def_readwrite("meshID", &ObjectComponent::meshID)
		.def_readwrite("cascadeMask", &ObjectComponent::cascadeMask)
		.def_readwrite("filterMask", &ObjectComponent::filterMask)
		.def_readwrite("color", &ObjectComponent::color)
		.def_readwrite("emissiveColor", &ObjectComponent::emissiveColor)
		.def_readwrite("userStencilRef", &ObjectComponent::userStencilRef)
		.def_readwrite("lod_distance_multiplier", &ObjectComponent::lod_distance_multiplier)
		.def_readwrite("draw_distance", &ObjectComponent::draw_distance)
		.def_readwrite("lightmapWidth", &ObjectComponent::lightmapWidth)
		.def_readwrite("lightmapHeight", &ObjectComponent::lightmapHeight)
		.def_readwrite("lightmapTextureData", &ObjectComponent::lightmapTextureData)
		.def_readwrite("sort_priority", &ObjectComponent::sort_priority)
		.def_readwrite("filterMaskDynamic", &ObjectComponent::filterMaskDynamic)
		.def_readwrite("lightmap", &ObjectComponent::lightmap)
		.def_readwrite("lightmapIterationCount", &ObjectComponent::lightmapIterationCount)
		.def_readwrite("center", &ObjectComponent::center)
		.def_readwrite("radius", &ObjectComponent::radius)
		.def_readwrite("fadeDistance", &ObjectComponent::fadeDistance)
		.def_readwrite("lod", &ObjectComponent::lod)
		.def_readwrite("mesh_index", &ObjectComponent::mesh_index)
		.def_readwrite("sort_bits", &ObjectComponent::sort_bits)
		;

	// MANAGERS -------------------------------------------
	register_component_manager<NameComponent>(mod, "NameComponentManager");
	register_component_manager<TransformComponent>(mod, "TransformComponentManager");
	register_component_manager<MeshComponent>(mod, "MeshComponentManager");
	register_component_manager<MaterialComponent>(mod, "MaterialComponentManager");
	register_component_manager<ObjectComponent>(mod, "ObjectComponentManager");
}

PYBIND11_MODULE(pywickedengine, mod)
{
	init_wicked(mod);
	export_primitives(mod);
	export_resourcemanager(mod);
	export_ECS(mod);
	export_Archive(mod);
	export_Scene(mod);
}
