#include "wiOcean.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "ShaderInterop_Ocean.h"

using namespace wiGraphicsTypes;
using namespace std;

ComputeShader*			wiOcean::m_pUpdateSpectrumCS = nullptr;
VertexShader*			wiOcean::m_pQuadVS = nullptr;
PixelShader*			wiOcean::m_pUpdateDisplacementPS = nullptr;
PixelShader*			wiOcean::m_pGenGradientFoldingPS = nullptr;
VertexShader*			wiOcean::g_pOceanSurfVS = nullptr;
PixelShader*			wiOcean::g_pWireframePS = nullptr;
PixelShader*			wiOcean::g_pOceanSurfPS = nullptr;
VertexLayout*			wiOcean::m_pQuadLayout = nullptr;
VertexLayout*			wiOcean::g_pMeshLayout = nullptr;

// Disable warning "conditional expression is constant"
#pragma warning(disable:4127)


#define HALF_SQRT_2	0.7071068f
#define GRAV_ACCEL	981.0f	// The acceleration of gravity, cm/s^2

#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16

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


	//assert(*ppBuffer);

	//// Create undordered access view
	//UNORDERED_ACCESS_VIEW_DESC uav_desc;
	//uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	//uav_desc.ViewDimension = UAV_DIMENSION_BUFFER;
	//uav_desc.Buffer.FirstElement = 0;
	//uav_desc.Buffer.NumElements = byte_width / byte_stride;
	//uav_desc.Buffer.Flags = 0;

	//device->CreateUnorderedAccessView(*ppBuffer, &uav_desc, ppUAV);
	//assert(*ppUAV);

	//// Create shader resource view
	//SHADER_RESOURCE_VIEW_DESC srv_desc;
	//srv_desc.Format = DXGI_FORMAT_UNKNOWN;
	//srv_desc.ViewDimension = SRV_DIMENSION_BUFFER;
	//srv_desc.Buffer.FirstElement = 0;
	//srv_desc.Buffer.NumElements = byte_width / byte_stride;

	//device->CreateShaderResourceView(*ppBuffer, &srv_desc, ppSRV);
	//assert(*ppSRV);
}

void createTextureAndViews(UINT width, UINT height, FORMAT format, Texture2D** ppTex)
{
	// Create 2D texture
	Texture2DDesc tex_desc;
	tex_desc.Width = width;
	tex_desc.Height = height;
	tex_desc.MipLevels = 0;
	tex_desc.ArraySize = 1;
	tex_desc.Format = format;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = USAGE_DEFAULT;
	tex_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = RESOURCE_MISC_GENERATE_MIPS;

	*ppTex = new Texture2D;
	wiRenderer::GetDevice()->CreateTexture2D(&tex_desc, NULL, ppTex);


	//assert(*ppTex);

	//// Create shader resource view
	//(*ppTex)->GetDesc(&tex_desc);
	//if (ppSRV)
	//{
	//	SHADER_RESOURCE_VIEW_DESC srv_desc;
	//	srv_desc.Format = format;
	//	srv_desc.ViewDimension = SRV_DIMENSION_TEXTURE2D;
	//	srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	//	srv_desc.Texture2D.MostDetailedMip = 0;

	//	device->CreateShaderResourceView(*ppTex, &srv_desc, ppSRV);
	//	assert(*ppSRV);
	//}

	//// Create render target view
	//if (ppRTV)
	//{
	//	RENDER_TARGET_VIEW_DESC rtv_desc;
	//	rtv_desc.Format = format;
	//	rtv_desc.ViewDimension = RTV_DIMENSION_TEXTURE2D;
	//	rtv_desc.Texture2D.MipSlice = 0;

	//	device->CreateRenderTargetView(*ppTex, &rtv_desc, ppRTV);
	//	assert(*ppRTV);
	//}
}



wiOcean::wiOcean(const wiOceanParameter& params)
{
	// Height map H(0)
	int height_map_size = (params.dmap_dim + 4) * (params.dmap_dim + 1);
	XMFLOAT2* h0_data = new XMFLOAT2[height_map_size * sizeof(XMFLOAT2)];
	float* omega_data = new float[height_map_size * sizeof(float)];
	initHeightMap(h0_data, omega_data);

	m_param = params;
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

	// D3D11 Textures
	createTextureAndViews(hmap_dim, hmap_dim, FORMAT_R32G32B32A32_FLOAT, &m_pDisplacementMap);
	createTextureAndViews(hmap_dim, hmap_dim, FORMAT_R16G16B16A16_FLOAT, &m_pGradientMap);

	// Samplers
	SamplerDesc sam_desc;
	sam_desc.Filter = FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sam_desc.AddressU = TEXTURE_ADDRESS_WRAP;
	sam_desc.AddressV = TEXTURE_ADDRESS_WRAP;
	sam_desc.AddressW = TEXTURE_ADDRESS_WRAP;
	sam_desc.MipLODBias = 0;
	sam_desc.MaxAnisotropy = 1;
	sam_desc.ComparisonFunc = COMPARISON_NEVER;
	sam_desc.BorderColor[0] = 1.0f;
	sam_desc.BorderColor[1] = 1.0f;
	sam_desc.BorderColor[2] = 1.0f;
	sam_desc.BorderColor[3] = 1.0f;
	sam_desc.MinLOD = -FLT_MAX;
	sam_desc.MaxLOD = FLT_MAX;
	wiRenderer::GetDevice()->CreateSamplerState(&sam_desc, &m_pPointSamplerState);


	// Quad vertex buffer
	GPUBufferDesc vb_desc;
	vb_desc.ByteWidth = 4 * sizeof(XMFLOAT4);
	vb_desc.Usage = USAGE_IMMUTABLE;
	vb_desc.BindFlags = BIND_VERTEX_BUFFER;
	vb_desc.CPUAccessFlags = 0;
	vb_desc.MiscFlags = 0;

	float quad_verts[] =
	{
		-1, -1, 0, 1,
		-1,  1, 0, 1,
		1, -1, 0, 1,
		1,  1, 0, 1,
	};
	SubresourceData init_data;
	init_data.pSysMem = &quad_verts[0];
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	m_pQuadVB = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&vb_desc, &init_data, m_pQuadVB);

	// Constant buffers
	UINT actual_dim = m_param.dmap_dim;
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

	// FFT
	fft512x512_create_plan(&m_fft_plan, 3);

	initRenderResource();
}

