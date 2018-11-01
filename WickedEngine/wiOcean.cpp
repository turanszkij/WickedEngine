#include "wiOcean.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "ShaderInterop_Ocean.h"
#include "wiSceneSystem.h"
#include "wiBackLog.h"

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiSceneSystem;

namespace wiOcean_Internal
{
	ComputeShader*		m_pUpdateSpectrumCS = nullptr;
	ComputeShader*		m_pUpdateDisplacementMapCS = nullptr;
	ComputeShader*		m_pUpdateGradientFoldingCS = nullptr;
	VertexShader*		g_pOceanSurfVS = nullptr;
	PixelShader*		g_pWireframePS = nullptr;
	PixelShader*		g_pOceanSurfPS = nullptr;

	GPUBuffer*			g_pShadingCB = nullptr;
	RasterizerState		rasterizerState;
	RasterizerState		wireRS;
	DepthStencilState	depthStencilState;
	BlendState			blendState;

	GraphicsPSO PSO, PSO_wire;
	ComputePSO CPSO_updateSpectrum;
	ComputePSO CPSO_updateDisplacementMap;
	ComputePSO CPSO_updateGradientFolding;

	CSFFT512x512_Plan m_fft_plan;
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

void createBufferAndUAV(void* data, UINT byte_width, UINT byte_stride, GPUBuffer** ppBuffer)
{
	*ppBuffer = new GPUBuffer;

	// Create buffer
	GPUBufferDesc buf_desc;
	buf_desc.ByteWidth = byte_width;
	buf_desc.Usage = USAGE_DEFAULT;
	buf_desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	buf_desc.CPUAccessFlags = 0;
	buf_desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	buf_desc.StructureByteStride = byte_stride;

	SubresourceData init_data;
	init_data.pSysMem = data;

	wiRenderer::GetDevice()->CreateBuffer(&buf_desc, data != NULL ? &init_data : NULL, *ppBuffer);

}

void createTextureAndViews(UINT width, UINT height, FORMAT format, Texture2D** ppTex)
{
	// Create 2D texture
	TextureDesc tex_desc;
	tex_desc.Width = width;
	tex_desc.Height = height;
	tex_desc.MipLevels = 0;
	tex_desc.ArraySize = 1;
	tex_desc.Format = format;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = USAGE_DEFAULT;
	tex_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS | BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	//tex_desc.MiscFlags = RESOURCE_MISC_GENERATE_MIPS;

	*ppTex = new Texture2D;
	(*ppTex)->RequestIndependentShaderResourcesForMIPs(true);
	(*ppTex)->RequestIndependentUnorderedAccessResourcesForMIPs(true);
	wiRenderer::GetDevice()->CreateTexture2D(&tex_desc, NULL, ppTex);

}



wiOcean::wiOcean(const WeatherComponent& weather)
{
	auto& params = weather.oceanParameters;

	// Height map H(0)
	int height_map_size = (params.dmap_dim + 4) * (params.dmap_dim + 1);
	XMFLOAT2* h0_data = new XMFLOAT2[height_map_size * sizeof(XMFLOAT2)];
	float* omega_data = new float[height_map_size * sizeof(float)];
	initHeightMap(weather, h0_data, omega_data);

	int hmap_dim = params.dmap_dim;
	int input_full_size = (hmap_dim + 4) * (hmap_dim + 1);
	// This value should be (hmap_dim / 2 + 1) * hmap_dim, but we use full sized buffer here for simplicity.
	int input_half_size = hmap_dim * hmap_dim;
	int output_size = hmap_dim * hmap_dim;

	// For filling the buffer with zeroes.
	char* zero_data = new char[3 * output_size * sizeof(float) * 2];
	memset(zero_data, 0, 3 * output_size * sizeof(float) * 2);

	// RW buffer allocations
	// H0
	UINT float2_stride = 2 * sizeof(float);
	createBufferAndUAV(h0_data, input_full_size * float2_stride, float2_stride, &m_pBuffer_Float2_H0);

	// Notice: The following 3 buffers should be half sized buffer because of conjugate symmetric input. But
	// we use full sized buffers due to the CS4.0 restriction.

	// Put H(t), Dx(t) and Dy(t) into one buffer because CS4.0 allows only 1 UAV at a time
	createBufferAndUAV(zero_data, 3 * input_half_size * float2_stride, float2_stride, &m_pBuffer_Float2_Ht);

	// omega
	createBufferAndUAV(omega_data, input_full_size * sizeof(float), sizeof(float), &m_pBuffer_Float_Omega);

	// Notice: The following 3 should be real number data. But here we use the complex numbers and C2C FFT
	// due to the CS4.0 restriction.
	// Put Dz, Dx and Dy into one buffer because CS4.0 allows only 1 UAV at a time
	createBufferAndUAV(zero_data, 3 * output_size * float2_stride, float2_stride, &m_pBuffer_Float_Dxyz);

	SAFE_DELETE_ARRAY(zero_data);
	SAFE_DELETE_ARRAY(h0_data);
	SAFE_DELETE_ARRAY(omega_data);


	createTextureAndViews(hmap_dim, hmap_dim, FORMAT_R32G32B32A32_FLOAT, &m_pDisplacementMap);
	createTextureAndViews(hmap_dim, hmap_dim, FORMAT_R16G16B16A16_FLOAT, &m_pGradientMap);


	// Constant buffers
	UINT actual_dim = params.dmap_dim;
	UINT input_width = actual_dim + 4;
	// We use full sized data here. The value "output_width" should be actual_dim/2+1 though.
	UINT output_width = actual_dim;
	UINT output_height = actual_dim;
	UINT dtx_offset = actual_dim * actual_dim;
	UINT dty_offset = actual_dim * actual_dim * 2;
	Ocean_Simulation_ImmutableCB immutable_consts = { actual_dim, input_width, output_width, output_height, dtx_offset, dty_offset };
	SubresourceData init_cb0;
	init_cb0.pSysMem = &immutable_consts;

	GPUBufferDesc cb_desc;
	cb_desc.Usage = USAGE_IMMUTABLE;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = 0;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = sizeof(Ocean_Simulation_ImmutableCB);
	m_pImmutableCB = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&cb_desc, &init_cb0, m_pImmutableCB);

