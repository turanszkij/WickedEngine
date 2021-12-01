#include "wiOcean.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "shaders/ShaderInterop_Ocean.h"
#include "wiScene.h"
#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiTimer.h"
#include "wiVector.h"

#include <algorithm>

using namespace wi::graphics;
using namespace wi::scene;

namespace wi
{
	namespace ocean_internal
	{
		Shader		updateSpectrumCS;
		Shader		updateDisplacementMapCS;
		Shader		updateGradientFoldingCS;
		Shader		oceanSurfVS;
		Shader		wireframePS;
		Shader		oceanSurfPS;

		RasterizerState		rasterizerState;
		RasterizerState		wireRS;
		DepthStencilState	depthStencilState;
		BlendState			blendState;

		PipelineState PSO, PSO_wire;

		wi::fftgenerator::CSFFT512x512_Plan m_fft_plan;


		void LoadShaders()
		{
			wi::renderer::LoadShader(ShaderStage::CS, updateSpectrumCS, "oceanSimulatorCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, updateDisplacementMapCS, "oceanUpdateDisplacementMapCS.cso");
			wi::renderer::LoadShader(ShaderStage::CS, updateGradientFoldingCS, "oceanUpdateGradientFoldingCS.cso");

			wi::renderer::LoadShader(ShaderStage::VS, oceanSurfVS, "oceanSurfaceVS.cso");

			wi::renderer::LoadShader(ShaderStage::PS, oceanSurfPS, "oceanSurfacePS.cso");
			wi::renderer::LoadShader(ShaderStage::PS, wireframePS, "oceanSurfaceSimplePS.cso");


			GraphicsDevice* device = wi::graphics::GetDevice();

			{
				PipelineStateDesc desc;
				desc.vs = &oceanSurfVS;
				desc.ps = &oceanSurfPS;
				desc.bs = &blendState;
				desc.rs = &rasterizerState;
				desc.dss = &depthStencilState;
				device->CreatePipelineState(&desc, &PSO);

				desc.ps = &wireframePS;
				desc.rs = &wireRS;
				device->CreatePipelineState(&desc, &PSO_wire);
			}
		}
	}
	using namespace ocean_internal;


#define HALF_SQRT_2	0.7071068f
#define GRAV_ACCEL	981.0f	// The acceleration of gravity, cm/s^2

	// Generating gaussian random number with mean 0 and standard deviation 1.
	float Gauss()
	{
		float u1 = rand() / (float)RAND_MAX;
		float u2 = rand() / (float)RAND_MAX;
		if (u1 < 1e-6f)
			u1 = 1e-6f;
		return std::sqrt(-2 * logf(u1)) * cosf(2 * XM_PI * u2);
	}

	// Phillips Spectrum
	// K: normalized wave vector, W: wind direction, v: wind velocity, a: amplitude constant
	float Phillips(XMFLOAT2 K, XMFLOAT2 W, float v, float a, float dir_depend)
	{
		// largest possible wave from constant wind of velocity v
		float l = v * v / GRAV_ACCEL;
		// damp out waves with very small length w << l
		float w = l / 1000;

		float Ksqr = K.x * K.x + K.y * K.y;
		float Kcos = K.x * W.x + K.y * W.y;
		float phillips = a * expf(-1 / (l * l * Ksqr)) / (Ksqr * Ksqr * Ksqr) * (Kcos * Kcos);

		// filter out waves moving opposite to wind
		if (Kcos < 0)
			phillips *= dir_depend;

		// damp out waves with very small length w << l
		return phillips * expf(-Ksqr * w * w);
	}



	void Ocean::Create(const OceanParameters& params)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		// Height map H(0)
		int height_map_size = (params.dmap_dim + 4) * (params.dmap_dim + 1);
		wi::vector<XMFLOAT2> h0_data(height_map_size);
		wi::vector<float> omega_data(height_map_size);
		initHeightMap(params, h0_data.data(), omega_data.data());

		int hmap_dim = params.dmap_dim;
		int input_full_size = (hmap_dim + 4) * (hmap_dim + 1);
		// This value should be (hmap_dim / 2 + 1) * hmap_dim, but we use full sized buffer here for simplicity.
		int input_half_size = hmap_dim * hmap_dim;
		int output_size = hmap_dim * hmap_dim;

		// For filling the buffer with zeroes.
		wi::vector<char> zero_data(3 * output_size * sizeof(float) * 2);
		std::fill(zero_data.begin(), zero_data.end(), 0);