wiOcean::~wiOcean()
{
	fft512x512_destroy_plan(&m_fft_plan);

	SAFE_DELETE(m_pBuffer_Float2_H0);
	SAFE_DELETE(m_pBuffer_Float_Omega);
	SAFE_DELETE(m_pBuffer_Float2_Ht);
	SAFE_DELETE(m_pBuffer_Float_Dxyz);

	SAFE_DELETE(m_pQuadVB);

	SAFE_DELETE(m_pDisplacementMap);
	SAFE_DELETE(m_pGradientMap);


	SAFE_DELETE(m_pImmutableCB);
	SAFE_DELETE(m_pPerFrameCB);


	cleanupRenderResource();
}




// Simulation functions:



// Initialize the vector field.
// wlen_x: width of wave tile, in meters
// wlen_y: length of wave tile, in meters
void wiOcean::initHeightMap(XMFLOAT2* out_h0, float* out_omega)
{
	int i, j;
	XMFLOAT2 K;

	XMFLOAT2 wind_dir;
	XMStoreFloat2(&wind_dir, XMVector2Normalize(XMLoadFloat2(&m_param.wind_dir)));
	float a = m_param.wave_amplitude * 1e-7f;	// It is too small. We must scale it for editing.
	float v = m_param.wind_speed;
	float dir_depend = m_param.wind_dependency;

	int height_map_dim = m_param.dmap_dim;
	float patch_length = m_param.patch_length;

	// initialize random generator.
	srand(0);

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

void wiOcean::UpdateDisplacementMap(float time, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("OceanSimulator", threadID);

	// ---------------------------- H(0) -> H(t), D(x, t), D(y, t) --------------------------------
	// Compute shader
	device->BindCS(m_pUpdateSpectrumCS, threadID);

	// Buffers
	GPUResource* cs0_srvs[2] = { 
		m_pBuffer_Float2_H0, 
		m_pBuffer_Float_Omega
	};
	device->BindResourcesCS(cs0_srvs, TEXSLOT_ONDEMAND0, 2, threadID);

	GPUUnorderedResource* cs0_uavs[1] = { m_pBuffer_Float2_Ht };
	device->BindUnorderedAccessResourcesCS(cs0_uavs, 0, 1, threadID);

	Ocean_Simulation_PerFrameCB perFrameData;
	perFrameData.g_Time = time * m_param.time_scale;
	perFrameData.g_ChoppyScale = m_param.choppy_scale;
	perFrameData.g_GridLen = m_param.dmap_dim / m_param.patch_length;
	device->UpdateBuffer(m_pPerFrameCB, &perFrameData, threadID);

	device->BindConstantBufferCS(m_pImmutableCB, CB_GETBINDSLOT(Ocean_Simulation_ImmutableCB), threadID);
	device->BindConstantBufferCS(m_pPerFrameCB, CB_GETBINDSLOT(Ocean_Simulation_PerFrameCB), threadID);

	// Run the CS
	UINT group_count_x = (m_param.dmap_dim + BLOCK_SIZE_X - 1) / BLOCK_SIZE_X;
	UINT group_count_y = (m_param.dmap_dim + BLOCK_SIZE_Y - 1) / BLOCK_SIZE_Y;
	device->Dispatch(group_count_x, group_count_y, 1, threadID);

	//// Unbind resources for CS
	//cs0_uavs[0] = NULL;
	//m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, cs0_uavs, (UINT*)(&cs0_uavs[0]));
	//cs0_srvs[0] = NULL;
	//cs0_srvs[1] = NULL;
	//m_pd3dImmediateContext->CSSetShaderResources(0, 2, cs0_srvs);

	device->UnBindUnorderedAccessResources(0, 1, threadID);
	device->UnBindResources(TEXSLOT_ONDEMAND0, 2, threadID);


	// ------------------------------------ Perform FFT -------------------------------------------
	fft_512x512_c2c(&m_fft_plan, m_pBuffer_Float_Dxyz, m_pBuffer_Float_Dxyz, m_pBuffer_Float2_Ht, threadID);

	// --------------------------------- Wrap Dx, Dy and Dz ---------------------------------------
	// Push RT
	//ID3D11RenderTargetView* old_target;
	//ID3D11DepthStencilView* old_depth;
	//m_pd3dImmediateContext->OMGetRenderTargets(1, &old_target, &old_depth);
	//VIEWPORT old_viewport;
	//UINT num_viewport = 1;
	//m_pd3dImmediateContext->RSGetViewports(&num_viewport, &old_viewport);

	ViewPort new_vp;
	new_vp.TopLeftX = 0;
	new_vp.TopLeftX = 0;
	new_vp.Width = (float)m_param.dmap_dim;
	new_vp.Height = (float)m_param.dmap_dim;
	new_vp.MinDepth = 0.0f;
	new_vp.MaxDepth = 1.0f;
	device->BindViewports(1, &new_vp, threadID);

	// Set RT
	device->BindRenderTargets(1, (Texture**)&m_pDisplacementMap, nullptr, threadID);

	// VS & PS
	device->BindVS(m_pQuadVS, threadID);
	device->BindPS(m_pUpdateDisplacementPS, threadID);

	device->BindConstantBufferPS(m_pImmutableCB, CB_GETBINDSLOT(Ocean_Simulation_ImmutableCB), threadID);
	device->BindConstantBufferPS(m_pPerFrameCB, CB_GETBINDSLOT(Ocean_Simulation_PerFrameCB), threadID);

	// Buffer resources
	GPUResource* ps_srvs[1] = { m_pBuffer_Float_Dxyz };
	device->BindResourcesPS(ps_srvs, TEXSLOT_ONDEMAND0, 1, threadID);

	// IA setup
	GPUBuffer* vbs[1] = { m_pQuadVB };
	UINT strides[1] = { sizeof(XMFLOAT4) };
	UINT offsets[1] = { 0 };
	device->BindVertexBuffers(&vbs[0], 0, 1, &strides[0], &offsets[0], threadID);

	device->BindVertexLayout(m_pQuadLayout, threadID);
	device->BindPrimitiveTopology(TRIANGLESTRIP, threadID);

	// Perform draw call
	device->Draw(4, 0, threadID);

	// Unbind
	device->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);


	// ----------------------------------- Generate Normal ----------------------------------------
	// Set RT
	device->BindRenderTargets(1, (Texture**)&m_pGradientMap, nullptr, threadID);

	// VS & PS
	device->BindVS(m_pQuadVS, threadID);
	device->BindPS(m_pGenGradientFoldingPS, threadID);

	// Texture resource and sampler
	ps_srvs[0] = m_pDisplacementMap;
	device->BindResourcesPS(ps_srvs, TEXSLOT_ONDEMAND0, 1, threadID);

	device->BindSamplerPS(&m_pPointSamplerState, SSLOT_ONDEMAND0, threadID);

	// Perform draw call
	device->Draw(4, 0, threadID);

	// Unbind
	device->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);

	//// Pop RT
	//m_pd3dImmediateContext->RSSetViewports(1, &old_viewport);
	//m_pd3dImmediateContext->OMSetRenderTargets(1, &old_target, old_depth);
	//SAFE_DELETE(old_target);
	//SAFE_DELETE(old_depth);

	device->GenerateMips(m_pGradientMap, threadID);


	device->EventEnd(threadID);
}