	cb_desc.Usage = USAGE_DYNAMIC;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = CPU_ACCESS_WRITE;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = sizeof(Ocean_Simulation_PerFrameCB);
	m_pPerFrameCB = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&cb_desc, nullptr, m_pPerFrameCB);
}

wiOcean::~wiOcean()
{

	SAFE_DELETE(m_pBuffer_Float2_H0);
	SAFE_DELETE(m_pBuffer_Float_Omega);
	SAFE_DELETE(m_pBuffer_Float2_Ht);
	SAFE_DELETE(m_pBuffer_Float_Dxyz);

	SAFE_DELETE(m_pDisplacementMap);
	SAFE_DELETE(m_pGradientMap);


	SAFE_DELETE(m_pImmutableCB);
	SAFE_DELETE(m_pPerFrameCB);
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

void wiOcean::UpdateDisplacementMap(const WeatherComponent& weather, float time, GRAPHICSTHREAD threadID)
{
	auto& params = weather.oceanParameters;

	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Ocean Simulation", threadID);

	// ---------------------------- H(0) -> H(t), D(x, t), D(y, t) --------------------------------

	device->BindComputePSO(&CPSO_updateSpectrum, threadID);

	// Buffers
	GPUResource* cs0_srvs[2] = { 
		m_pBuffer_Float2_H0, 
		m_pBuffer_Float_Omega
	};
	device->BindResources(CS, cs0_srvs, TEXSLOT_ONDEMAND0, ARRAYSIZE(cs0_srvs), threadID);

	GPUResource* cs0_uavs[1] = { m_pBuffer_Float2_Ht };
	device->BindUAVs(CS, cs0_uavs, 0, ARRAYSIZE(cs0_uavs), threadID);

	Ocean_Simulation_PerFrameCB perFrameData;
	perFrameData.g_Time = time * params.time_scale;
	perFrameData.g_ChoppyScale = params.choppy_scale;
	perFrameData.g_GridLen = params.dmap_dim / params.patch_length;
	device->UpdateBuffer(m_pPerFrameCB, &perFrameData, threadID);

	device->BindConstantBuffer(CS, m_pImmutableCB, CB_GETBINDSLOT(Ocean_Simulation_ImmutableCB), threadID);
	device->BindConstantBuffer(CS, m_pPerFrameCB, CB_GETBINDSLOT(Ocean_Simulation_PerFrameCB), threadID);

	// Run the CS
	UINT group_count_x = (params.dmap_dim + OCEAN_COMPUTE_TILESIZE - 1) / OCEAN_COMPUTE_TILESIZE;
	UINT group_count_y = (params.dmap_dim + OCEAN_COMPUTE_TILESIZE - 1) / OCEAN_COMPUTE_TILESIZE;
	device->Dispatch(group_count_x, group_count_y, 1, threadID);
	device->UAVBarrier(cs0_uavs, ARRAYSIZE(cs0_uavs), threadID);

	device->UnbindUAVs(0, 1, threadID);
	device->UnbindResources(TEXSLOT_ONDEMAND0, 2, threadID);


	// ------------------------------------ Perform FFT -------------------------------------------
	fft_512x512_c2c(&m_fft_plan, m_pBuffer_Float_Dxyz, m_pBuffer_Float_Dxyz, m_pBuffer_Float2_Ht, threadID);



	device->BindConstantBuffer(CS, m_pImmutableCB, CB_GETBINDSLOT(Ocean_Simulation_ImmutableCB), threadID);
	device->BindConstantBuffer(CS, m_pPerFrameCB, CB_GETBINDSLOT(Ocean_Simulation_PerFrameCB), threadID);


	// Update displacement map:
	device->BindComputePSO(&CPSO_updateDisplacementMap, threadID);
	GPUResource* cs_uavs[] = { m_pDisplacementMap };
	device->BindUAVs(CS, cs_uavs, 0, 1, threadID);
	GPUResource* cs_srvs[1] = { m_pBuffer_Float_Dxyz };
	device->BindResources(CS, cs_srvs, TEXSLOT_ONDEMAND0, 1, threadID);
	device->Dispatch(params.dmap_dim / OCEAN_COMPUTE_TILESIZE, params.dmap_dim / OCEAN_COMPUTE_TILESIZE, 1, threadID);
	device->UAVBarrier(cs_uavs, ARRAYSIZE(cs_uavs), threadID);

	// Update gradient map:
	device->BindComputePSO(&CPSO_updateGradientFolding, threadID);
	cs_uavs[0] = { m_pGradientMap };
	device->BindUAVs(CS, cs_uavs, 0, 1, threadID);
	cs_srvs[0] = m_pDisplacementMap;
	device->BindResources(CS, cs_srvs, TEXSLOT_ONDEMAND0, 1, threadID);
	device->Dispatch(params.dmap_dim / OCEAN_COMPUTE_TILESIZE, params.dmap_dim / OCEAN_COMPUTE_TILESIZE, 1, threadID);
	device->UAVBarrier(cs_uavs, ARRAYSIZE(cs_uavs), threadID);

	// Unbind
	device->UnbindUAVs(0, 1, threadID);
	device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);


	//device->GenerateMips(m_pGradientMap, threadID);
	wiRenderer::GenerateMipChain(m_pGradientMap, wiRenderer::MIPGENFILTER_LINEAR, threadID);

	device->EventEnd(threadID);
}


