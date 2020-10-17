#include "wiOcean.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "ShaderInterop_Ocean.h"
#include "wiScene.h"
#include "wiBackLog.h"
#include "wiEvent.h"

#include <algorithm>
#include <vector>

using namespace std;
using namespace wiGraphics;
using namespace wiScene;

namespace wiOcean_Internal
{
	Shader		updateSpectrumCS;
	Shader		updateDisplacementMapCS;
	Shader		updateGradientFoldingCS;
	Shader		oceanSurfVS;
	Shader		wireframePS;
	Shader		oceanSurfPS;

	GPUBuffer			shadingCB;
	RasterizerState		rasterizerState;
	RasterizerState		wireRS;
	DepthStencilState	depthStencilState;
	BlendState			blendState;

	PipelineState PSO, PSO_wire;

	wiFFTGenerator::CSFFT512x512_Plan m_fft_plan;


	void LoadShaders()
	{

		std::string path = wiRenderer::GetShaderPath();

		wiRenderer::LoadShader(CS, updateSpectrumCS, "oceanSimulatorCS.cso");
		wiRenderer::LoadShader(CS, updateDisplacementMapCS, "oceanUpdateDisplacementMapCS.cso");
		wiRenderer::LoadShader(CS, updateGradientFoldingCS, "oceanUpdateGradientFoldingCS.cso");

		wiRenderer::LoadShader(VS, oceanSurfVS, "oceanSurfaceVS.cso");

		wiRenderer::LoadShader(PS, oceanSurfPS, "oceanSurfacePS.cso");
		wiRenderer::LoadShader(PS, wireframePS, "oceanSurfaceSimplePS.cso");


		GraphicsDevice* device = wiRenderer::GetDevice();

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
using namespace wiOcean_Internal;


#define HALF_SQRT_2	0.7071068f
#define GRAV_ACCEL	981.0f	// The acceleration of gravity, cm/s^2

// Generating gaussian random number with mean 0 and standard deviation 1.
float Gauss()
{
	float u1 = rand() / (float)RAND_MAX;
	float u2 = rand() / (float)RAND_MAX;
	if (u1 < 1e-6f)
		u1 = 1e-6f;
	return sqrtf(-2 * logf(u1)) * cosf(2 * XM_PI * u2);
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



wiOcean::wiOcean(const WeatherComponent& weather)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	auto& params = weather.oceanParameters;

	// Height map H(0)
	int height_map_size = (params.dmap_dim + 4) * (params.dmap_dim + 1);
	std::vector<XMFLOAT2> h0_data(height_map_size);
	std::vector<float> omega_data(height_map_size);
	initHeightMap(weather, h0_data.data(), omega_data.data());

	int hmap_dim = params.dmap_dim;
	int input_full_size = (hmap_dim + 4) * (hmap_dim + 1);
	// This value should be (hmap_dim / 2 + 1) * hmap_dim, but we use full sized buffer here for simplicity.
	int input_half_size = hmap_dim * hmap_dim;
	int output_size = hmap_dim * hmap_dim;

	// For filling the buffer with zeroes.
	std::vector<char> zero_data(3 * output_size * sizeof(float) * 2);
	std::fill(zero_data.begin(), zero_data.end(), 0);

	GPUBufferDesc buf_desc;
	buf_desc.Usage = USAGE_DEFAULT;
	buf_desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	buf_desc.CPUAccessFlags = 0;
	buf_desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	SubresourceData init_data;

	// RW buffer allocations
	// H0
	buf_desc.StructureByteStride = sizeof(float2);
	buf_desc.ByteWidth = buf_desc.StructureByteStride * input_full_size;
	init_data.pSysMem = h0_data.data();
	device->CreateBuffer(&buf_desc, &init_data, &buffer_Float2_H0);

	// Notice: The following 3 buffers should be half sized buffer because of conjugate symmetric input. But
	// we use full sized buffers due to the CS4.0 restriction.

	// Put H(t), Dx(t) and Dy(t) into one buffer because CS4.0 allows only 1 UAV at a time
	buf_desc.StructureByteStride = sizeof(float2);
	buf_desc.ByteWidth = buf_desc.StructureByteStride * 3 * input_half_size;
	init_data.pSysMem = zero_data.data();
	device->CreateBuffer(&buf_desc, &init_data, &buffer_Float2_Ht);

	// omega
	buf_desc.StructureByteStride = sizeof(float);
	buf_desc.ByteWidth = buf_desc.StructureByteStride * input_full_size;
	init_data.pSysMem = omega_data.data();
	device->CreateBuffer(&buf_desc, &init_data, &buffer_Float_Omega);

	// Notice: The following 3 should be real number data. But here we use the complex numbers and C2C FFT
	// due to the CS4.0 restriction.
	// Put Dz, Dx and Dy into one buffer because CS4.0 allows only 1 UAV at a time
	buf_desc.StructureByteStride = sizeof(float2);
	buf_desc.ByteWidth = buf_desc.StructureByteStride * 3 * output_size;
	init_data.pSysMem = zero_data.data();
	device->CreateBuffer(&buf_desc, &init_data, &buffer_Float_Dxyz);

	TextureDesc tex_desc;
	tex_desc.Width = hmap_dim;
	tex_desc.Height = hmap_dim;
	tex_desc.ArraySize = 1;
	tex_desc.SampleCount = 1;
	tex_desc.Usage = USAGE_DEFAULT;
	tex_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS | BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;

	tex_desc.Format = FORMAT_R16G16B16A16_FLOAT;
	tex_desc.MipLevels = 0;
	device->CreateTexture(&tex_desc, nullptr, &gradientMap);

	for (uint32_t i = 0; i < gradientMap.GetDesc().MipLevels; ++i)
	{
		int subresource_index;
		subresource_index = device->CreateSubresource(&gradientMap, SRV, 0, 1, i, 1);
		assert(subresource_index == i);
		subresource_index = device->CreateSubresource(&gradientMap, UAV, 0, 1, i, 1);
		assert(subresource_index == i);
	}

	tex_desc.Format = FORMAT_R32G32B32A32_FLOAT;
	tex_desc.MipLevels = 1;
	device->CreateTexture(&tex_desc, nullptr, &displacementMap);


	// Constant buffers
	uint32_t actual_dim = params.dmap_dim;
	uint32_t input_width = actual_dim + 4;
	// We use full sized data here. The value "output_width" should be actual_dim/2+1 though.
	uint32_t output_width = actual_dim;
	uint32_t output_height = actual_dim;
	uint32_t dtx_offset = actual_dim * actual_dim;
	uint32_t dty_offset = actual_dim * actual_dim * 2;
	Ocean_Simulation_ImmutableCB immutable_consts = { actual_dim, input_width, output_width, output_height, dtx_offset, dty_offset };
	SubresourceData init_cb0;
	init_cb0.pSysMem = &immutable_consts;

	GPUBufferDesc cb_desc;
	cb_desc.Usage = USAGE_IMMUTABLE;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = 0;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = sizeof(Ocean_Simulation_ImmutableCB);
	device->CreateBuffer(&cb_desc, &init_cb0, &immutableCB);

	cb_desc.Usage = USAGE_DEFAULT;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = 0;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = sizeof(Ocean_Simulation_PerFrameCB);
	device->CreateBuffer(&cb_desc, nullptr, &perFrameCB);
}



// Initialize the vector field.
// wlen_x: width of wave tile, in meters
// wlen_y: length of wave tile, in meters
void wiOcean::initHeightMap(const WeatherComponent& weather, XMFLOAT2* out_h0, float* out_omega)
{
	auto& params = weather.oceanParameters;

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

			float phil = (K.x == 0 && K.y == 0) ? 0 : sqrtf(Phillips(K, wind_dir, v, a, dir_depend));

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
			out_omega[i * (height_map_dim + 4) + j] = sqrtf(GRAV_ACCEL * sqrtf(K.x * K.x + K.y * K.y));
		}
	}
}