Texture2D* wiOcean::getDisplacementMap()
{
	return m_pDisplacementMap;
}

Texture2D* wiOcean::getGradientMap()
{
	return m_pGradientMap;
}


const wiOceanParameter& wiOcean::getParameters()
{
	return m_param;
}






// Rendering functions:


void wiOcean::initRenderResource()
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiOceanParameter ocean_param = m_param;

	g_PatchLength = ocean_param.patch_length;
	g_DisplaceMapDim = ocean_param.dmap_dim;
	g_WindDir = ocean_param.wind_dir;

	// D3D buffers
	createSurfaceMesh();
	createFresnelMap();
	loadTextures();


	// Constants
	GPUBufferDesc cb_desc;
	cb_desc.Usage = USAGE_DYNAMIC;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = CPU_ACCESS_WRITE;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = sizeof(Ocean_Rendering_PatchCB);
	cb_desc.StructureByteStride = 0;
	g_pPerCallCB = new GPUBuffer;
	device->CreateBuffer(&cb_desc, nullptr, g_pPerCallCB);

	Ocean_Rendering_ShadingCB shading_data;
	// Grid side length * 2
	shading_data.g_TexelLength_x2 = g_PatchLength / g_DisplaceMapDim * 2;;
	// Color
	shading_data.g_SkyColor = g_SkyColor;
	shading_data.g_WaterbodyColor = g_WaterbodyColor;
	// Texcoord
	shading_data.g_UVScale = 1.0f / g_PatchLength;
	shading_data.g_UVOffset = 0.5f / g_DisplaceMapDim;
	// Perlin
	shading_data.g_PerlinSize = g_PerlinSize;
	shading_data.g_PerlinAmplitude = g_PerlinAmplitude;
	shading_data.g_PerlinGradient = g_PerlinGradient;
	shading_data.g_PerlinOctave = g_PerlinOctave;
	// Multiple reflection workaround
	shading_data.g_BendParam = g_BendParam;
	// Sun streaks
	shading_data.g_SunColor = g_SunColor;
	shading_data.g_SunDir = g_SunDir;
	shading_data.g_Shineness = g_Shineness;

	SubresourceData cb_init_data;
	cb_init_data.pSysMem = &shading_data;
	cb_init_data.SysMemPitch = 0;
	cb_init_data.SysMemSlicePitch = 0;

	cb_desc.Usage = USAGE_IMMUTABLE;
	cb_desc.CPUAccessFlags = 0;
	cb_desc.ByteWidth = sizeof(Ocean_Rendering_ShadingCB);
	cb_desc.StructureByteStride = 0;
	g_pShadingCB = new GPUBuffer;
	device->CreateBuffer(&cb_desc, &cb_init_data, g_pShadingCB);

	// State blocks
	RasterizerStateDesc ras_desc;
	ras_desc.FillMode = FILL_SOLID;
	ras_desc.CullMode = CULL_NONE;
	ras_desc.FrontCounterClockwise = false;
	ras_desc.DepthBias = 0;
	ras_desc.SlopeScaledDepthBias = 0.0f;
	ras_desc.DepthBiasClamp = 0.0f;
	ras_desc.DepthClipEnable = true;
	ras_desc.ScissorEnable = false;
	ras_desc.MultisampleEnable = true;
	ras_desc.AntialiasedLineEnable = false;

	g_pRSState_Solid = new RasterizerState;
	device->CreateRasterizerState(&ras_desc, g_pRSState_Solid);

	ras_desc.FillMode = FILL_WIREFRAME;

	g_pRSState_Wireframe = new RasterizerState;
	device->CreateRasterizerState(&ras_desc, g_pRSState_Wireframe);

	DepthStencilStateDesc depth_desc;
	memset(&depth_desc, 0, sizeof(DepthStencilStateDesc));
	depth_desc.DepthEnable = true;
	depth_desc.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	depth_desc.DepthFunc = COMPARISON_GREATER;
	depth_desc.StencilEnable = false;
	g_pDSState_Disable = new DepthStencilState;
	device->CreateDepthStencilState(&depth_desc, g_pDSState_Disable);

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
	g_pBState_Transparent = new BlendState;
	device->CreateBlendState(&blend_desc, g_pBState_Transparent);
}

void wiOcean::cleanupRenderResource()
{
	SAFE_DELETE(g_pMeshIB);
	SAFE_DELETE(g_pMeshVB);

	SAFE_DELETE(g_pFresnelMap);
	SAFE_DELETE(g_pPerlinMap);

	SAFE_DELETE(g_pPerCallCB);
	SAFE_DELETE(g_pPerFrameCB);
	SAFE_DELETE(g_pShadingCB);

	SAFE_DELETE(g_pRSState_Solid);
	SAFE_DELETE(g_pRSState_Wireframe);
	SAFE_DELETE(g_pDSState_Disable);
	SAFE_DELETE(g_pBState_Transparent);

	g_render_list.clear();
}

#define MESH_INDEX_2D(x, y)	(((y) + vert_rect.bottom) * (g_MeshDim + 1) + (x) + vert_rect.left)