		GPUBufferDesc buf_desc;
		buf_desc.usage = Usage::DEFAULT;
		buf_desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
		buf_desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;

		// RW buffer allocations
		// H0
		buf_desc.stride = sizeof(float2);
		buf_desc.size = buf_desc.stride * input_full_size;
		device->CreateBuffer(&buf_desc, h0_data.data(), &buffer_Float2_H0);

		// Notice: The following 3 buffers should be half sized buffer because of conjugate symmetric input. But
		// we use full sized buffers due to the CS4.0 restriction.

		// Put H(t), Dx(t) and Dy(t) into one buffer because CS4.0 allows only 1 UAV at a time
		buf_desc.stride = sizeof(float2);
		buf_desc.size = buf_desc.stride * 3 * input_half_size;
		device->CreateBuffer(&buf_desc, zero_data.data(), &buffer_Float2_Ht);

		// omega
		buf_desc.stride = sizeof(float);
		buf_desc.size = buf_desc.stride * input_full_size;
		device->CreateBuffer(&buf_desc, omega_data.data(), &buffer_Float_Omega);

		// Notice: The following 3 should be real number data. But here we use the complex numbers and C2C FFT
		// due to the CS4.0 restriction.
		// Put Dz, Dx and Dy into one buffer because CS4.0 allows only 1 UAV at a time
		buf_desc.stride = sizeof(float2);
		buf_desc.size = buf_desc.stride * 3 * output_size;
		device->CreateBuffer(&buf_desc, zero_data.data(), &buffer_Float_Dxyz);

		TextureDesc tex_desc;
		tex_desc.width = hmap_dim;
		tex_desc.height = hmap_dim;
		tex_desc.array_size = 1;
		tex_desc.sample_count = 1;
		tex_desc.usage = Usage::DEFAULT;
		tex_desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;

		tex_desc.format = Format::R16G16B16A16_FLOAT;
		tex_desc.mip_levels = 0;
		tex_desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
		device->CreateTexture(&tex_desc, nullptr, &gradientMap);
		device->SetName(&gradientMap, "gradientMap");

		for (uint32_t i = 0; i < gradientMap.GetDesc().mip_levels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&gradientMap, SubresourceType::SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&gradientMap, SubresourceType::UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}

		tex_desc.format = Format::R32G32B32A32_FLOAT;
		tex_desc.mip_levels = 1;
		device->CreateTexture(&tex_desc, nullptr, &displacementMap);
		device->SetName(&displacementMap, "displacementMap");


		// Constant buffers
		uint32_t actual_dim = params.dmap_dim;
		uint32_t input_width = actual_dim + 4;
		// We use full sized data here. The value "output_width" should be actual_dim/2+1 though.
		uint32_t output_width = actual_dim;
		uint32_t output_height = actual_dim;
		uint32_t dtx_offset = actual_dim * actual_dim;
		uint32_t dty_offset = actual_dim * actual_dim * 2;
		Ocean_Simulation_ImmutableCB immutable_consts = { actual_dim, input_width, output_width, output_height, dtx_offset, dty_offset };

		GPUBufferDesc cb_desc;
		cb_desc.bind_flags = BindFlag::CONSTANT_BUFFER;
		cb_desc.size = sizeof(Ocean_Simulation_ImmutableCB);
		device->CreateBuffer(&cb_desc, &immutable_consts, &immutableCB);

		cb_desc.usage = Usage::DEFAULT;
		cb_desc.bind_flags = BindFlag::CONSTANT_BUFFER;
		cb_desc.size = sizeof(Ocean_Simulation_PerFrameCB);
		device->CreateBuffer(&cb_desc, nullptr, &perFrameCB);
	}



