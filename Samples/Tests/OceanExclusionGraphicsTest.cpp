// Opt-in integration test: real Scene::Update uploads, GPU shaders and Jolt.
#include "WickedEngine.h"
#if defined(__APPLE__)
#include "wiGraphicsDevice_Metal.h"
#else
#include "wiGraphicsDevice_Vulkan.h"
#include "wiGraphicsDevice_DX12.h"
#endif
#include "shaders/ShaderInterop_Renderer.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace wi::ecs;
using namespace wi::graphics;
using namespace wi::scene;

namespace
{
	int failures = 0;

	bool Check(bool passed, const char* message)
	{
		if (!passed)
		{
			std::cerr << "Ocean exclusion integration test: " << message << '\n';
			++failures;
		}
		return passed;
	}

	struct TestRay
	{
		XMFLOAT4 start;
		XMFLOAT4 end;
	};

	void CheckGPU(GraphicsDevice& device, const Shader& shader, const Scene& scene, const wi::vector<TestRay>& rays, const float* fractions, const bool* rain_blocked)
	{
		GPUBuffer input, output, readback;
		GPUBufferDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
		desc.size = rays.size() * sizeof(TestRay);
		desc.stride = sizeof(TestRay);
		if (!Check(device.CreateBuffer(&desc, rays.data(), &input), "couldn't create ray buffer"))
		{
			return;
		}
		desc.bind_flags = BindFlag::UNORDERED_ACCESS;
		desc.size = rays.size() * sizeof(XMFLOAT4);
		desc.stride = sizeof(XMFLOAT4);
		if (!Check(device.CreateBuffer(&desc, nullptr, &output), "couldn't create output buffer"))
		{
			return;
		}
		desc = {};
		desc.usage = Usage::READBACK;
		desc.size = output.desc.size;
		if (!Check(device.CreateBuffer(&desc, nullptr, &readback), "couldn't create readback buffer"))
		{
			return;
		}

		const CommandList cmd = device.BeginCommandList();
		FrameCB frame = {};
		frame.scene = scene.shaderscene;
		device.BindDynamicConstantBuffer(frame, CBSLOT_RENDERER_FRAME, cmd);
		device.BindComputeShader(&shader, cmd);
		device.BindResource(&input, 0, cmd);
		device.BindUAV(&output, 0, cmd);
		device.Dispatch(uint32_t(rays.size()), 1, 1, cmd);
		device.Barrier(GPUBarrier::Buffer(&output, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SRC), cmd);
		device.CopyResource(&readback, &output, cmd);
		device.SubmitCommandLists();
		device.WaitForGPU();

		const XMFLOAT4* result = static_cast<const XMFLOAT4*>(readback.mapped_data);
		if (!Check(result != nullptr, "GPU readback wasn't mapped"))
		{
			return;
		}
		for (size_t i = 0; i < rays.size(); ++i)
		{
			const XMFLOAT4& value = result[i];
			const bool has_ocean = scene.ocean.IsValid();
			const auto excluded = [&](const XMFLOAT4& p) { return has_ocean && scene.IsOceanExcludedAt(XMFLOAT3(p.x, p.y, p.z)); };
			if (!Check((value.x != 0) == excluded(rays[i].start) && (value.y != 0) == excluded(rays[i].end), "GPU point membership differs from CPU") ||
				!Check(std::abs(value.z - fractions[i]) < 1e-4f, "wrong wet segment fraction") ||
				!Check((value.w != 0) == rain_blocked[i], "wrong ocean rain blocker"))
			{
				std::cerr << "Ray " << i << ": " << value.x << ", " << value.y << ", " << value.z << ", " << value.w
					<< " (expected fraction " << fractions[i] << ", rain blocker " << rain_blocked[i] << ")\n";
			}
		}
	}