// Generate boundary mesh for a patch. Return the number of generated indices
int wiOcean::generateBoundaryMesh(int left_degree, int right_degree, int bottom_degree, int top_degree,
	RECT vert_rect, DWORD* output)
{
	// Triangle list for bottom boundary
	int i, j;
	int counter = 0;
	int width = vert_rect.right - vert_rect.left;

	if (bottom_degree > 0)
	{
		int b_step = width / bottom_degree;

		for (i = 0; i < width; i += b_step)
		{
			output[counter++] = MESH_INDEX_2D(i, 0);
			output[counter++] = MESH_INDEX_2D(i + b_step / 2, 1);
			output[counter++] = MESH_INDEX_2D(i + b_step, 0);

			for (j = 0; j < b_step / 2; j++)
			{
				if (i == 0 && j == 0 && left_degree > 0)
					continue;

				output[counter++] = MESH_INDEX_2D(i, 0);
				output[counter++] = MESH_INDEX_2D(i + j, 1);
				output[counter++] = MESH_INDEX_2D(i + j + 1, 1);
			}

			for (j = b_step / 2; j < b_step; j++)
			{
				if (i == width - b_step && j == b_step - 1 && right_degree > 0)
					continue;

				output[counter++] = MESH_INDEX_2D(i + b_step, 0);
				output[counter++] = MESH_INDEX_2D(i + j, 1);
				output[counter++] = MESH_INDEX_2D(i + j + 1, 1);
			}
		}
	}

	// Right boundary
	int height = vert_rect.top - vert_rect.bottom;

	if (right_degree > 0)
	{
		int r_step = height / right_degree;

		for (i = 0; i < height; i += r_step)
		{
			output[counter++] = MESH_INDEX_2D(width, i);
			output[counter++] = MESH_INDEX_2D(width - 1, i + r_step / 2);
			output[counter++] = MESH_INDEX_2D(width, i + r_step);

			for (j = 0; j < r_step / 2; j++)
			{
				if (i == 0 && j == 0 && bottom_degree > 0)
					continue;

				output[counter++] = MESH_INDEX_2D(width, i);
				output[counter++] = MESH_INDEX_2D(width - 1, i + j);
				output[counter++] = MESH_INDEX_2D(width - 1, i + j + 1);
			}

			for (j = r_step / 2; j < r_step; j++)
			{
				if (i == height - r_step && j == r_step - 1 && top_degree > 0)
					continue;

				output[counter++] = MESH_INDEX_2D(width, i + r_step);
				output[counter++] = MESH_INDEX_2D(width - 1, i + j);
				output[counter++] = MESH_INDEX_2D(width - 1, i + j + 1);
			}
		}
	}

	// Top boundary
	if (top_degree > 0)
	{
		int t_step = width / top_degree;

		for (i = 0; i < width; i += t_step)
		{
			output[counter++] = MESH_INDEX_2D(i, height);
			output[counter++] = MESH_INDEX_2D(i + t_step / 2, height - 1);
			output[counter++] = MESH_INDEX_2D(i + t_step, height);

			for (j = 0; j < t_step / 2; j++)
			{
				if (i == 0 && j == 0 && left_degree > 0)
					continue;

				output[counter++] = MESH_INDEX_2D(i, height);
				output[counter++] = MESH_INDEX_2D(i + j, height - 1);
				output[counter++] = MESH_INDEX_2D(i + j + 1, height - 1);
			}

			for (j = t_step / 2; j < t_step; j++)
			{
				if (i == width - t_step && j == t_step - 1 && right_degree > 0)
					continue;

				output[counter++] = MESH_INDEX_2D(i + t_step, height);
				output[counter++] = MESH_INDEX_2D(i + j, height - 1);
				output[counter++] = MESH_INDEX_2D(i + j + 1, height - 1);
			}
		}
	}

	// Left boundary
	if (left_degree > 0)
	{
		int l_step = height / left_degree;

		for (i = 0; i < height; i += l_step)
		{
			output[counter++] = MESH_INDEX_2D(0, i);
			output[counter++] = MESH_INDEX_2D(1, i + l_step / 2);
			output[counter++] = MESH_INDEX_2D(0, i + l_step);

			for (j = 0; j < l_step / 2; j++)
			{
				if (i == 0 && j == 0 && bottom_degree > 0)
					continue;

				output[counter++] = MESH_INDEX_2D(0, i);
				output[counter++] = MESH_INDEX_2D(1, i + j);
				output[counter++] = MESH_INDEX_2D(1, i + j + 1);
			}

			for (j = l_step / 2; j < l_step; j++)
			{
				if (i == height - l_step && j == l_step - 1 && top_degree > 0)
					continue;

				output[counter++] = MESH_INDEX_2D(0, i + l_step);
				output[counter++] = MESH_INDEX_2D(1, i + j);
				output[counter++] = MESH_INDEX_2D(1, i + j + 1);
			}
		}
	}

	return counter;
}

// Generate boundary mesh for a patch. Return the number of generated indices
int wiOcean::generateInnerMesh(RECT vert_rect, DWORD* output)
{
	int i, j;
	int counter = 0;
	int width = vert_rect.right - vert_rect.left;
	int height = vert_rect.top - vert_rect.bottom;

	bool reverse = false;
	for (i = 0; i < height; i++)
	{
		if (reverse == false)
		{
			output[counter++] = MESH_INDEX_2D(0, i);
			output[counter++] = MESH_INDEX_2D(0, i + 1);
			for (j = 0; j < width; j++)
			{
				output[counter++] = MESH_INDEX_2D(j + 1, i);
				output[counter++] = MESH_INDEX_2D(j + 1, i + 1);
			}
		}
		else
		{
			output[counter++] = MESH_INDEX_2D(width, i);
			output[counter++] = MESH_INDEX_2D(width, i + 1);
			for (j = width - 1; j >= 0; j--)
			{
				output[counter++] = MESH_INDEX_2D(j, i);
				output[counter++] = MESH_INDEX_2D(j, i + 1);
			}
		}

		reverse = !reverse;
	}

	return counter;
}