void wiOcean::UpdateDisplacementMap(const WeatherComponent& weather, float time, CommandList cmd) const
{
	auto& params = weather.oceanParameters;

	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Ocean Simulation", cmd);

	// ---------------------------- H(0) -> H(t), D(x, t), D(y, t) --------------------------------

	device->BindComputeShader(&updateSpectrumCS, cmd);

	// Buffers
	const GPUResource* cs0_srvs[2] = { 
		&buffer_Float2_H0, 
		&buffer_Float_Omega
	};
	device->BindResources(CS, cs0_srvs, TEXSLOT_ONDEMAND0, arraysize(cs0_srvs), cmd);

	const GPUResource* cs0_uavs[1] = { &buffer_Float2_Ht };
	device->BindUAVs(CS, cs0_uavs, 0, arraysize(cs0_uavs), cmd);

	Ocean_Simulation_PerFrameCB perFrameData;
	perFrameData.g_Time = time * params.time_scale;
	perFrameData.g_ChoppyScale = params.choppy_scale;
	perFrameData.g_GridLen = params.dmap_dim / params.patch_length;
	device->UpdateBuffer(&perFrameCB, &perFrameData, cmd);

	device->BindConstantBuffer(CS, &immutableCB, CB_GETBINDSLOT(Ocean_Simulation_ImmutableCB), cmd);
	device->BindConstantBuffer(CS, &perFrameCB, CB_GETBINDSLOT(Ocean_Simulation_PerFrameCB), cmd);

	// Run the CS
	uint32_t group_count_x = (params.dmap_dim + OCEAN_COMPUTE_TILESIZE - 1) / OCEAN_COMPUTE_TILESIZE;
	uint32_t group_count_y = (params.dmap_dim + OCEAN_COMPUTE_TILESIZE - 1) / OCEAN_COMPUTE_TILESIZE;
	device->Dispatch(group_count_x, group_count_y, 1, cmd);
	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, 1, cmd);
	device->UnbindResources(TEXSLOT_ONDEMAND0, 2, cmd);


	// ------------------------------------ Perform FFT -------------------------------------------
	fft_512x512_c2c(m_fft_plan, buffer_Float_Dxyz, buffer_Float_Dxyz, buffer_Float2_Ht, cmd);



	device->BindConstantBuffer(CS, &immutableCB, CB_GETBINDSLOT(Ocean_Simulation_ImmutableCB), cmd);
	device->BindConstantBuffer(CS, &perFrameCB, CB_GETBINDSLOT(Ocean_Simulation_PerFrameCB), cmd);


	// Update displacement map:
	device->BindComputeShader(&updateDisplacementMapCS, cmd);
	const GPUResource* cs_uavs[] = { &displacementMap };
	device->BindUAVs(CS, cs_uavs, 0, 1, cmd);
	const GPUResource* cs_srvs[1] = { &buffer_Float_Dxyz };
	device->BindResources(CS, cs_srvs, TEXSLOT_ONDEMAND0, 1, cmd);
	device->Dispatch(params.dmap_dim / OCEAN_COMPUTE_TILESIZE, params.dmap_dim / OCEAN_COMPUTE_TILESIZE, 1, cmd);
	device->Barrier(barriers, arraysize(barriers), cmd);

	// Update gradient map:
	device->BindComputeShader(&updateGradientFoldingCS, cmd);
	cs_uavs[0] = { &gradientMap };
	device->BindUAVs(CS, cs_uavs, 0, 1, cmd);
	cs_srvs[0] = &displacementMap;
	device->BindResources(CS, cs_srvs, TEXSLOT_ONDEMAND0, 1, cmd);
	device->Dispatch(params.dmap_dim / OCEAN_COMPUTE_TILESIZE, params.dmap_dim / OCEAN_COMPUTE_TILESIZE, 1, cmd);
	device->Barrier(barriers, arraysize(barriers), cmd);

	// Unbind
	device->UnbindUAVs(0, 1, cmd);
	device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);


	wiRenderer::GenerateMipChain(gradientMap, wiRenderer::MIPGENFILTER_LINEAR, cmd);

	device->EventEnd(cmd);
}


