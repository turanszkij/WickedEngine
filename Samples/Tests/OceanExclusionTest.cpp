// Tests of the ocean exclusion volumes (wi::scene::OceanExclusionComponent):
//	the shapes, the scene queries, the shader data and the serialization.
//	This doesn't use the graphics device, the rendering is tested by the "Ocean Exclusion Test" of the Tests application.
#include "WickedEngine.h"

#include <cmath>
#include <cstring>
#include <iostream>

using namespace wi::ecs;
using namespace wi::scene;

namespace
{
	int failures = 0;

	void Check(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << "Ocean exclusion test failed: " << message << '\n';
			failures++;
		}
	}

	// CPU-side check of the packed shader data, independent of the component cache.
	bool ShaderExclusionCheck(const wi::vector<uint8_t>& data, uint32_t count, const XMFLOAT3& P)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			ShaderOceanExclusion exclusion;
			std::memcpy(&exclusion, data.data() + i * sizeof(ShaderOceanExclusion), sizeof(exclusion));
			if (P.x < exclusion.aabb_min.x || P.x > exclusion.aabb_max.x ||
				P.y < exclusion.aabb_min.y || P.y > exclusion.aabb_max.y ||
				P.z < exclusion.aabb_min.z || P.z > exclusion.aabb_max.z)
			{
				continue;
			}
			const auto dot = [&](const XMFLOAT4& row) { return row.x * P.x + row.y * P.y + row.z * P.z + row.w; };
			const XMFLOAT3 L = XMFLOAT3(dot(exclusion.transform_x), dot(exclusion.transform_y), dot(exclusion.transform_z));
			if (std::abs(L.y) > 1)
			{
				continue;
			}
			switch (exclusion.shape)
			{
			default:
			case OCEAN_EXCLUSION_SHAPE_BOX:
				if (std::abs(L.x) <= 1 && std::abs(L.z) <= 1)
				{
					return true;
				}
				break;
			case OCEAN_EXCLUSION_SHAPE_SPHERE:
				if (L.x * L.x + L.y * L.y + L.z * L.z <= 1)
				{
					return true;
				}
				break;
			case OCEAN_EXCLUSION_SHAPE_CYLINDER:
				if (L.x * L.x + L.z * L.z <= 1)
				{
					return true;
				}
				break;
			case OCEAN_EXCLUSION_SHAPE_POLYGON:
				{
					bool inside = false;
					XMFLOAT2 a;
					std::memcpy(&a, data.data() + exclusion.polygon_offset + (exclusion.polygon_count - 1) * sizeof(XMFLOAT2), sizeof(a));
					for (uint32_t j = 0; j < exclusion.polygon_count; ++j)
					{
						XMFLOAT2 b;
						std::memcpy(&b, data.data() + exclusion.polygon_offset + j * sizeof(XMFLOAT2), sizeof(b));
						if ((a.y > L.z) != (b.y > L.z) && L.x < (b.x - a.x) * (L.z - a.y) / (b.y - a.y) + a.x)
						{
							inside = !inside;
						}
						a = b;
					}
					if (inside)
					{
						return true;
					}
				}
				break;
			}
		}
		return false;
	}

	// An outline like the letter L: a square of 4 with one quarter cut out
	wi::vector<XMFLOAT2> LetterL()
	{
		return { XMFLOAT2(0, 0), XMFLOAT2(4, 0), XMFLOAT2(4, 2), XMFLOAT2(2, 2), XMFLOAT2(2, 4), XMFLOAT2(0, 4) };
	}

	void TestShapes()
	{
		OceanExclusionComponent exclusion;
		Check(exclusion.shape == OceanExclusionComponent::Shape::Box, "the default shape is not the box");
		Check(!exclusion.IsDisabled(), "a new volume is disabled");

		// Box: [-1, 1] on all axes
		Check(exclusion.ContainsLocal(XMFLOAT3(0, 0, 0)), "box doesn't contain its center");
		Check(exclusion.ContainsLocal(XMFLOAT3(0.99f, -0.99f, 0.99f)), "box doesn't contain a point near its corner");
		Check(!exclusion.ContainsLocal(XMFLOAT3(1.01f, 0, 0)), "box contains a point beyond X");
		Check(!exclusion.ContainsLocal(XMFLOAT3(0, 1.01f, 0)), "box contains a point beyond Y");
		Check(!exclusion.ContainsLocal(XMFLOAT3(0, 0, -1.01f)), "box contains a point beyond Z");

		// Sphere: radius 1
		exclusion.shape = OceanExclusionComponent::Shape::Sphere;
		Check(exclusion.ContainsLocal(XMFLOAT3(0.57f, 0.57f, 0.57f)), "sphere doesn't contain a point inside");
		Check(!exclusion.ContainsLocal(XMFLOAT3(0.6f, 0.6f, 0.6f)), "sphere contains a point of the box corner");
		Check(!exclusion.ContainsLocal(XMFLOAT3(0, 1.01f, 0)), "sphere contains a point beyond Y");

		// Cylinder: radius 1 around Y, Y in [-1, 1]
		exclusion.shape = OceanExclusionComponent::Shape::Cylinder;
		Check(exclusion.ContainsLocal(XMFLOAT3(0.7f, 0.99f, 0.7f)), "cylinder doesn't contain a point near its top rim");
		Check(!exclusion.ContainsLocal(XMFLOAT3(0.72f, 0, 0.72f)), "cylinder contains a point of the box corner");
		Check(!exclusion.ContainsLocal(XMFLOAT3(0, -1.01f, 0)), "cylinder contains a point beyond Y");

		// Polygon: an outline on XZ, Y in [-1, 1]
		exclusion.shape = OceanExclusionComponent::Shape::Polygon;
		Check(!exclusion.ContainsLocal(XMFLOAT3(0, 0, 0)), "polygon without points contains a point");
		exclusion.polygon = { XMFLOAT2(0, 0), XMFLOAT2(1, 1) };
		Check(!exclusion.ContainsLocal(XMFLOAT3(0.5f, 0, 0.5f)), "polygon of two points contains a point");
		exclusion.polygon = LetterL();
		Check(exclusion.ContainsLocal(XMFLOAT3(1, 0, 1)), "polygon doesn't contain a point inside");
		Check(exclusion.ContainsLocal(XMFLOAT3(3, 0, 1)), "polygon doesn't contain a point in its first arm");
		Check(exclusion.ContainsLocal(XMFLOAT3(1, 0, 3)), "polygon doesn't contain a point in its second arm");
		Check(!exclusion.ContainsLocal(XMFLOAT3(3, 0, 3)), "polygon contains a point of the part that is cut out");
		Check(!exclusion.ContainsLocal(XMFLOAT3(-1, 0, 1)), "polygon contains a point outside");
		Check(!exclusion.ContainsLocal(XMFLOAT3(1, 1.01f, 1)), "polygon contains a point beyond Y");

		// The outline can turn the other way around too:
		std::reverse(exclusion.polygon.begin(), exclusion.polygon.end());
		Check(exclusion.ContainsLocal(XMFLOAT3(1, 0, 3)), "reversed polygon doesn't contain a point inside");
		Check(!exclusion.ContainsLocal(XMFLOAT3(3, 0, 3)), "reversed polygon contains a point of the part that is cut out");

		const wi::primitive::AABB bounds = exclusion.GetLocalAABB();
		Check(bounds.getMin().x == 0 && bounds.getMin().y == -1 && bounds.getMin().z == 0, "wrong minimum of the polygon bounds");
		Check(bounds.getMax().x == 4 && bounds.getMax().y == 1 && bounds.getMax().z == 4, "wrong maximum of the polygon bounds");
	}

	void TestTransform()
	{
		// A box of 20 x 4 x 6, turned by 90 degrees around Y, at (100, 10, -50): it is long on the world Z axis
		TransformComponent transform;
		transform.Scale(XMFLOAT3(10, 2, 3));
		transform.RotateRollPitchYaw(XMFLOAT3(0, XM_PIDIV2, 0));
		transform.Translate(XMFLOAT3(100, 10, -50));
		transform.UpdateTransform();

		OceanExclusionComponent exclusion;
		exclusion.UpdateTransform(transform.world);

		Check(exclusion.Contains(XMFLOAT3(100, 10, -50)), "transformed box doesn't contain its center");
		Check(exclusion.Contains(XMFLOAT3(100, 11.9f, -41)), "transformed box doesn't contain a point along its long side");
		Check(exclusion.Contains(XMFLOAT3(102.9f, 8.1f, -59)), "transformed box doesn't contain a point near its corner");
		Check(!exclusion.Contains(XMFLOAT3(109, 10, -50)), "transformed box was not rotated");
		Check(!exclusion.Contains(XMFLOAT3(100, 12.1f, -50)), "transformed box contains a point above it");
		Check(!exclusion.Contains(XMFLOAT3(0, 0, 0)), "transformed box contains the origin");

		// The bounds are in world space:
		Check(std::abs(exclusion.aabb.getMin().x - 97) < 0.01f && std::abs(exclusion.aabb.getMax().x - 103) < 0.01f, "wrong bounds on X");
		Check(std::abs(exclusion.aabb.getMin().y - 8) < 0.01f && std::abs(exclusion.aabb.getMax().y - 12) < 0.01f, "wrong bounds on Y");
		Check(std::abs(exclusion.aabb.getMin().z + 60) < 0.01f && std::abs(exclusion.aabb.getMax().z + 40) < 0.01f, "wrong bounds on Z");

		// A disabled volume contains nothing:
		exclusion.SetDisabled(true);
		Check(exclusion.IsDisabled(), "the volume was not disabled");
		Check(!exclusion.Contains(XMFLOAT3(100, 10, -50)), "disabled volume contains its center");
		exclusion.SetDisabled(false);
		Check(exclusion.Contains(XMFLOAT3(100, 10, -50)), "the volume was not enabled again");

		// A polygon from world space points: a triangle around (100, -50) on the ground, it is projected onto the local XZ plane
		const XMFLOAT3 points[] = { XMFLOAT3(90, 0, -60), XMFLOAT3(110, 55, -60), XMFLOAT3(100, -3, -30) };
		exclusion.SetPolygonFromWorldPoints(points, 3);
		Check(exclusion.shape == OceanExclusionComponent::Shape::Polygon, "the shape didn't become a polygon");
		Check(exclusion.polygon.size() == 3, "the polygon doesn't have the points that were given");
		Check(exclusion.Contains(XMFLOAT3(100, 10, -50)), "polygon from world points doesn't contain a point inside");
		Check(exclusion.Contains(XMFLOAT3(100, 11.9f, -35)), "polygon from world points doesn't contain a point near its tip");
		Check(!exclusion.Contains(XMFLOAT3(92, 10, -35)), "polygon from world points contains a point beside its tip");
		Check(!exclusion.Contains(XMFLOAT3(100, 12.1f, -50)), "polygon from world points is not bounded by the height of the volume");
		Check(exclusion.aabb.getMax().z > -31 && exclusion.aabb.getMin().x < 91, "the bounds don't follow the outline");
	}

	// Compares what the scene answers with what the shaders compute from the uploaded data, on a grid of points:
	void CheckShaderDataAgainstScene(const Scene& scene, const char* message)
	{
		wi::vector<uint8_t> data;
		const uint32_t count = scene.WriteOceanExclusionShaderData(data);
		int differences = 0;
		int inside = 0;
		for (float x = -40; x <= 40; x += 2.5f)
		{
			for (float y = -12; y <= 12; y += 1.5f)
			{
				for (float z = -40; z <= 40; z += 2.5f)
				{
					const XMFLOAT3 P = XMFLOAT3(x + 0.13f, y + 0.07f, z - 0.11f); // off the faces of the volumes
					const bool scene_result = scene.IsOceanExcludedAt(P);
					inside += scene_result ? 1 : 0;
					differences += scene_result != ShaderExclusionCheck(data, count, P) ? 1 : 0;
				}
			}
		}
		Check(differences == 0, message);
		Check(inside > 100, "the test points don't go through the volumes");
	}

	void TestScene()
	{
		Scene scene;
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(0, 0, 0)), "an empty scene has an ocean exclusion");
		wi::vector<uint8_t> data = { 1, 2, 3 };
		Check(scene.WriteOceanExclusionShaderData(data) == 0 && data.empty(), "an empty scene has shader data");

		const Entity box = scene.Entity_CreateOceanExclusion("box", XMFLOAT3(-20, 0, -20), XMFLOAT3(8, 5, 4));
		const Entity sphere = scene.Entity_CreateOceanExclusion("sphere", XMFLOAT3(20, 0, -20), XMFLOAT3(9, 9, 9), OceanExclusionComponent::Shape::Sphere);
		const Entity cylinder = scene.Entity_CreateOceanExclusion("cylinder", XMFLOAT3(-20, 0, 20), XMFLOAT3(7, 10, 7), OceanExclusionComponent::Shape::Cylinder);
		const Entity polygon = scene.Entity_CreateOceanExclusion("polygon", XMFLOAT3(12, -2, 12), XMFLOAT3(4, 6, 4), OceanExclusionComponent::Shape::Polygon);
		const Entity empty_polygon = scene.Entity_CreateOceanExclusion("empty polygon", XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), OceanExclusionComponent::Shape::Polygon);
		Check(scene.ocean_exclusions.GetCount() == 5, "the volumes were not created");
		Check(scene.names.Contains(box) && scene.transforms.Contains(box) && scene.layers.Contains(box), "the entity of a volume lacks components");
		scene.ocean_exclusions.GetComponent(polygon)->polygon = LetterL();

		// A new volume can be asked about before the scene is updated:
		Check(scene.IsOceanExcludedAt(XMFLOAT3(-20, 0, -20)), "a new volume is not there before the update");

		// The transforms are changed, then the update system picks them up:
		scene.transforms.GetComponent(box)->RotateRollPitchYaw(XMFLOAT3(0.3f, 0.7f, -0.2f));
		scene.transforms.GetComponent(polygon)->RotateRollPitchYaw(XMFLOAT3(0, 0.5f, 0));
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			scene.transforms[i].UpdateTransform();
		}
		wi::jobsystem::context ctx;
		scene.RunOceanExclusionUpdateSystem(ctx);
		wi::jobsystem::Wait(ctx);

		Check(scene.IsOceanExcludedAt(XMFLOAT3(-20, 0, -20)), "the box doesn't contain its center");
		Check(scene.IsOceanExcludedAt(XMFLOAT3(20, 8.9f, -20)), "the sphere doesn't contain a point near its top");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(26.5f, 6.5f, -20)), "the sphere contains a point of its box corner");
		Check(scene.IsOceanExcludedAt(XMFLOAT3(-15.2f, 9.9f, 24.8f)), "the cylinder doesn't contain a point near its top rim");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(-20, 10.1f, 20)), "the cylinder contains a point above it");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(0, 0, 0)), "the polygon without points contains its center");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(0, 30, 0)), "a point far above is in a volume");

		// What the shaders get: the volumes with a shape, then the outline of the polygon
		const uint32_t count = scene.WriteOceanExclusionShaderData(data);
		Check(count == 4, "the polygon without points was not left out of the shader data");
		Check(data.size() == 4 * sizeof(ShaderOceanExclusion) + 6 * sizeof(XMFLOAT2), "wrong size of the shader data");
		Check(sizeof(ShaderOceanExclusion) % 16 == 0, "the shader structure is not aligned");
		ShaderOceanExclusion shader_polygon;
		std::memcpy(&shader_polygon, data.data() + 3 * sizeof(ShaderOceanExclusion), sizeof(shader_polygon));
		Check(shader_polygon.shape == OCEAN_EXCLUSION_SHAPE_POLYGON, "wrong shape in the shader data");
		Check(shader_polygon.polygon_count == 6, "wrong outline point count in the shader data");
		Check(shader_polygon.polygon_offset == 4 * sizeof(ShaderOceanExclusion), "wrong outline offset in the shader data");
		Check(std::memcmp(data.data() + shader_polygon.polygon_offset, LetterL().data(), 6 * sizeof(XMFLOAT2)) == 0, "wrong outline points in the shader data");
		CheckShaderDataAgainstScene(scene, "the shader data doesn't give the same result as the scene");

		// A disabled volume is not in the shader data:
		scene.ocean_exclusions.GetComponent(sphere)->SetDisabled(true);
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(20, 0, -20)), "the disabled sphere contains its center");
		Check(scene.WriteOceanExclusionShaderData(data) == 3, "the disabled sphere is in the shader data");
		CheckShaderDataAgainstScene(scene, "the shader data doesn't give the same result as the scene after disabling a volume");
		scene.ocean_exclusions.GetComponent(sphere)->SetDisabled(false);

		// Removing the entity removes the volume:
		scene.Entity_Remove(cylinder);
		Check(scene.ocean_exclusions.GetCount() == 4, "the volume was not removed with its entity");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(-20, 0, 20)), "the removed cylinder contains its center");

		// A volume without a transform is the shape in world space:
		const Entity bare = CreateEntity();
		scene.ocean_exclusions.Create(bare);
		scene.RunOceanExclusionUpdateSystem(ctx);
		wi::jobsystem::Wait(ctx);
		Check(scene.IsOceanExcludedAt(XMFLOAT3(0.5f, 0.5f, 0.5f)), "a volume without transform is not a unit box at the origin");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(1.5f, 0.5f, 0.5f)), "a volume without transform is larger than a unit box");

		// Without an ocean nothing is under the ocean:
		Check(!scene.IsUnderOcean(XMFLOAT3(50, -100, 50)), "a point is under the ocean of a scene without ocean");

		// The polygon outline from a spline: four nodes around (60, 60) make a closed curve
		const Entity spline_entity = CreateEntity();
		SplineComponent& spline = scene.splines.Create(spline_entity);
		const XMFLOAT3 node_positions[] = { XMFLOAT3(50, 0, 50), XMFLOAT3(70, 3, 50), XMFLOAT3(70, -3, 70), XMFLOAT3(50, 0, 70) };
		for (const XMFLOAT3& position : node_positions)
		{
			const Entity node = CreateEntity();
			TransformComponent& node_transform = scene.transforms.Create(node);
			node_transform.Translate(position);
			node_transform.UpdateTransform();
			spline.spline_node_entities.push_back(node);
		}
		const Entity from_spline = scene.Entity_CreateOceanExclusion("from spline", XMFLOAT3(60, 0, 60), XMFLOAT3(1, 5, 1));
		Check(!scene.SetOceanExclusionPolygonFromSpline(spline_entity, spline_entity), "an entity without ocean exclusion took a polygon");
		Check(!scene.SetOceanExclusionPolygonFromSpline(from_spline, box), "an entity without spline gave a polygon");
		Check(scene.SetOceanExclusionPolygonFromSpline(from_spline, spline_entity, 8), "the polygon was not made from the spline");
		const OceanExclusionComponent& spline_exclusion = *scene.ocean_exclusions.GetComponent(from_spline);
		Check(spline_exclusion.shape == OceanExclusionComponent::Shape::Polygon, "the shape didn't become a polygon from the spline");
		Check(spline_exclusion.polygon.size() == 4 * 8, "wrong number of outline points from the spline");
		Check(std::abs(spline_exclusion.polygon[0].x + 10) < 0.001f && std::abs(spline_exclusion.polygon[0].y + 10) < 0.001f, "the outline doesn't start at the first node, in local space");
		Check(scene.IsOceanExcludedAt(XMFLOAT3(60, 0, 60)), "the polygon from the spline doesn't contain its middle");
		Check(scene.IsOceanExcludedAt(XMFLOAT3(51, 4, 51)), "the polygon from the spline doesn't contain a point near a node");
		Check(scene.IsOceanExcludedAt(XMFLOAT3(60, 0, 48.5f)), "the outline doesn't bulge out between the nodes like the spline");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(45, 0, 45)), "the polygon from the spline contains a point outside");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(60, 5.5f, 60)), "the polygon from the spline is not bounded by the height of the volume");
		spline.spline_node_entities.resize(2);
		Check(!scene.SetOceanExclusionPolygonFromSpline(from_spline, spline_entity), "a spline of two nodes gave a polygon");
	}

	void TestInvalidVolumesAndHierarchy()
	{
		Scene scene;
		const Entity parent = scene.Entity_CreateTransform("parent");
		const Entity group = CreateEntity(); // A parent without a TransformComponent.
		scene.hierarchy.Create(group).parentID = parent;
		const Entity child = scene.Entity_CreateOceanExclusion("child", XMFLOAT3(2, 0, 0), XMFLOAT3(-3, 2, 1));
		scene.hierarchy.Create(child).parentID = group;
		TransformComponent& parent_transform = *scene.transforms.GetComponent(parent);
		parent_transform.Translate(XMFLOAT3(25, -5, 12)); // Do not update world yet.

		wi::jobsystem::context ctx;
		scene.RunOceanExclusionUpdateSystem(ctx, true);
		wi::jobsystem::Wait(ctx);
		Check(scene.IsOceanExcludedAt(XMFLOAT3(27, -5, 12)), "simulation didn't use the edited parent across a transform-less ancestor");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(2, 0, 0)), "simulation used stale transforms");

		OceanExclusionComponent& exclusion = *scene.ocean_exclusions.GetComponent(child);
		Check(exclusion.IsActive(), "negative nonzero scale should be valid");
		Check(!exclusion.SetPolygon({ XMFLOAT2(0, 0), XMFLOAT2(1, 1) }), "two points were accepted as a polygon");
		Check(!exclusion.SetPolygon({ XMFLOAT2(0, 0), XMFLOAT2(1, 1), XMFLOAT2(INFINITY, 3) }), "non-finite vertex was accepted");
		Check(exclusion.shape == OceanExclusionComponent::Shape::Box, "invalid outline changed the old shape");

		TransformComponent& child_transform = *scene.transforms.GetComponent(child);
		child_transform.scale_local.y = 0;
		scene.RunOceanExclusionUpdateSystem(ctx, true);
		wi::jobsystem::Wait(ctx);
		wi::vector<uint8_t> data;
		Check(!exclusion.IsActive(), "a singular transform is active");
		Check(!scene.IsOceanExcludedAt(XMFLOAT3(27, -5, 12)), "a singular transform excludes water");
		Check(scene.WriteOceanExclusionShaderData(data) == 0 && data.empty(), "a singular transform was uploaded");

		child_transform.scale_local.y = 2;
		parent_transform.Translate(XMFLOAT3(15, 0, 0));
		scene.RunOceanExclusionUpdateSystem(ctx, true);
		wi::jobsystem::Wait(ctx);
		Check(scene.IsOceanExcludedAt(XMFLOAT3(42, -5, 12)), "valid scale did not restore the moved exclusion");
		scene.RunHierarchyUpdateSystem(ctx);
		wi::jobsystem::Wait(ctx);
		scene.RunOceanExclusionUpdateSystem(ctx);
		wi::jobsystem::Wait(ctx);
		Check(scene.IsOceanExcludedAt(XMFLOAT3(42, -5, 12)), "render and simulation snapshots disagree");
		Check(scene.WriteOceanExclusionShaderData(data) == 1, "valid volume was not uploaded after restoring scale");
	}

	void TestSerialization()
	{
		Scene scene;
		const Entity a = scene.Entity_CreateOceanExclusion("a", XMFLOAT3(1, 2, 3), XMFLOAT3(4, 5, 6), OceanExclusionComponent::Shape::Cylinder);
		const Entity b = scene.Entity_CreateOceanExclusion("b", XMFLOAT3(-7, 0, 9), XMFLOAT3(2, 2, 2), OceanExclusionComponent::Shape::Polygon);
		scene.ocean_exclusions.GetComponent(a)->SetDisabled(true);
		scene.ocean_exclusions.GetComponent(b)->polygon = LetterL();

		// One component:
		{
			wi::Archive archive;
			EntitySerializer seri;
			scene.ocean_exclusions.GetComponent(b)->Serialize(archive, seri);
			archive.SetReadModeAndResetPos(true);
			OceanExclusionComponent loaded;
			loaded.Serialize(archive, seri);
			Check(loaded.shape == OceanExclusionComponent::Shape::Polygon, "the shape was not serialized");
			Check(loaded.polygon.size() == 6 && loaded.polygon[2].x == 4 && loaded.polygon[2].y == 2, "the polygon was not serialized");
			Check(!loaded.IsDisabled(), "the flags were not serialized");
		}

		// Use the real scene archive, including entity ID remapping.
		{
			wi::Archive archive;
			scene.Serialize(archive);
			archive.SetReadModeAndResetPos(true);
			Scene loaded;
			loaded.Serialize(archive);
			Check(loaded.ocean_exclusions.GetCount() == 2, "the volumes were not serialized with the scene");
			const OceanExclusionComponent* loaded_a = loaded.ocean_exclusions.GetComponent(loaded.Entity_FindByName("a"));
			const OceanExclusionComponent* loaded_b = loaded.ocean_exclusions.GetComponent(loaded.Entity_FindByName("b"));
			Check(loaded_a != nullptr && loaded_b != nullptr, "the volumes are not on their entities after serialization");
			if (loaded_a != nullptr && loaded_b != nullptr)
			{
				Check(loaded_a->shape == OceanExclusionComponent::Shape::Cylinder && loaded_a->IsDisabled(), "the cylinder was not serialized with the scene");
				Check(loaded_b->shape == OceanExclusionComponent::Shape::Polygon && loaded_b->polygon.size() == 6, "the polygon was not serialized with the scene");
			}

			// After an update the loaded volume is where the saved one was:
			wi::jobsystem::context ctx;
			loaded.RunOceanExclusionUpdateSystem(ctx);
			wi::jobsystem::Wait(ctx);
			Check(loaded.IsOceanExcludedAt(XMFLOAT3(-5, 1, 11)), "the loaded polygon is not where it was saved");
			Check(!loaded.IsOceanExcludedAt(XMFLOAT3(1, 2, 3)), "the loaded cylinder is not disabled");
		}
	}

	void TestScripting()
	{
		wi::lua::Initialize();
		const char* script = R"(
			local scene = GetScene()
			local entity = CreateEntity()
			local transform = scene.Component_CreateTransform(entity)
			transform.Scale(Vector(10, 5, 10))
			transform.Translate(Vector(500, 0, 500))
			transform.UpdateTransform()
			local exclusion = scene.Component_CreateOceanExclusion(entity)
			ocean_exclusion_test_default_shape = exclusion.Shape
			exclusion.Shape = OceanExclusionShape.Cylinder
			exclusion.SetDisabled(true)
			ocean_exclusion_test_disabled = exclusion.IsDisabled()
			exclusion.SetDisabled(false)
			exclusion.SetPolygon({ Vector(-1, -1), Vector(1, -1), Vector(0, 1) })
			ocean_exclusion_test_polygon_count = #exclusion.GetPolygon()
			ocean_exclusion_test_polygon_z = exclusion.GetPolygon()[3].GetY()
			ocean_exclusion_test_shape = exclusion.Shape
			ocean_exclusion_test_count = #scene.Component_GetOceanExclusionArray()
			ocean_exclusion_test_entity = scene.Entity_GetOceanExclusionArray()[1] == entity
			ocean_exclusion_test_found = scene.Component_GetOceanExclusion(entity) ~= nil
			-- Growing the array must not stale an existing Lua component handle.
			local added = {}
			for i = 1, 25 do
				added[i] = CreateEntity()
				scene.Component_CreateOceanExclusion(added[i])
			end
			transform.Translate(Vector(20, 0, 0))
			exclusion.SetPolygonFromWorldPoints({ Vector(515, 0, 495), Vector(525, 0, 495), Vector(520, 0, 505) })
			ocean_exclusion_test_after_move = exclusion.Contains(Vector(520, 0, 500))
			ocean_exclusion_test_stale_transform = exclusion.Contains(Vector(500, 0, 500))
			for i = 1, #added do scene.Component_RemoveOceanExclusion(added[i]) end
			scene.Component_RemoveOceanExclusion(entity)
			ocean_exclusion_test_removed = scene.Component_GetOceanExclusion(entity) == nil
			ocean_exclusion_test_excluded = scene.IsOceanExcludedAt(Vector(500, 0, 500))
			ocean_exclusion_test_under_ocean = scene.IsUnderOcean(Vector(500, -100, 500))
			SetDebugOceanExclusionsEnabled(true)
		)";
		Check(wi::lua::RunText(script), "the script of the scripting test failed");
		lua_State* L = wi::lua::GetLuaState();
		const auto get_int = [&](const char* name) {
			lua_getglobal(L, name);
			const int value = (int)lua_tointeger(L, -1);
			lua_pop(L, 1);
			return value;
		};
		const auto get_bool = [&](const char* name) {
			lua_getglobal(L, name);
			const bool value = lua_toboolean(L, -1) != 0;
			lua_pop(L, 1);
			return value;
		};
		Check(get_int("ocean_exclusion_test_default_shape") == (int)OceanExclusionComponent::Shape::Box, "script: wrong default shape");
		Check(get_bool("ocean_exclusion_test_disabled"), "script: SetDisabled() didn't disable the volume");
		Check(get_int("ocean_exclusion_test_polygon_count") == 3, "script: SetPolygon() didn't set the outline");
		Check(get_int("ocean_exclusion_test_polygon_z") == 1, "script: GetPolygon() didn't give the outline back");
		Check(get_int("ocean_exclusion_test_shape") == (int)OceanExclusionComponent::Shape::Polygon, "script: SetPolygon() didn't set the polygon shape");
		Check(get_int("ocean_exclusion_test_count") >= 1, "script: Component_GetOceanExclusionArray() is empty");
		Check(get_bool("ocean_exclusion_test_entity"), "script: Entity_GetOceanExclusionArray() doesn't have the entity");
		Check(get_bool("ocean_exclusion_test_found"), "script: Component_GetOceanExclusion() didn't find the volume");
		Check(get_bool("ocean_exclusion_test_after_move"), "script: polygon uses a stale transform or component handle");
		Check(!get_bool("ocean_exclusion_test_stale_transform"), "script: moved polygon still covers original position");
		Check(get_bool("ocean_exclusion_test_removed"), "script: Component_RemoveOceanExclusion() didn't remove the volume");
		Check(!get_bool("ocean_exclusion_test_excluded"), "script: the removed volume still excludes the ocean");
		Check(!get_bool("ocean_exclusion_test_under_ocean"), "script: a point is under the ocean of a scene without ocean");
		Check(wi::renderer::GetToDrawDebugOceanExclusions(), "script: SetDebugOceanExclusionsEnabled() didn't turn the visualizer on");
		wi::renderer::SetToDrawDebugOceanExclusions(false);
	}
}

int main()
{
	wi::jobsystem::Initialize();

	TestShapes();
	TestTransform();
	TestScene();
	TestInvalidVolumesAndHierarchy();
	TestSerialization();
	TestScripting();
	wi::jobsystem::ShutDown();

	if (failures > 0)
	{
		std::cerr << failures << " checks of the ocean exclusion test failed\n";
		return 1;
	}
	std::cout << "Ocean exclusion test passed\n";
	return 0;
}