void wiOcean::createSurfaceMesh()
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	// --------------------------------- Vertex Buffer -------------------------------
	int num_verts = (g_MeshDim + 1) * (g_MeshDim + 1);
	ocean_vertex* pV = new ocean_vertex[num_verts];
	assert(pV);

	int i, j;
	for (i = 0; i <= g_MeshDim; i++)
	{
		for (j = 0; j <= g_MeshDim; j++)
		{
			pV[i * (g_MeshDim + 1) + j].index_x = (float)j;
			pV[i * (g_MeshDim + 1) + j].index_y = (float)i;
		}
	}

	GPUBufferDesc vb_desc;
	vb_desc.ByteWidth = num_verts * sizeof(ocean_vertex);
	vb_desc.Usage = USAGE_IMMUTABLE;
	vb_desc.BindFlags = BIND_VERTEX_BUFFER;
	vb_desc.CPUAccessFlags = 0;
	vb_desc.MiscFlags = 0;
	vb_desc.StructureByteStride = sizeof(ocean_vertex);

	SubresourceData init_data;
	init_data.pSysMem = pV;
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	g_pMeshVB = new GPUBuffer;
	device->CreateBuffer(&vb_desc, &init_data, g_pMeshVB);

	SAFE_DELETE_ARRAY(pV);


	// --------------------------------- Index Buffer -------------------------------
	// The index numbers for all mesh LODs (up to 256x256)
	const int index_size_lookup[] = { 0, 0, 4284, 18828, 69444, 254412, 956916, 3689820, 14464836 };

	memset(&g_mesh_patterns[0][0][0][0][0], 0, sizeof(g_mesh_patterns));

	g_Lods = 0;
	for (i = g_MeshDim; i > 1; i >>= 1)
		g_Lods++;

	// Generate patch meshes. Each patch contains two parts: the inner mesh which is a regular
	// grids in a triangle strip. The boundary mesh is constructed w.r.t. the edge degrees to
	// meet water-tight requirement.
	DWORD* index_array = new DWORD[index_size_lookup[g_Lods]];
	assert(index_array);

	int offset = 0;
	int level_size = g_MeshDim;

	// Enumerate patterns
	for (int level = 0; level <= g_Lods - 2; level++)
	{
		int left_degree = level_size;

		for (int left_type = 0; left_type < 3; left_type++)
		{
			int right_degree = level_size;

			for (int right_type = 0; right_type < 3; right_type++)
			{
				int bottom_degree = level_size;

				for (int bottom_type = 0; bottom_type < 3; bottom_type++)
				{
					int top_degree = level_size;

					for (int top_type = 0; top_type < 3; top_type++)
					{
						QuadRenderParam* pattern = &g_mesh_patterns[level][left_type][right_type][bottom_type][top_type];

						// Inner mesh (triangle strip)
						RECT inner_rect;
						inner_rect.left = (left_degree == level_size) ? 0 : 1;
						inner_rect.right = (right_degree == level_size) ? level_size : level_size - 1;
						inner_rect.bottom = (bottom_degree == level_size) ? 0 : 1;
						inner_rect.top = (top_degree == level_size) ? level_size : level_size - 1;

						int num_new_indices = generateInnerMesh(inner_rect, index_array + offset);

						pattern->inner_start_index = offset;
						pattern->num_inner_verts = (level_size + 1) * (level_size + 1);
						pattern->num_inner_faces = num_new_indices - 2;
						offset += num_new_indices;

						// Boundary mesh (triangle list)
						int l_degree = (left_degree == level_size) ? 0 : left_degree;
						int r_degree = (right_degree == level_size) ? 0 : right_degree;
						int b_degree = (bottom_degree == level_size) ? 0 : bottom_degree;
						int t_degree = (top_degree == level_size) ? 0 : top_degree;

						RECT outer_rect = { 0, level_size, level_size, 0 };
						num_new_indices = generateBoundaryMesh(l_degree, r_degree, b_degree, t_degree, outer_rect, index_array + offset);

						pattern->boundary_start_index = offset;
						pattern->num_boundary_verts = (level_size + 1) * (level_size + 1);
						pattern->num_boundary_faces = num_new_indices / 3;
						offset += num_new_indices;

						top_degree /= 2;
					}
					bottom_degree /= 2;
				}
				right_degree /= 2;
			}
			left_degree /= 2;
		}
		level_size /= 2;
	}

	assert(offset == index_size_lookup[g_Lods]);

	GPUBufferDesc ib_desc;
	ib_desc.ByteWidth = index_size_lookup[g_Lods] * sizeof(DWORD);
	ib_desc.Usage = USAGE_IMMUTABLE;
	ib_desc.BindFlags = BIND_INDEX_BUFFER;
	ib_desc.CPUAccessFlags = 0;
	ib_desc.MiscFlags = 0;
	ib_desc.StructureByteStride = sizeof(DWORD);

	init_data.pSysMem = index_array;

	g_pMeshIB = new GPUBuffer;
	device->CreateBuffer(&ib_desc, &init_data, g_pMeshIB);

	SAFE_DELETE_ARRAY(index_array);
}

void wiOcean::createFresnelMap()
{
	static const int FRESNEL_TEX_SIZE = 256;
	static const float g_SkyBlending = 16.0f;

	uint32_t* buffer = new uint32_t[FRESNEL_TEX_SIZE];
	for (int i = 0; i < FRESNEL_TEX_SIZE; i++)
	{
		float cos_a = i / (FLOAT)FRESNEL_TEX_SIZE;
		// Using water's refraction index 1.33
		uint32_t fresnel = (uint32_t)(XMVectorGetX(XMFresnelTerm(XMVectorSet(cos_a, cos_a, cos_a, cos_a), XMVectorSet(1.33f, 1.33f, 1.33f, 1.33f))) * 255);

		uint32_t sky_blend = (uint32_t)(powf(1 / (1 + cos_a), g_SkyBlending) * 255);

		buffer[i] = (sky_blend << 8) | fresnel;
	}



	Texture1DDesc tex_desc;
	tex_desc.Width = FRESNEL_TEX_SIZE;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = FORMAT_R8G8B8A8_UNORM;
	tex_desc.Usage = USAGE_IMMUTABLE;
	tex_desc.BindFlags = BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;

	SubresourceData init_data;
	init_data.pSysMem = buffer;
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	HRESULT hr = wiRenderer::GetDevice()->CreateTexture1D(&tex_desc, &init_data, &g_pFresnelMap);
	assert(SUCCEEDED(hr));

	delete[] buffer;
}

void wiOcean::loadTextures()
{
	wiRenderer::GetDevice()->CreateTextureFromFile("perlin_noise.dds", &g_pPerlinMap, true, GRAPHICSTHREAD_IMMEDIATE);
}