void wiOcean::Render(const CameraComponent& camera, const WeatherComponent& weather, float time, CommandList cmd) const
{
	auto& params = weather.oceanParameters;

	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Ocean Rendering", cmd);

	bool wire = wiRenderer::IsWireRender();

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

	device->UpdateBuffer(&shadingCB, &cb, cmd);

	device->BindConstantBuffer(VS, &shadingCB, CB_GETBINDSLOT(Ocean_RenderCB), cmd);
	device->BindConstantBuffer(PS, &shadingCB, CB_GETBINDSLOT(Ocean_RenderCB), cmd);

	device->BindResource(VS, &displacementMap, TEXSLOT_ONDEMAND0, cmd);
	device->BindResource(PS, &gradientMap, TEXSLOT_ONDEMAND1, cmd);

	device->Draw(dim.x*dim.y*6, 0, cmd);

	device->EventEnd(cmd);
}


void wiOcean::Initialize()
{
	GraphicsDevice* device = wiRenderer::GetDevice();


	GPUBufferDesc cb_desc;
	cb_desc.Usage = USAGE_DYNAMIC;
	cb_desc.CPUAccessFlags = CPU_ACCESS_WRITE;
	cb_desc.ByteWidth = sizeof(Ocean_RenderCB);
	cb_desc.StructureByteStride = 0;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	device->CreateBuffer(&cb_desc, nullptr, &shadingCB);


	RasterizerStateDesc ras_desc;
	ras_desc.FillMode = FILL_SOLID;
	ras_desc.CullMode = CULL_NONE;
	ras_desc.FrontCounterClockwise = false;
	ras_desc.DepthBias = 0;
	ras_desc.SlopeScaledDepthBias = 0.0f;
	ras_desc.DepthBiasClamp = 0.0f;
	ras_desc.DepthClipEnable = true;
	ras_desc.MultisampleEnable = true;
	ras_desc.AntialiasedLineEnable = false;

	device->CreateRasterizerState(&ras_desc, &rasterizerState);

	ras_desc.FillMode = FILL_WIREFRAME;

	device->CreateRasterizerState(&ras_desc, &wireRS);

	DepthStencilStateDesc depth_desc;
	memset(&depth_desc, 0, sizeof(DepthStencilStateDesc));
	depth_desc.DepthEnable = true;
	depth_desc.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	depth_desc.DepthFunc = COMPARISON_GREATER;
	depth_desc.StencilEnable = false;
	device->CreateDepthStencilState(&depth_desc, &depthStencilState);

	BlendStateDesc blend_desc;
	memset(&blend_desc, 0, sizeof(BlendStateDesc));
	blend_desc.AlphaToCoverageEnable = false;
	blend_desc.IndependentBlendEnable = false;
	blend_desc.RenderTarget[0].BlendEnable = true;
	blend_desc.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&blend_desc, &blendState);


	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); wiFFTGenerator::LoadShaders(); });

	LoadShaders();
	wiFFTGenerator::LoadShaders();
	fft512x512_create_plan(m_fft_plan, 3);

	wiBackLog::post("wiOcean Initialized");
}

const Texture* wiOcean::getDisplacementMap() const
{
	return &displacementMap;
}

const Texture* wiOcean::getGradientMap() const
{
	return &gradientMap;
}