	void TestVolumeUploads(GraphicsDevice& device, const Shader& shader)
	{
		Scene scene;
		const Entity weather_entity = CreateEntity();
		WeatherComponent& weather = scene.weathers.Create(weather_entity);
		weather.SetOceanEnabled(true);
		weather.oceanParameters.wave_amplitude = 0;

		const Entity parent = scene.Entity_CreateTransform("pocket carrier");
		const Entity box = scene.Entity_CreateOceanExclusion("box", XMFLOAT3(0, 0, 0), XMFLOAT3(2, 2, 2));
		scene.hierarchy.Create(box).parentID = parent;
		const Entity sphere = scene.Entity_CreateOceanExclusion("sphere", XMFLOAT3(2, 0, 0), XMFLOAT3(2, 2, 2), OceanExclusionComponent::Shape::Sphere);
		scene.Entity_CreateOceanExclusion("cylinder", XMFLOAT3(0, 0, 10), XMFLOAT3(2, 2, 1), OceanExclusionComponent::Shape::Cylinder);
		const Entity polygon = scene.Entity_CreateOceanExclusion("prism", XMFLOAT3(0, 0, 20), XMFLOAT3(1, 2, 1), OceanExclusionComponent::Shape::Polygon);
		// U-shaped prism: the horizontal ray at local z = 1 crosses two disjoint dry spans.
		scene.ocean_exclusions.GetComponent(polygon)->SetPolygon({ XMFLOAT2(-3, -2), XMFLOAT2(3, -2),
			XMFLOAT2(3, 2), XMFLOAT2(1, 2), XMFLOAT2(1, -0.5f), XMFLOAT2(-1, -0.5f), XMFLOAT2(-1, 2), XMFLOAT2(-3, 2) });
		scene.Entity_CreateOceanExclusion("submerged cabin", XMFLOAT3(0, -6, 30), XMFLOAT3(2, 2, 2));
		scene.Entity_CreateOceanExclusion("singular", XMFLOAT3(0, 0, 0), XMFLOAT3(100, 0, 100));

		const wi::vector<TestRay> rays = {
			{ XMFLOAT4(-4, 0, 0, 0), XMFLOAT4(6, 0, 0, 0) }, // overlapping box + sphere, union [-2, 4]
			{ XMFLOAT4(6, 0, 0, 0), XMFLOAT4(-4, 0, 0, 0) }, // same ray, reverse order
			{ XMFLOAT4(0, -1, 0, 0), XMFLOAT4(1, -1, 0, 0) }, // entirely dry and below the waterline
			{ XMFLOAT4(0, -4, 10, 0), XMFLOAT4(0, 4, 10, 0) }, // cylinder's vertical limits
			{ XMFLOAT4(-4, 0, 10, 0), XMFLOAT4(4, 0, 10, 0) }, // cylinder radius + nonuniform scale
			{ XMFLOAT4(-5, 0, 21, 0), XMFLOAT4(5, 0, 21, 0) }, // concave prism, [−3, −1] and [1, 3]
			{ XMFLOAT4(5, 0, 21, 0), XMFLOAT4(-5, 0, 21, 0) },
			{ XMFLOAT4(2, -1, 21, 0), XMFLOAT4(2, 3, 21, 0) }, // vertical ray, constant local XZ
			{ XMFLOAT4(0, -6, 30, 0), XMFLOAT4(0, -1, 30, 0) }, // dry cabin into open water, intact surface blocks rain
			{ XMFLOAT4(20, -1, 0, 0), XMFLOAT4(24, -1, 0, 0) }, // no intersections; then place the moving box here
			{ XMFLOAT4(2, 0, 20, 0), XMFLOAT4(2, 0, 20, 0) }, // zero-length segment in the prism
		};
		const float first[] = { 0.4f, 0.4f, 0, 0.5f, 0.5f, 0.6f, 0.6f, 0.25f, 0.6f, 1, 0 };
		const bool first_rain[] = { false, false, false, true, false, false, false, false, true, true, false };
		scene.Update(0);
		Check(scene.oceanExclusionCount == 5, "scene didn't upload exactly the valid volumes");
		Check(!scene.IsUnderOcean(XMFLOAT3(0, -6, 30)), "fully submerged dry cabin is treated as water");
		Check(scene.IsUnderOcean(XMFLOAT3(0, -1, 30)), "water above cabin was removed");
		CheckGPU(device, shader, scene, rays, first, first_rain);

		scene.transforms.GetComponent(parent)->Translate(XMFLOAT3(22, 0, 0));
		scene.ocean_exclusions.GetComponent(sphere)->SetDisabled(true);
		// A separate surface opening above the cabin doesn't clear the wet space between them.
		scene.Entity_CreateOceanExclusion("shallow opening", XMFLOAT3(0, 0, 30), XMFLOAT3(2, 0.25f, 2));
		scene.Update(0);
		Check(scene.oceanExclusionCount == 5, "disabled volume is still uploaded or the new one is missing");
		Check(scene.IsUnderOcean(XMFLOAT3(0, -1, 0)) && !scene.IsUnderOcean(XMFLOAT3(22, -1, 0)), "CPU query didn't follow the moving dry pocket");
		const float moved[] = { 1, 1, 1, 0.5f, 0.5f, 0.6f, 0.6f, 0.25f, 0.6f, 0, 0 };
		const bool moved_rain[] = { false, false, true, true, false, false, false, false, true, false, false };
		CheckGPU(device, shader, scene, rays, moved, moved_rain);

		for (size_t i = 0; i < scene.ocean_exclusions.GetCount(); ++i)
		{
			scene.ocean_exclusions[i].SetDisabled();
		}
		scene.Update(0);
		Check(scene.oceanExclusionCount == 0 && scene.ocean.IsValid(), "ocean doesn't stay active with all volumes disabled");
		const float disabled[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
		const bool intact_rain[] = { false, false, true, true, false, false, false, true, true, true, false };
		CheckGPU(device, shader, scene, rays, disabled, intact_rain);

		weather.SetOceanEnabled(false);
		scene.Update(0);
		Check(scene.shaderscene.weather.ocean.exclusion_count == 0 && scene.shaderscene.weather.ocean.exclusion_buffer < 0, "disabling ocean left a stale GPU binding");
		Check(!scene.IsUnderOcean(XMFLOAT3(0, -1, 30)), "disabled ocean still reports underwater");
		const bool disabled_rain[11] = {};
		CheckGPU(device, shader, scene, rays, disabled, disabled_rain);
	}

	void TestBuoyancyAndSwimming(GraphicsDevice& device)
	{
		Scene scene;
		// Also exercise the scene's fallback weather, without a weather entity.
		scene.weather.SetOceanEnabled(true);
		scene.weather.oceanParameters.wave_amplitude = 0;
		const Entity volume = scene.Entity_CreateOceanExclusion("dry", XMFLOAT3(9, 0, 0), XMFLOAT3(3, 8, 8));
		const auto add_body = [&](float x) {
			const Entity entity = scene.Entity_CreateTransform("floating test body");
			scene.transforms.GetComponent(entity)->Translate(XMFLOAT3(x, -2, 0));
			RigidBodyPhysicsComponent& body = scene.rigidbodies.Create(entity);
			body.shape = RigidBodyPhysicsComponent::BOX;
			body.mass = 1;
			body.buoyancy = 4;
			return entity;
		};
		const auto add_character = [&](float x) {
			const Entity entity = scene.Entity_CreateTransform("swimming test character");
			const Entity neck = scene.Entity_CreateTransform("neck");
			TransformComponent& neck_transform = *scene.transforms.GetComponent(neck);
			neck_transform.Translate(XMFLOAT3(x, -1, 4));
			neck_transform.UpdateTransform();
			HumanoidComponent& humanoid = scene.humanoids.Create(entity);
			humanoid.SetRagdollDisabled(); // This fixture only supplies a neck, not a full skeleton.
			humanoid.bones[size_t(HumanoidComponent::HumanoidBone::Neck)] = neck;
			CharacterComponent& character = scene.characters.Create(entity);
			character.SetActive(true);
			character.position = XMFLOAT3(x, -3, 4);
			return entity;
		};
		// An authored water mesh in a dry ocean pocket must keep its own physics.
		const Entity mesh_water = scene.Entity_CreatePlane("indoor pool");
		scene.materials.GetComponent(mesh_water)->shaderType = MaterialComponent::SHADERTYPE_WATER;
		TransformComponent& pool = *scene.transforms.GetComponent(mesh_water);
		pool.Scale(XMFLOAT3(3, 1, 6));
		pool.Translate(XMFLOAT3(20, -0.5f, 0));
		scene.Entity_CreateOceanExclusion("pool room", XMFLOAT3(20, -1, 0), XMFLOAT3(4, 4, 8));
		const Entity pool_body = add_body(20);
		const Entity pool_character = add_character(20);
		const Entity dry_body = add_body(0);
		const Entity wet_body = add_body(9);
		const Entity dry_character = add_character(0);
		const Entity wet_character = add_character(9);
		scene.Update(0);

		// Move the exclusion immediately before simulation. These checks should fail
		// if physics or swimming samples the last completed render frame's transforms.
		scene.transforms.GetComponent(volume)->Translate(XMFLOAT3(-9, 0, 0));
		constexpr float dt = 1.0f / 60.0f;
		for (int i = 0; i < 6; ++i)
		{
			scene.Update(dt);
			if (i == 0)
			{
				Check(!scene.characters.GetComponent(dry_character)->swimming, "character swims in a dry volume on its first frame");
				Check(scene.characters.GetComponent(wet_character)->swimming, "character doesn't swim where the volume moved away");
				Check(!scene.IsUnderOcean(XMFLOAT3(20, -1, 4)) && scene.characters.GetComponent(pool_character)->swimming,
					"excluding ocean disables swimming in an unrelated water mesh");
			}
			// Processing buffered frames also keeps the scene's GPU upload lifetimes realistic.
			device.BeginCommandList();
			device.SubmitCommandLists();
			device.WaitForGPU();
		}
		const float dry_velocity = wi::physics::GetVelocity(*scene.rigidbodies.GetComponent(dry_body)).y;
		const float wet_velocity = wi::physics::GetVelocity(*scene.rigidbodies.GetComponent(wet_body)).y;
		Check(dry_velocity < -0.5f && wet_velocity > 0.5f, "Jolt did not differentiate dry and wet rigid bodies");
		Check(wi::physics::GetVelocity(*scene.rigidbodies.GetComponent(pool_body)).y > 0.5f,
			"excluding ocean disabled buoyancy from an unrelated water mesh");

		scene.Entity_Remove(volume);
		scene.Update(dt);
		Check(scene.characters.GetComponent(dry_character)->swimming, "removing a volume didn't restore character swimming");
		device.BeginCommandList();
		device.SubmitCommandLists();
		device.WaitForGPU();
		scene.Clear();
		scene.Update(0);
		Check(!scene.ocean.IsValid() && scene.oceanExclusionCount == 0, "clearing the scene recreates its old ocean");
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: OceanExclusionGraphicsTest <vulkan|dx12|metal>\n";
		return 1;
	}
	const std::string backend = argv[1];
	wi::jobsystem::Initialize();
	wi::physics::Initialize();
	wi::renderer::SetOcclusionCullingEnabled(false);
	std::unique_ptr<GraphicsDevice> device;
#if defined(PLATFORM_APPLE)
	if (backend == "metal")
	{
		device = std::make_unique<GraphicsDevice_Metal>(ValidationMode::Disabled);
	}
#else
#ifdef WICKEDENGINE_BUILD_VULKAN
	if (backend == "vulkan")
	{
		device = std::make_unique<GraphicsDevice_Vulkan>(nullptr, ValidationMode::Disabled);
	}
#endif
#ifdef WICKEDENGINE_BUILD_DX12
	if (backend == "dx12")
	{
		device = std::make_unique<GraphicsDevice_DX12>(ValidationMode::Disabled);
	}
#endif
#endif
	if (!device)
	{
		std::cerr << "Unsupported graphics backend for this build: " << backend << '\n';
		wi::jobsystem::ShutDown();
		return 1;
	}

	{
		GetDevice() = device.get();
		wi::shadercompiler::CompilerInput input;
		input.format = device->GetShaderFormat();
		input.stage = ShaderStage::CS;
		input.shadersourcefilename = WICKED_OCEAN_TEST_SHADER;
		input.include_directories.push_back(WICKED_ENGINE_SHADER_DIR);
		wi::shadercompiler::CompilerOutput compiled;
		wi::shadercompiler::Compile(input, compiled);
		Shader shader;
		if (Check(compiled.IsValid(), "couldn't compile regression shader") &&
			Check(device->CreateShader(ShaderStage::CS, compiled.shaderdata, compiled.shadersize, &shader), "couldn't create regression shader"))
		{
			TestVolumeUploads(*device, shader);
			TestBuoyancyAndSwimming(*device);
		}
		else
		{
			std::cerr << compiled.error_message << '\n';
		}
		GetDevice() = nullptr;
	}
	device.reset();
	wi::jobsystem::ShutDown();
	if (failures == 0)
	{
		std::cout << "Ocean exclusion graphics and physics integration test passed (" << backend << ")\n";
	}
	return failures == 0 ? 0 : 1;
}
