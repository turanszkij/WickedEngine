#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "shaders/SamplerMapping.h"
#include "shaders/ResourceMapping.h"
#include "shaders/ShaderInterop_Image.h"
#include "wiBackLog.h"
#include "wiEvent.h"
#include "wiTimer.h"

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
		if (params.isHDR10OutputMappingEnabled())
		{
			assert(params.isFullScreenEnabled()); // for now, this effect is only usable in full screen rendering
			push.flags |= IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084;
		}
		if (params.isLinearOutputMappingEnabled())
		{
			assert(params.isFullScreenEnabled()); // for now, this effect is only usable in full screen rendering
			push.flags |= IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR;
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

		XMVECTOR V = XMVectorSet(params.corners[0].x - params.pivot.x, params.corners[0].y - params.pivot.y, 0, 1);
		V = XMVector2Transform(V, M); // division by w will happen on GPU
		XMStoreFloat4(&push.corners0, V);

		V = XMVectorSet(params.corners[1].x - params.pivot.x, params.corners[1].y - params.pivot.y, 0, 1);
		V = XMVector2Transform(V, M); // division by w will happen on GPU
		XMStoreFloat4(&push.corners1, V);

		V = XMVectorSet(params.corners[2].x - params.pivot.x, params.corners[2].y - params.pivot.y, 0, 1);
		V = XMVector2Transform(V, M); // division by w will happen on GPU
		XMStoreFloat4(&push.corners2, V);

		V = XMVectorSet(params.corners[3].x - params.pivot.x, params.corners[3].y - params.pivot.y, 0, 1);
		V = XMVector2Transform(V, M); // division by w will happen on GPU
		XMStoreFloat4(&push.corners3, V);

		if (params.isMirrorEnabled())
		{
			std::swap(push.corners0, push.corners1);
			std::swap(push.corners2, push.corners3);
		}

		const TextureDesc& desc = texture->GetDesc();
		const float inv_width = 1.0f / float(desc.width);
		const float inv_height = 1.0f / float(desc.height);

		XMFLOAT4 texMulAdd;
		if (params.isDrawRectEnabled())
		{
			texMulAdd.x = params.drawRect.z * inv_width;	// drawRec.width: mul
			texMulAdd.y = params.drawRect.w * inv_height;	// drawRec.heigh: mul
			texMulAdd.z = params.drawRect.x * inv_width;	// drawRec.x: add
			texMulAdd.w = params.drawRect.y * inv_height;	// drawRec.y: add
		}
		else
		{
			texMulAdd = XMFLOAT4(1, 1, 0, 0);	// disabled draw rect
		}
		texMulAdd.z += params.texOffset.x * inv_width;	// texOffset.x: add
		texMulAdd.w += params.texOffset.y * inv_height;	// texOffset.y: add
		XMHALF4 half_texMulAdd;
		half_texMulAdd.x = XMConvertFloatToHalf(texMulAdd.x);
		half_texMulAdd.y = XMConvertFloatToHalf(texMulAdd.y);
		half_texMulAdd.z = XMConvertFloatToHalf(texMulAdd.z);
		half_texMulAdd.w = XMConvertFloatToHalf(texMulAdd.w);
		push.texMulAdd.x = uint(half_texMulAdd.v);
		push.texMulAdd.y = uint(half_texMulAdd.v >> 32ull);

		XMFLOAT4 texMulAdd2;
		if (params.isDrawRect2Enabled())
		{
			texMulAdd2.x = params.drawRect2.z * inv_width;	// drawRec.width: mul
			texMulAdd2.y = params.drawRect2.w * inv_height;	// drawRec.heigh: mul
			texMulAdd2.z = params.drawRect2.x * inv_width;	// drawRec.x: add
			texMulAdd2.w = params.drawRect2.y * inv_height;	// drawRec.y: add
		}
		else
		{
			texMulAdd2 = XMFLOAT4(1, 1, 0, 0);	// disabled draw rect
		}
		texMulAdd2.z += params.texOffset2.x * inv_width;	// texOffset.x: add
		texMulAdd2.w += params.texOffset2.y * inv_height;	// texOffset.y: add
		XMHALF4 half_texMulAdd2;
		half_texMulAdd2.x = XMConvertFloatToHalf(texMulAdd2.x);
		half_texMulAdd2.y = XMConvertFloatToHalf(texMulAdd2.y);
		half_texMulAdd2.z = XMConvertFloatToHalf(texMulAdd2.z);
		half_texMulAdd2.w = XMConvertFloatToHalf(texMulAdd2.w);
		push.texMulAdd2.x = uint(half_texMulAdd2.v);
		push.texMulAdd2.y = uint(half_texMulAdd2.v >> 32ull);

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
		wiTimer timer;

		GraphicsDevice* device = wiRenderer::GetDevice();

		RasterizerState rs;
		rs.fill_mode = FILL_SOLID;
		rs.cull_mode = CULL_NONE;
		rs.front_counter_clockwise = false;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = true;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rasterizerState = rs;




		for (int i = 0; i < STENCILREFMODE_COUNT; ++i)
		{
			DepthStencilState dsd;
			dsd.depth_enable = false;
			dsd.stencil_enable = false;
			depthStencilStates[STENCILMODE_DISABLED][i] = dsd;

			dsd.stencil_enable = true;
			switch (i)
			{
			case STENCILREFMODE_ENGINE:
				dsd.stencil_read_mask = STENCILREF_MASK_ENGINE;
				break;
			case STENCILREFMODE_USER:
				dsd.stencil_read_mask = STENCILREF_MASK_USER;
				break;
			default:
				dsd.stencil_read_mask = STENCILREF_MASK_ALL;
				break;
			}
			dsd.stencil_write_mask = 0;
			dsd.front_face.stencil_pass_op = STENCIL_OP_KEEP;
			dsd.front_face.stencil_fail_op = STENCIL_OP_KEEP;
			dsd.front_face.stencil_depth_fail_op = STENCIL_OP_KEEP;
			dsd.back_face.stencil_pass_op = STENCIL_OP_KEEP;
			dsd.back_face.stencil_fail_op = STENCIL_OP_KEEP;
			dsd.back_face.stencil_depth_fail_op = STENCIL_OP_KEEP;

			dsd.front_face.stencil_func = COMPARISON_EQUAL;
			dsd.back_face.stencil_func = COMPARISON_EQUAL;
			depthStencilStates[STENCILMODE_EQUAL][i] = dsd;

			dsd.front_face.stencil_func = COMPARISON_LESS;
			dsd.back_face.stencil_func = COMPARISON_LESS;
			depthStencilStates[STENCILMODE_LESS][i] = dsd;

			dsd.front_face.stencil_func = COMPARISON_LESS_EQUAL;
			dsd.back_face.stencil_func = COMPARISON_LESS_EQUAL;
			depthStencilStates[STENCILMODE_LESSEQUAL][i] = dsd;

			dsd.front_face.stencil_func = COMPARISON_GREATER;
			dsd.back_face.stencil_func = COMPARISON_GREATER;
			depthStencilStates[STENCILMODE_GREATER][i] = dsd;

			dsd.front_face.stencil_func = COMPARISON_GREATER_EQUAL;
			dsd.back_face.stencil_func = COMPARISON_GREATER_EQUAL;
			depthStencilStates[STENCILMODE_GREATEREQUAL][i] = dsd;

			dsd.front_face.stencil_func = COMPARISON_NOT_EQUAL;
			dsd.back_face.stencil_func = COMPARISON_NOT_EQUAL;
			depthStencilStates[STENCILMODE_NOT][i] = dsd;

			dsd.front_face.stencil_func = COMPARISON_ALWAYS;
			dsd.back_face.stencil_func = COMPARISON_ALWAYS;
			depthStencilStates[STENCILMODE_ALWAYS][i] = dsd;
		}


		BlendState bd;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = BLEND_SRC_ALPHA;
		bd.render_target[0].dest_blend = BLEND_INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BLEND_OP_ADD;
		bd.render_target[0].src_blend_alpha = BLEND_ONE;
		bd.render_target[0].dest_blend_alpha = BLEND_INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BLEND_OP_ADD;
		bd.render_target[0].render_target_write_mask = COLOR_WRITE_ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_ALPHA] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = BLEND_ONE;
		bd.render_target[0].dest_blend = BLEND_INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BLEND_OP_ADD;
		bd.render_target[0].src_blend_alpha = BLEND_ONE;
		bd.render_target[0].dest_blend_alpha = BLEND_INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BLEND_OP_ADD;
		bd.render_target[0].render_target_write_mask = COLOR_WRITE_ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_PREMULTIPLIED] = bd;

		bd.render_target[0].blend_enable = false;
		bd.render_target[0].render_target_write_mask = COLOR_WRITE_ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_OPAQUE] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = BLEND_SRC_ALPHA;
		bd.render_target[0].dest_blend = BLEND_ONE;
		bd.render_target[0].blend_op = BLEND_OP_ADD;
		bd.render_target[0].src_blend_alpha = BLEND_ZERO;
		bd.render_target[0].dest_blend_alpha = BLEND_ONE;
		bd.render_target[0].blend_op_alpha = BLEND_OP_ADD;
		bd.render_target[0].render_target_write_mask = COLOR_WRITE_ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_ADDITIVE] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = BLEND_ZERO;
		bd.render_target[0].dest_blend = BLEND_SRC_COLOR;
		bd.render_target[0].blend_op = BLEND_OP_ADD;
		bd.render_target[0].src_blend_alpha = BLEND_ZERO;
		bd.render_target[0].dest_blend_alpha = BLEND_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BLEND_OP_ADD;
		bd.render_target[0].render_target_write_mask = COLOR_WRITE_ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_MULTIPLY] = bd;

		static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wiBackLog::post("wiImage Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		initialized.store(true);
	}

}