	// Initialize the vector field.
	// wlen_x: width of wave tile, in meters
	// wlen_y: length of wave tile, in meters
	void Ocean::initHeightMap(const OceanParameters& params, XMFLOAT2* out_h0, float* out_omega)
	{
		int i, j;
		XMFLOAT2 K;

		XMFLOAT2 wind_dir;
		XMStoreFloat2(&wind_dir, XMVector2Normalize(XMLoadFloat2(&params.wind_dir)));
		float a = params.wave_amplitude * 1e-7f;	// It is too small. We must scale it for editing.
		float v = params.wind_speed;
		float dir_depend = params.wind_dependency;

		int height_map_dim = params.dmap_dim;
		float patch_length = params.patch_length;


		for (i = 0; i <= height_map_dim; i++)
		{
			// K is wave-vector, range [-|DX/W, |DX/W], [-|DY/H, |DY/H]
			K.y = (-height_map_dim / 2.0f + i) * (2 * XM_PI / patch_length);

			for (j = 0; j <= height_map_dim; j++)
			{
				K.x = (-height_map_dim / 2.0f + j) * (2 * XM_PI / patch_length);

				float phil = (K.x == 0 && K.y == 0) ? 0 : std::sqrt(Phillips(K, wind_dir, v, a, dir_depend));

				out_h0[i * (height_map_dim + 4) + j].x = float(phil * Gauss() * HALF_SQRT_2);
				out_h0[i * (height_map_dim + 4) + j].y = float(phil * Gauss() * HALF_SQRT_2);

				// The angular frequency is following the dispersion relation:
				//            out_omega^2 = g*k
				// The equation of Gerstner wave:
				//            x = x0 - K/k * A * sin(dot(K, x0) - sqrt(g * k) * t), x is a 2D vector.
				//            z = A * cos(dot(K, x0) - sqrt(g * k) * t)
				// Gerstner wave shows that a point on a simple sinusoid wave is doing a uniform circular
				// motion with the center (x0, y0, z0), radius A, and the circular plane is parallel to
				// vector K.
				out_omega[i * (height_map_dim + 4) + j] = std::sqrt(GRAV_ACCEL * sqrtf(K.x * K.x + K.y * K.y));
			}
		}
	}

	void Ocean::UpdateDisplacementMap(const OceanParameters& params, CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		device->EventBegin("Ocean Simulation", cmd);

		// ---------------------------- H(0) -> H(t), D(x, t), D(y, t) --------------------------------

		device->BindComputeShader(&updateSpectrumCS, cmd);

		// Buffers
		const GPUResource* cs0_srvs[2] = {
			&buffer_Float2_H0,
			&buffer_Float_Omega
		};
		device->BindResources(cs0_srvs, 0, arraysize(cs0_srvs), cmd);

		const GPUResource* cs0_uavs[1] = { &buffer_Float2_Ht };
		device->BindUAVs(cs0_uavs, 0, arraysize(cs0_uavs), cmd);

		Ocean_Simulation_PerFrameCB perFrameData;
		perFrameData.g_TimeScale = params.time_scale;
		perFrameData.g_ChoppyScale = params.choppy_scale;
		perFrameData.g_GridLen = params.dmap_dim / params.patch_length;

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&perFrameCB, ResourceState::CONSTANT_BUFFER, ResourceState::COPY_DST),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
		device->UpdateBuffer(&perFrameCB, &perFrameData, cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&perFrameCB, ResourceState::COPY_DST, ResourceState::CONSTANT_BUFFER),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->BindConstantBuffer(&immutableCB, CB_GETBINDSLOT(Ocean_Simulation_ImmutableCB), cmd);
		device->BindConstantBuffer(&perFrameCB, CB_GETBINDSLOT(Ocean_Simulation_PerFrameCB), cmd);

		// Run the CS
		uint32_t group_count_x = (params.dmap_dim + OCEAN_COMPUTE_TILESIZE - 1) / OCEAN_COMPUTE_TILESIZE;
		uint32_t group_count_y = (params.dmap_dim + OCEAN_COMPUTE_TILESIZE - 1) / OCEAN_COMPUTE_TILESIZE;
		device->Dispatch(group_count_x, group_count_y, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}



		// ------------------------------------ Perform FFT -------------------------------------------
		fft_512x512_c2c(m_fft_plan, buffer_Float_Dxyz, buffer_Float_Dxyz, buffer_Float2_Ht, cmd);



		device->BindConstantBuffer(&immutableCB, CB_GETBINDSLOT(Ocean_Simulation_ImmutableCB), cmd);
		device->BindConstantBuffer(&perFrameCB, CB_GETBINDSLOT(Ocean_Simulation_PerFrameCB), cmd);


