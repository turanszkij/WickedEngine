#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "shaders/SamplerMapping.h"
#include "shaders/ResourceMapping.h"
#include "shaders/ShaderInterop_Image.h"
#include "wiBackLog.h"
#include "wiEvent.h"

#include <atomic>

using namespace wiGraphics;

namespace wiImage
{

	enum IMAGE_SHADER
	{
		IMAGE_SHADER_STANDARD,
		IMAGE_SHADER_FULLSCREEN,
		IMAGE_SHADER_COUNT
	};

	Shader					vertexShader;
	Shader					screenVS;
	Shader					imagePS[IMAGE_SHADER_COUNT];
	BlendState				blendStates[BLENDMODE_COUNT];
	RasterizerState			rasterizerState;
	DepthStencilState		depthStencilStates[STENCILMODE_COUNT][STENCILREFMODE_COUNT];
	PipelineState			imagePSO[IMAGE_SHADER_COUNT][BLENDMODE_COUNT][STENCILMODE_COUNT][STENCILREFMODE_COUNT];
	Texture					backgroundTextures[COMMANDLIST_COUNT];
	wiCanvas				canvases[COMMANDLIST_COUNT];

	std::atomic_bool initialized{ false };

	void SetBackground(const Texture& texture, CommandList cmd)
	{
		backgroundTextures[cmd] = texture;
	}

	void SetCanvas(const wiCanvas& canvas, wiGraphics::CommandList cmd)
	{
		canvases[cmd] = canvas;
	}