void wiOcean::Render(const CameraComponent& camera, const WeatherComponent& weather, float time, GRAPHICSTHREAD threadID)
{
	auto& params = weather.oceanParameters;

	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Ocean Rendering", threadID);

	bool wire = wiRenderer::IsWireRender();

	if (wire)
	{
		device->BindGraphicsPSO(&PSO_wire, threadID);
	}
	else
	{
		device->BindGraphicsPSO(&PSO, threadID);
	}


	const uint2 dim = uint2(160 * params.surfaceDetail, 90 * params.surfaceDetail);

	Ocean_RenderCB cb;
	cb.xOceanWaterColor = params.waterColor;
	cb.xOceanTexelLength = params.patch_length / params.dmap_dim;
	cb.xOceanScreenSpaceParams = XMFLOAT4((float)dim.x, (float)dim.y, 1.0f / (float)dim.x, 1.0f / (float)dim.y);
	cb.xOceanPatchSizeRecip = 1.0f / params.patch_length;
	cb.xOceanMapHalfTexel = 0.5f / params.dmap_dim;
	cb.xOceanWaterHeight = params.waterHeight;
	cb.xOceanSurfaceDisplacementTolerance = max(1, params.surfaceDisplacementTolerance);

	device->UpdateBuffer(g_pShadingCB, &cb, threadID);

	device->BindConstantBuffer(VS, g_pShadingCB, CB_GETBINDSLOT(Ocean_RenderCB), threadID);
	device->BindConstantBuffer(PS, g_pShadingCB, CB_GETBINDSLOT(Ocean_RenderCB), threadID);

	device->BindResource(VS, m_pDisplacementMap, TEXSLOT_ONDEMAND0, threadID);
	device->BindResource(PS, m_pGradientMap, TEXSLOT_ONDEMAND0, threadID);

	device->Draw(dim.x*dim.y*6, 0, threadID);

	device->EventEnd(threadID);
}