bool wiOcean::checkNodeVisibility(const QuadNode& quad_node, const Camera& camera)
{
	// Plane equation setup

	XMMATRIX matProj = camera.GetRealProjection();

	// Left plane
	float fov_x = atan(1.0f / XMVectorGetX(matProj.r[0]));
	XMVECTOR plane_left = XMVectorSet(cos(fov_x), 0, sin(fov_x), 0);
	// Right plane
	XMVECTOR plane_right = XMVectorSet(-cos(fov_x), 0, sin(fov_x), 0);

	// Bottom plane
	float fov_y = atan(1.0f / XMVectorGetY(matProj.r[1]));
	XMVECTOR plane_bottom = XMVectorSet(0, cos(fov_y), sin(fov_y), 0);
	// Top plane
	XMVECTOR plane_top = XMVectorSet(0, -cos(fov_y), sin(fov_y), 0);

	// Test quad corners against view frustum in view space
	XMVECTOR corner_verts[4];
	corner_verts[0] = XMVectorSet(quad_node.bottom_left.x, quad_node.bottom_left.y, 0, 1);
	corner_verts[1] = corner_verts[0] + XMVectorSet(quad_node.length, 0, 0, 0);
	corner_verts[2] = corner_verts[0] + XMVectorSet(quad_node.length, quad_node.length, 0, 0);
	corner_verts[3] = corner_verts[0] + XMVectorSet(0, quad_node.length, 0, 0);

	XMMATRIX matView = XMMATRIX(1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1) * camera.GetView();
	corner_verts[0] = XMVector4Transform(corner_verts[0], matView);
	corner_verts[1] = XMVector4Transform(corner_verts[1], matView);
	corner_verts[2] = XMVector4Transform(corner_verts[2], matView);
	corner_verts[3] = XMVector4Transform(corner_verts[3], matView);

	// Test against eye plane
	if (XMVectorGetZ(corner_verts[0]) < 0 && XMVectorGetZ(corner_verts[1]) < 0 && XMVectorGetZ(corner_verts[2]) < 0 && XMVectorGetZ(corner_verts[3]) < 0)
		return false;

	// Test against left plane
	float dist_0 = XMVectorGetX(XMVector4Dot(corner_verts[0], plane_left));
	float dist_1 = XMVectorGetX(XMVector4Dot(corner_verts[1], plane_left));
	float dist_2 = XMVectorGetX(XMVector4Dot(corner_verts[2], plane_left));
	float dist_3 = XMVectorGetX(XMVector4Dot(corner_verts[3], plane_left));
	if (dist_0 < 0 && dist_1 < 0 && dist_2 < 0 && dist_3 < 0)
		return false;

	// Test against right plane
	dist_0 = XMVectorGetX(XMVector4Dot(corner_verts[0], plane_right));
	dist_1 = XMVectorGetX(XMVector4Dot(corner_verts[1], plane_right));
	dist_2 = XMVectorGetX(XMVector4Dot(corner_verts[2], plane_right));
	dist_3 = XMVectorGetX(XMVector4Dot(corner_verts[3], plane_right));
	if (dist_0 < 0 && dist_1 < 0 && dist_2 < 0 && dist_3 < 0)
		return false;

	// Test against bottom plane
	dist_0 = XMVectorGetX(XMVector4Dot(corner_verts[0], plane_bottom));
	dist_1 = XMVectorGetX(XMVector4Dot(corner_verts[1], plane_bottom));
	dist_2 = XMVectorGetX(XMVector4Dot(corner_verts[2], plane_bottom));
	dist_3 = XMVectorGetX(XMVector4Dot(corner_verts[3], plane_bottom));
	if (dist_0 < 0 && dist_1 < 0 && dist_2 < 0 && dist_3 < 0)
		return false;

	// Test against top plane
	dist_0 = XMVectorGetX(XMVector4Dot(corner_verts[0], plane_top));
	dist_1 = XMVectorGetX(XMVector4Dot(corner_verts[1], plane_top));
	dist_2 = XMVectorGetX(XMVector4Dot(corner_verts[2], plane_top));
	dist_3 = XMVectorGetX(XMVector4Dot(corner_verts[3], plane_top));
	if (dist_0 < 0 && dist_1 < 0 && dist_2 < 0 && dist_3 < 0)
		return false;

	return true;
}

float wiOcean::estimateGridCoverage(const QuadNode& quad_node, const Camera& camera, float screen_area)
{
	// Estimate projected area

	// Test 16 points on the quad and find out the biggest one.
	const static float sample_pos[16][2] =
	{
		{ 0, 0 },
		{ 0, 1 },
		{ 1, 0 },
		{ 1, 1 },
		{ 0.5f, 0.333f },
		{ 0.25f, 0.667f },
		{ 0.75f, 0.111f },
		{ 0.125f, 0.444f },
		{ 0.625f, 0.778f },
		{ 0.375f, 0.222f },
		{ 0.875f, 0.556f },
		{ 0.0625f, 0.889f },
		{ 0.5625f, 0.037f },
		{ 0.3125f, 0.37f },
		{ 0.8125f, 0.704f },
		{ 0.1875f, 0.148f },
	};

	XMMATRIX matProj = camera.GetRealProjection();
	XMFLOAT3 eye = camera.translation;
	XMVECTOR eye_point = XMVectorSet(eye.x, eye.z, eye.y, 0);
	float grid_len_world = quad_node.length / g_MeshDim;

	float max_area_proj = 0;
	for (int i = 0; i < 16; i++)
	{
		XMVECTOR test_point = XMVectorSet(quad_node.bottom_left.x + quad_node.length * sample_pos[i][0], quad_node.bottom_left.y + quad_node.length * sample_pos[i][1], 0, 0);
		XMVECTOR eye_vec = test_point - eye_point;
		float dist = XMVectorGetX(XMVector3Length(eye_vec));

		float area_world = grid_len_world * grid_len_world;// * abs(eye_point.z) / sqrt(nearest_sqr_dist);
		float area_proj = area_world * XMVectorGetX(matProj.r[0]) * XMVectorGetY(matProj.r[1]) / (dist * dist);

		if (max_area_proj < area_proj)
			max_area_proj = area_proj;
	}

	float pixel_coverage = max_area_proj * screen_area * 0.25f;

	return pixel_coverage;
}

bool wiOcean::isLeaf(const QuadNode& quad_node)
{
	return (quad_node.sub_node[0] == -1 && quad_node.sub_node[1] == -1 && quad_node.sub_node[2] == -1 && quad_node.sub_node[3] == -1);
}

int wiOcean::searchLeaf(const vector<QuadNode>& node_list, const XMFLOAT2& point)
{
	int index = -1;

	int size = (int)node_list.size();
	QuadNode node = node_list[size - 1];

	while (!isLeaf(node))
	{
		bool found = false;

		for (int i = 0; i < 4; i++)
		{
			index = node.sub_node[i];
			if (index == -1)
				continue;

			QuadNode sub_node = node_list[index];
			if (point.x >= sub_node.bottom_left.x && point.x <= sub_node.bottom_left.x + sub_node.length &&
				point.y >= sub_node.bottom_left.y && point.y <= sub_node.bottom_left.y + sub_node.length)
			{
				node = sub_node;
				found = true;
				break;
			}
		}

		if (!found)
			return -1;
	}

	return index;
}