	void Draw(const Texture* texture, const wiImageParams& params, CommandList cmd)
	{
		if (!initialized.load())
		{
			return;
		}

		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin("Image", cmd);

		uint32_t stencilRef = params.stencilRef;
		if (params.stencilRefMode == STENCILREFMODE_USER)
		{
			stencilRef = wiRenderer::CombineStencilrefs(STENCILREF_EMPTY, (uint8_t)stencilRef);
		}
		device->BindStencilRef(stencilRef, cmd);

		const Sampler* sampler = wiRenderer::GetSampler(SSLOT_LINEAR_CLAMP);

		if (params.quality == QUALITY_NEAREST)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				sampler = wiRenderer::GetSampler(SSLOT_POINT_MIRROR);
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				sampler = wiRenderer::GetSampler(SSLOT_POINT_WRAP);
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				sampler = wiRenderer::GetSampler(SSLOT_POINT_CLAMP);
		}
		else if (params.quality == QUALITY_LINEAR)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				sampler = wiRenderer::GetSampler(SSLOT_LINEAR_MIRROR);
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				sampler = wiRenderer::GetSampler(SSLOT_LINEAR_WRAP);
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				sampler = wiRenderer::GetSampler(SSLOT_LINEAR_CLAMP);
		}
		else if (params.quality == QUALITY_ANISOTROPIC)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				sampler = wiRenderer::GetSampler(SSLOT_ANISO_MIRROR);
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				sampler = wiRenderer::GetSampler(SSLOT_ANISO_WRAP);
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				sampler = wiRenderer::GetSampler(SSLOT_ANISO_CLAMP);
		}

		PushConstantsImage push;
		push.texture_base_index = device->GetDescriptorIndex(texture, SRV);
		push.texture_mask_index = device->GetDescriptorIndex(params.maskMap, SRV);
		if (params.isBackgroundEnabled())
		{
			push.texture_background_index = device->GetDescriptorIndex(&backgroundTextures[cmd], SRV);
		}
		else
		{
			push.texture_background_index = -1;
		}
		push.sampler_index = device->GetDescriptorIndex(sampler);

		XMFLOAT4 color = params.color;
		const float darken = 1 - params.fade;
		color.x *= darken;
		color.y *= darken;
		color.z *= darken;
		color.w *= params.opacity;

		XMHALF4 packed_color;
		packed_color.x = XMConvertFloatToHalf(color.x);
		packed_color.y = XMConvertFloatToHalf(color.y);
		packed_color.z = XMConvertFloatToHalf(color.z);
		packed_color.w = XMConvertFloatToHalf(color.w);

		push.packed_color.x = uint(packed_color.v);
		push.packed_color.y = uint(packed_color.v >> 32ull);

		push.flags = 0;
		if (params.isExtractNormalMapEnabled())
		{
			push.flags |= IMAGE_FLAG_EXTRACT_NORMALMAP;
		}

		if (params.isFullScreenEnabled())
		{
			// Full screen image uses a fast path with full screen triangle and no effects
			device->BindPipelineState(&imagePSO[IMAGE_SHADER_FULLSCREEN][params.blendFlag][params.stencilComp][params.stencilRefMode], cmd);
			device->PushConstants(&push, sizeof(push), cmd);
			device->Draw(3, 0, cmd);
			device->EventEnd(cmd);
			return;
		}

		XMMATRIX M = XMMatrixScaling(params.scale.x * params.siz.x, params.scale.y * params.siz.y, 1);
		M = M * XMMatrixRotationZ(params.rotation);

		if (params.customRotation != nullptr)
		{
			M = M * (*params.customRotation);
		}

		M = M * XMMatrixTranslation(params.pos.x, params.pos.y, params.pos.z);

		if (params.customProjection != nullptr)
		{
			M = XMMatrixScaling(1, -1, 1) * M; // reason: screen projection is Y down (like UV-space) and that is the common case for image rendering. But custom projections will use the "world space"
			M = M * (*params.customProjection);
		}
		else
		{
			const wiCanvas& canvas = canvases[cmd];
			// Asserts will check that a proper canvas was set for this cmd with wiImage::SetCanvas()
			//	The canvas must be set to have dpi aware rendering
			assert(canvas.width > 0);
			assert(canvas.height > 0);
			assert(canvas.dpi > 0);
			M = M * canvas.GetProjection();
		}

		for (int i = 0; i < 4; ++i)
		{
			XMVECTOR V = XMVectorSet(params.corners[i].x - params.pivot.x, params.corners[i].y - params.pivot.y, 0, 1);
			V = XMVector2Transform(V, M); // division by w will happen on GPU
			XMStoreFloat4(&push.corners[i], V);
		}

		if (params.isMirrorEnabled())
		{
			std::swap(push.corners[0], push.corners[1]);
			std::swap(push.corners[2], push.corners[3]);
		}

		const TextureDesc& desc = texture->GetDesc();
		const float inv_width = 1.0f / float(desc.Width);
		const float inv_height = 1.0f / float(desc.Height);

		if (params.isDrawRectEnabled())
		{
			push.texMulAdd.x = params.drawRect.z * inv_width;	// drawRec.width: mul
			push.texMulAdd.y = params.drawRect.w * inv_height;	// drawRec.heigh: mul
			push.texMulAdd.z = params.drawRect.x * inv_width;	// drawRec.x: add
			push.texMulAdd.w = params.drawRect.y * inv_height;	// drawRec.y: add
		}
		else
		{
			push.texMulAdd = XMFLOAT4(1, 1, 0, 0);	// disabled draw rect
		}
		push.texMulAdd.z += params.texOffset.x * inv_width;	// texOffset.x: add
		push.texMulAdd.w += params.texOffset.y * inv_height;	// texOffset.y: add

		if (params.isDrawRect2Enabled())
		{
			push.texMulAdd2.x = params.drawRect2.z * inv_width;	// drawRec.width: mul
			push.texMulAdd2.y = params.drawRect2.w * inv_height;	// drawRec.heigh: mul
			push.texMulAdd2.z = params.drawRect2.x * inv_width;	// drawRec.x: add
			push.texMulAdd2.w = params.drawRect2.y * inv_height;	// drawRec.y: add
		}
		else
		{
			push.texMulAdd2 = XMFLOAT4(1, 1, 0, 0);	// disabled draw rect
		}
		push.texMulAdd2.z += params.texOffset2.x * inv_width;	// texOffset.x: add
		push.texMulAdd2.w += params.texOffset2.y * inv_height;	// texOffset.y: add

		device->BindPipelineState(&imagePSO[IMAGE_SHADER_STANDARD][params.blendFlag][params.stencilComp][params.stencilRefMode], cmd);

		device->PushConstants(&push, sizeof(push), cmd);

		device->Draw(4, 0, cmd);

		device->EventEnd(cmd);
	}


	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		wiRenderer::LoadShader(VS, vertexShader, "imageVS.cso");
		wiRenderer::LoadShader(VS, screenVS, "screenVS.cso");

		wiRenderer::LoadShader(PS, imagePS[IMAGE_SHADER_STANDARD], "imagePS.cso");
		wiRenderer::LoadShader(PS, imagePS[IMAGE_SHADER_FULLSCREEN], "screenPS.cso");


		GraphicsDevice* device = wiRenderer::GetDevice();

		for (int i = 0; i < IMAGE_SHADER_COUNT; ++i)
		{
			PipelineStateDesc desc;
			desc.vs = &vertexShader;
			if (i == IMAGE_SHADER_FULLSCREEN)
			{
				desc.vs = &screenVS;
			}
			desc.rs = &rasterizerState;
			desc.pt = TRIANGLESTRIP;

			desc.ps = &imagePS[i];

			for (int j = 0; j < BLENDMODE_COUNT; ++j)
			{
				desc.bs = &blendStates[j];
				for (int k = 0; k < STENCILMODE_COUNT; ++k)
				{
					for (int m = 0; m < STENCILREFMODE_COUNT; ++m)
					{
						desc.dss = &depthStencilStates[k][m];

						device->CreatePipelineState(&desc, &imagePSO[i][j][k][m]);

					}
				}
			}
		}


	}

	void Initialize()
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		RasterizerState rs;
		rs.FillMode = FILL_SOLID;
		rs.CullMode = CULL_NONE;
		rs.FrontCounterClockwise = false;
		rs.DepthBias = 0;
		rs.DepthBiasClamp = 0;
		rs.SlopeScaledDepthBias = 0;
		rs.DepthClipEnable = true;
		rs.MultisampleEnable = false;
		rs.AntialiasedLineEnable = false;
		rasterizerState = rs;




		for (int i = 0; i < STENCILREFMODE_COUNT; ++i)
		{
			DepthStencilState dsd;
			dsd.DepthEnable = false;
			dsd.StencilEnable = false;
			depthStencilStates[STENCILMODE_DISABLED][i] = dsd;

			dsd.StencilEnable = true;
			switch (i)
			{
			case STENCILREFMODE_ENGINE:
				dsd.StencilReadMask = STENCILREF_MASK_ENGINE;
				break;
			case STENCILREFMODE_USER:
				dsd.StencilReadMask = STENCILREF_MASK_USER;
				break;
			default:
				dsd.StencilReadMask = STENCILREF_MASK_ALL;
				break;
			}
			dsd.StencilWriteMask = 0;
			dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
			dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
			dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
			dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
			dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
			dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;

			dsd.FrontFace.StencilFunc = COMPARISON_EQUAL;
			dsd.BackFace.StencilFunc = COMPARISON_EQUAL;
			depthStencilStates[STENCILMODE_EQUAL][i] = dsd;

			dsd.FrontFace.StencilFunc = COMPARISON_LESS;
			dsd.BackFace.StencilFunc = COMPARISON_LESS;
			depthStencilStates[STENCILMODE_LESS][i] = dsd;

			dsd.FrontFace.StencilFunc = COMPARISON_LESS_EQUAL;
			dsd.BackFace.StencilFunc = COMPARISON_LESS_EQUAL;
			depthStencilStates[STENCILMODE_LESSEQUAL][i] = dsd;

			dsd.FrontFace.StencilFunc = COMPARISON_GREATER;
			dsd.BackFace.StencilFunc = COMPARISON_GREATER;
			depthStencilStates[STENCILMODE_GREATER][i] = dsd;

			dsd.FrontFace.StencilFunc = COMPARISON_GREATER_EQUAL;
			dsd.BackFace.StencilFunc = COMPARISON_GREATER_EQUAL;
			depthStencilStates[STENCILMODE_GREATEREQUAL][i] = dsd;

			dsd.FrontFace.StencilFunc = COMPARISON_NOT_EQUAL;
			dsd.BackFace.StencilFunc = COMPARISON_NOT_EQUAL;
			depthStencilStates[STENCILMODE_NOT][i] = dsd;

			dsd.FrontFace.StencilFunc = COMPARISON_ALWAYS;
			dsd.BackFace.StencilFunc = COMPARISON_ALWAYS;
			depthStencilStates[STENCILMODE_ALWAYS][i] = dsd;
		}


		BlendState bd;
		bd.RenderTarget[0].BlendEnable = true;
		bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		blendStates[BLENDMODE_ALPHA] = bd;

		bd.RenderTarget[0].BlendEnable = true;
		bd.RenderTarget[0].SrcBlend = BLEND_ONE;
		bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		blendStates[BLENDMODE_PREMULTIPLIED] = bd;

		bd.RenderTarget[0].BlendEnable = false;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		blendStates[BLENDMODE_OPAQUE] = bd;

		bd.RenderTarget[0].BlendEnable = true;
		bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = BLEND_ONE;
		bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
		bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		blendStates[BLENDMODE_ADDITIVE] = bd;

		bd.RenderTarget[0].BlendEnable = true;
		bd.RenderTarget[0].SrcBlend = BLEND_ZERO;
		bd.RenderTarget[0].DestBlend = BLEND_SRC_COLOR;
		bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
		bd.RenderTarget[0].DestBlendAlpha = BLEND_SRC_ALPHA;
		bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		blendStates[BLENDMODE_MULTIPLY] = bd;

		static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wiBackLog::post("wiImage Initialized");
		initialized.store(true);
	}

}