void wiOcean::LoadShaders()
{

	std::string path = wiRenderer::GetShaderPath();

	m_pUpdateSpectrumCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "oceanSimulatorCS.cso", wiResourceManager::COMPUTESHADER));
	m_pUpdateDisplacementMapCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "oceanUpdateDisplacementMapCS.cso", wiResourceManager::COMPUTESHADER));
	m_pUpdateGradientFoldingCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "oceanUpdateGradientFoldingCS.cso", wiResourceManager::COMPUTESHADER));


	g_pOceanSurfVS = static_cast<VertexShader*>(wiResourceManager::GetShaderManager().add(path + "oceanSurfaceVS.cso", wiResourceManager::VERTEXSHADER));

	g_pOceanSurfPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager().add(path + "oceanSurfacePS.cso", wiResourceManager::PIXELSHADER));
	g_pWireframePS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager().add(path + "oceanSurfaceSimplePS.cso", wiResourceManager::PIXELSHADER));


	GraphicsDevice* device = wiRenderer::GetDevice();

	{
		GraphicsPSODesc desc;
		desc.vs = g_pOceanSurfVS;
		desc.ps = g_pOceanSurfPS;
		desc.bs = &blendState;
		desc.rs = &rasterizerState;
		desc.dss = &depthStencilState;
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
		desc.DSFormat = wiRenderer::DSFormat_full;
		device->CreateGraphicsPSO(&desc, &PSO);

		desc.ps = g_pWireframePS;
		desc.rs = &wireRS;
		device->CreateGraphicsPSO(&desc, &PSO_wire);
	}

	{
		ComputePSODesc desc;
		desc.cs = m_pUpdateSpectrumCS;
		device->CreateComputePSO(&desc, &CPSO_updateSpectrum);
		desc.cs = m_pUpdateDisplacementMapCS;
		device->CreateComputePSO(&desc, &CPSO_updateDisplacementMap);
		desc.cs = m_pUpdateGradientFoldingCS;
		device->CreateComputePSO(&desc, &CPSO_updateGradientFolding);
	}
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
	g_pShadingCB = new GPUBuffer;
	device->CreateBuffer(&cb_desc, nullptr, g_pShadingCB);


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


	LoadShaders();
	CSFFT_512x512_Data_t::LoadShaders();
	fft512x512_create_plan(&m_fft_plan, 3);

	wiBackLog::post("wiOcean Initialized");
}

void wiOcean::CleanUp()
{
	fft512x512_destroy_plan(&m_fft_plan);

	SAFE_DELETE(m_pUpdateSpectrumCS);
	SAFE_DELETE(m_pUpdateDisplacementMapCS);
	SAFE_DELETE(m_pUpdateGradientFoldingCS);

	SAFE_DELETE(g_pOceanSurfVS);
	SAFE_DELETE(g_pOceanSurfPS);
	SAFE_DELETE(g_pWireframePS);

	SAFE_DELETE(g_pShadingCB);
}

Texture2D* wiOcean::getDisplacementMap()
{
	return m_pDisplacementMap;
}

Texture2D* wiOcean::getGradientMap()
{
	return m_pGradientMap;
}