		// Update displacement map:
		device->BindComputeShader(&updateDisplacementMapCS, cmd);
		const GPUResource* cs_uavs[] = { &displacementMap };
		device->BindUAVs(cs_uavs, 0, 1, cmd);
		const GPUResource* cs_srvs[1] = { &buffer_Float_Dxyz };
		device->BindResources(cs_srvs, 0, 1, cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&displacementMap, displacementMap.desc.layout, ResourceState::UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
		device->Dispatch(params.dmap_dim / OCEAN_COMPUTE_TILESIZE, params.dmap_dim / OCEAN_COMPUTE_TILESIZE, 1, cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&displacementMap, ResourceState::UNORDERED_ACCESS, displacementMap.desc.layout),
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		// Update gradient map:
		device->BindComputeShader(&updateGradientFoldingCS, cmd);
		cs_uavs[0] = { &gradientMap };
		device->BindUAVs(cs_uavs, 0, 1, cmd);
		cs_srvs[0] = &displacementMap;
		device->BindResources(cs_srvs, 0, 1, cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&gradientMap, gradientMap.desc.layout, ResourceState::UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
		device->Dispatch(params.dmap_dim / OCEAN_COMPUTE_TILESIZE, params.dmap_dim / OCEAN_COMPUTE_TILESIZE, 1, cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&gradientMap, ResourceState::UNORDERED_ACCESS, gradientMap.desc.layout),
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		// Unbind


		wi::renderer::GenerateMipChain(gradientMap, wi::renderer::MIPGENFILTER_LINEAR, cmd);

		device->EventEnd(cmd);
	}


	void Ocean::Render(const CameraComponent& camera, const OceanParameters& params, CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		device->EventBegin("Ocean Rendering", cmd);

		bool wire = wi::renderer::IsWireRender();

		if (wire)
		{
			device->BindPipelineState(&PSO_wire, cmd);
		}
		else
		{
			device->BindPipelineState(&PSO, cmd);
		}


		const uint2 dim = uint2(160 * params.surfaceDetail, 90 * params.surfaceDetail);

		Ocean_RenderCB cb;
		cb.xOceanWaterColor = params.waterColor;
		cb.xOceanTexelLength = params.patch_length / params.dmap_dim;
		cb.xOceanScreenSpaceParams = XMFLOAT4((float)dim.x, (float)dim.y, 1.0f / (float)dim.x, 1.0f / (float)dim.y);
		cb.xOceanPatchSizeRecip = 1.0f / params.patch_length;
		cb.xOceanMapHalfTexel = 0.5f / params.dmap_dim;
		cb.xOceanWaterHeight = params.waterHeight;
		cb.xOceanSurfaceDisplacementTolerance = std::max(1.0f, params.surfaceDisplacementTolerance);

		device->BindDynamicConstantBuffer(cb, CB_GETBINDSLOT(Ocean_RenderCB), cmd);

		device->BindResource(&displacementMap, 0, cmd);
		device->BindResource(&gradientMap, 1, cmd);

		device->Draw(dim.x * dim.y * 6, 0, cmd);

		device->EventEnd(cmd);
	}


	void Ocean::Initialize()
	{
		wi::Timer timer;

		GraphicsDevice* device = wi::graphics::GetDevice();

		RasterizerState ras_desc;
		ras_desc.fill_mode = FillMode::SOLID;
		ras_desc.cull_mode = CullMode::NONE;
		ras_desc.front_counter_clockwise = false;
		ras_desc.depth_bias = 0;
		ras_desc.slope_scaled_depth_bias = 0.0f;
		ras_desc.depth_bias_clamp = 0.0f;
		ras_desc.depth_clip_enable = true;
		ras_desc.multisample_enable = true;
		ras_desc.antialiased_line_enable = false;

		rasterizerState = ras_desc;

		ras_desc.fill_mode = FillMode::WIREFRAME;
		wireRS = ras_desc;

		DepthStencilState depth_desc;
		depth_desc.depth_enable = true;
		depth_desc.depth_write_mask = DepthWriteMask::ALL;
		depth_desc.depth_func = ComparisonFunc::GREATER;
		depth_desc.stencil_enable = false;
		depthStencilState = depth_desc;

		BlendState blend_desc;
		blend_desc.alpha_to_coverage_enable = false;
		blend_desc.independent_blend_enable = false;
		blend_desc.render_target[0].blend_enable = true;
		blend_desc.render_target[0].src_blend = Blend::SRC_ALPHA;
		blend_desc.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		blend_desc.render_target[0].blend_op = BlendOp::ADD;
		blend_desc.render_target[0].src_blend_alpha = Blend::ONE;
		blend_desc.render_target[0].dest_blend_alpha = Blend::ZERO;
		blend_desc.render_target[0].blend_op_alpha = BlendOp::ADD;
		blend_desc.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		blendState = blend_desc;


		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); wi::fftgenerator::LoadShaders(); });

		LoadShaders();
		wi::fftgenerator::LoadShaders();
		fft512x512_create_plan(m_fft_plan, 3);

		wi::backlog::post("wi::Ocean Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	const Texture* Ocean::getDisplacementMap() const
	{
		return &displacementMap;
	}

	const Texture* Ocean::getGradientMap() const
	{
		return &gradientMap;
	}

}