wiOcean::QuadRenderParam& wiOcean::selectMeshPattern(const QuadNode& quad_node)
{
	// Check 4 adjacent quad.
	XMVECTOR bottom_left = XMLoadFloat2(&quad_node.bottom_left);
	XMVECTOR tmp;

	XMFLOAT2 point_left;
	tmp = bottom_left + XMVectorSet(-g_PatchLength * 0.5f, quad_node.length * 0.5f, 0, 0);
	XMStoreFloat2(&point_left, tmp);
	int left_adj_index = searchLeaf(g_render_list, point_left);

	XMFLOAT2 point_right;
	tmp = bottom_left + XMVectorSet(quad_node.length + g_PatchLength * 0.5f, quad_node.length * 0.5f, 0, 0);
	XMStoreFloat2(&point_right, tmp);
	int right_adj_index = searchLeaf(g_render_list, point_right);

	XMFLOAT2 point_bottom;
	tmp = bottom_left + XMVectorSet(quad_node.length * 0.5f, -g_PatchLength * 0.5f, 0, 0);
	XMStoreFloat2(&point_right, tmp);
	int bottom_adj_index = searchLeaf(g_render_list, point_bottom);

	XMFLOAT2 point_top;
	tmp = bottom_left + XMVectorSet(quad_node.length * 0.5f, quad_node.length + g_PatchLength * 0.5f, 0, 0);
	XMStoreFloat2(&point_right, tmp);
	int top_adj_index = searchLeaf(g_render_list, point_top);

	int left_type = 0;
	if (left_adj_index != -1 && g_render_list[left_adj_index].length > quad_node.length * 0.999f)
	{
		QuadNode adj_node = g_render_list[left_adj_index];
		float scale = adj_node.length / quad_node.length * (g_MeshDim >> quad_node.lod) / (g_MeshDim >> adj_node.lod);
		if (scale > 3.999f)
			left_type = 2;
		else if (scale > 1.999f)
			left_type = 1;
	}

	int right_type = 0;
	if (right_adj_index != -1 && g_render_list[right_adj_index].length > quad_node.length * 0.999f)
	{
		QuadNode adj_node = g_render_list[right_adj_index];
		float scale = adj_node.length / quad_node.length * (g_MeshDim >> quad_node.lod) / (g_MeshDim >> adj_node.lod);
		if (scale > 3.999f)
			right_type = 2;
		else if (scale > 1.999f)
			right_type = 1;
	}

	int bottom_type = 0;
	if (bottom_adj_index != -1 && g_render_list[bottom_adj_index].length > quad_node.length * 0.999f)
	{
		QuadNode adj_node = g_render_list[bottom_adj_index];
		float scale = adj_node.length / quad_node.length * (g_MeshDim >> quad_node.lod) / (g_MeshDim >> adj_node.lod);
		if (scale > 3.999f)
			bottom_type = 2;
		else if (scale > 1.999f)
			bottom_type = 1;
	}

	int top_type = 0;
	if (top_adj_index != -1 && g_render_list[top_adj_index].length > quad_node.length * 0.999f)
	{
		QuadNode adj_node = g_render_list[top_adj_index];
		float scale = adj_node.length / quad_node.length * (g_MeshDim >> quad_node.lod) / (g_MeshDim >> adj_node.lod);
		if (scale > 3.999f)
			top_type = 2;
		else if (scale > 1.999f)
			top_type = 1;
	}

	// Check lookup table, [L][R][B][T]
	return g_mesh_patterns[quad_node.lod][left_type][right_type][bottom_type][top_type];
}

// Return value: if successful pushed into the list, return the position. If failed, return -1.
int wiOcean::buildNodeList(QuadNode& quad_node, const Camera& camera)
{
	// Check against view frustum
	if (!checkNodeVisibility(quad_node, camera))
		return -1;

	// Estimate the min grid coverage
	auto res = wiRenderer::GetInternalResolution();
	float min_coverage = estimateGridCoverage(quad_node, camera, (float)res.x*res.y);

	// Recursively attatch sub-nodes.
	bool visible = true;
	XMVECTOR bottom_left = XMLoadFloat2(&quad_node.bottom_left);
	XMFLOAT2 tmp;
	if (min_coverage > g_UpperGridCoverage && quad_node.length > g_PatchLength)
	{
		// Recursive rendering for sub-quads.
		QuadNode sub_node_0 = { quad_node.bottom_left, quad_node.length / 2, 0,{ -1, -1, -1, -1 } };
		quad_node.sub_node[0] = buildNodeList(sub_node_0, camera);

		XMStoreFloat2(&tmp, bottom_left + XMVectorSet(quad_node.length / 2, 0, 0, 0));
		QuadNode sub_node_1 = { tmp, quad_node.length / 2, 0,{ -1, -1, -1, -1 } };
		quad_node.sub_node[1] = buildNodeList(sub_node_1, camera);

		XMStoreFloat2(&tmp, bottom_left + XMVectorSet(quad_node.length / 2, quad_node.length / 2, 0, 0));
		QuadNode sub_node_2 = { tmp, quad_node.length / 2, 0,{ -1, -1, -1, -1 } };
		quad_node.sub_node[2] = buildNodeList(sub_node_2, camera);

		XMStoreFloat2(&tmp, bottom_left + XMVectorSet(0, quad_node.length / 2, 0, 0));
		QuadNode sub_node_3 = { tmp, quad_node.length / 2, 0,{ -1, -1, -1, -1 } };
		quad_node.sub_node[3] = buildNodeList(sub_node_3, camera);

		visible = !isLeaf(quad_node);
	}

	if (visible)
	{
		// Estimate mesh LOD
		int lod = 0;
		for (lod = 0; lod < g_Lods - 1; lod++)
		{
			if (min_coverage > g_UpperGridCoverage)
				break;
			min_coverage *= 4;
		}

		// We don't use 1x1 and 2x2 patch. So the highest level is g_Lods - 2.
		quad_node.lod = min(lod, g_Lods - 2);
	}
	else
		return -1;

	// Insert into the list
	int position = (int)g_render_list.size();
	g_render_list.push_back(quad_node);

	return position;
}

void wiOcean::Render(const Camera* camera, float time, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	bool wire = wiRenderer::IsWireRender();

	// Build rendering list
	g_render_list.clear();
	float ocean_extent = g_PatchLength * (1 << g_FurthestCover);
	QuadNode root_node = { XMFLOAT2(-ocean_extent * 0.5f, -ocean_extent * 0.5f), ocean_extent, 0,{ -1,-1,-1,-1 } };
	buildNodeList(root_node, *camera);

	// Matrices
	XMMATRIX matView = XMMATRIX(1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1) * camera->GetView();
	XMMATRIX matProj = camera->GetProjection();

	// VS & PS
	device->BindVS(g_pOceanSurfVS, threadID);
	device->BindPS(wire ? g_pWireframePS : g_pOceanSurfPS, threadID);

	// Textures
	GPUResource* vs_srvs[2] = { m_pDisplacementMap, g_pPerlinMap };
	device->BindResourcesVS(&vs_srvs[0], TEXSLOT_ONDEMAND0, 2, threadID);

	GPUResource* ps_srvs[4] = { g_pPerlinMap, m_pGradientMap, g_pFresnelMap, wiRenderer::GetEnviromentMap() };
	device->BindResourcesPS(&ps_srvs[0], TEXSLOT_ONDEMAND1, 4, threadID);

	// IA setup
	device->BindIndexBuffer(g_pMeshIB, INDEXBUFFER_FORMAT::INDEXFORMAT_32BIT, 0, threadID);

	GPUBuffer* vbs[1] = { g_pMeshVB };
	UINT strides[1] = { sizeof(ocean_vertex) };
	UINT offsets[1] = { 0 };
	device->BindVertexBuffers(&vbs[0], 0, 1, &strides[0], &offsets[0], threadID);

	device->BindVertexLayout(g_pMeshLayout, threadID);

	// State blocks
	device->BindRasterizerState(wire ? g_pRSState_Wireframe : g_pRSState_Solid, threadID);
	device->BindDepthStencilState(g_pDSState_Disable, 0, threadID);

	// Constants
	device->BindConstantBufferVS(g_pShadingCB, CB_GETBINDSLOT(Ocean_Rendering_ShadingCB), threadID);
	device->BindConstantBufferPS(g_pShadingCB, CB_GETBINDSLOT(Ocean_Rendering_ShadingCB), threadID);

	// We assume the center of the ocean surface at (0, 0, 0).
	for (int i = 0; i < (int)g_render_list.size(); i++)
	{
		QuadNode& node = g_render_list[i];

		if (!isLeaf(node))
			continue;

		// Check adjacent patches and select mesh pattern
		QuadRenderParam& render_param = selectMeshPattern(node);

		// Find the right LOD to render
		int level_size = g_MeshDim;
		for (int lod = 0; lod < node.lod; lod++)
			level_size >>= 1;

		// Matrices and constants
		Ocean_Rendering_PatchCB call_consts;

		// Expand of the local coordinate to world space patch size
		XMMATRIX matScale = XMMatrixScaling(node.length / level_size, node.length / level_size, 0);
		call_consts.g_matLocal = XMMatrixTranspose(matScale);

		// WVP matrix
		XMMATRIX matWorld = XMMatrixTranslation(node.bottom_left.x, node.bottom_left.y, 0);
		XMMATRIX matWVP = matWorld * matView * matProj;
		call_consts.g_matWorldViewProj = XMMatrixTranspose(matWVP);

		// Texcoord for perlin noise
		XMVECTOR uv_base = XMLoadFloat2(&node.bottom_left) / g_PatchLength * g_PerlinSize;
		XMStoreFloat2(&call_consts.g_UVBase, uv_base);

		// Constant g_PerlinSpeed need to be adjusted mannually
		XMVECTOR perlin_move = -XMLoadFloat2(&g_WindDir) * time * g_PerlinSpeed;
		XMStoreFloat2(&call_consts.g_PerlinMovement, perlin_move);

		// Eye point
		XMMATRIX matInvWV = XMMatrixInverse(nullptr, matWorld * matView);
		XMVECTOR vLocalEye = XMVector3TransformCoord(XMVectorSet(0, 0, 0, 1), matInvWV);
		XMStoreFloat3(&call_consts.g_LocalEye, vLocalEye);

		device->UpdateBuffer(g_pPerCallCB, &call_consts, threadID);

		device->BindConstantBufferVS(g_pPerCallCB, CB_GETBINDSLOT(Ocean_Rendering_PatchCB), threadID);
		device->BindConstantBufferPS(g_pPerCallCB, CB_GETBINDSLOT(Ocean_Rendering_PatchCB), threadID);

		// Perform draw call
		if (render_param.num_inner_faces > 0)
		{
			// Inner mesh of the patch
			device->BindPrimitiveTopology(TRIANGLESTRIP, threadID);
			device->DrawIndexed(render_param.num_inner_faces + 2, render_param.inner_start_index, 0, threadID);
		}

		if (render_param.num_boundary_faces > 0)
		{
			// Boundary mesh of the patch
			device->BindPrimitiveTopology(TRIANGLELIST, threadID);
			device->DrawIndexed(render_param.num_boundary_faces * 3, render_param.boundary_start_index, 0, threadID);
		}
	}

	//// Unbind
	//vs_srvs[0] = NULL;
	//vs_srvs[1] = NULL;
	//pd3dContext->VSSetShaderResources(0, 2, &vs_srvs[0]);

	//ps_srvs[0] = NULL;
	//ps_srvs[1] = NULL;
	//ps_srvs[2] = NULL;
	//ps_srvs[3] = NULL;
	//pd3dContext->PSSetShaderResources(1, 4, &ps_srvs[0]);
}


void wiOcean::LoadShaders()
{

	m_pUpdateSpectrumCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanSimulatorCS.cso", wiResourceManager::COMPUTESHADER));

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, 0, INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanQuadVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			m_pQuadVS = vsinfo->vertexShader;
			m_pQuadLayout = vsinfo->vertexLayout;
		}
	}

	m_pUpdateDisplacementPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanUpdateDisplacementPS.cso", wiResourceManager::PIXELSHADER));
	m_pGenGradientFoldingPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanGradientFoldingPS.cso", wiResourceManager::PIXELSHADER));


	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32_FLOAT, 0, 0, INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanSurfaceVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			g_pOceanSurfVS = vsinfo->vertexShader;
			g_pMeshLayout = vsinfo->vertexLayout;
		}
	}

	g_pOceanSurfPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanSurfacePS.cso", wiResourceManager::PIXELSHADER));
	g_pWireframePS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanSurfaceSimplePS.cso", wiResourceManager::PIXELSHADER));

}

void wiOcean::SetUpStatic()
{
	LoadShaders();
	CSFFT_512x512_Data_t::LoadShaders();
}

void wiOcean::CleanUpStatic()
{
	SAFE_DELETE(m_pUpdateSpectrumCS);
	SAFE_DELETE(m_pQuadVS);
	SAFE_DELETE(m_pUpdateDisplacementPS);
	SAFE_DELETE(m_pGenGradientFoldingPS);

	SAFE_DELETE(m_pQuadLayout);

	SAFE_DELETE(g_pMeshLayout);

	SAFE_DELETE(g_pOceanSurfVS);
	SAFE_DELETE(g_pOceanSurfPS);
	SAFE_DELETE(g_pWireframePS);
}
