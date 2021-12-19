#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "shaders/ShaderInterop_Image.h"
#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiTimer.h"

using namespace wi::graphics;
using namespace wi::enums;

namespace wi::image
{
	Sampler					samplers[SAMPLER_COUNT];
	Shader					vertexShader;
	Shader					pixelShader;
	BlendState				blendStates[BLENDMODE_COUNT];
	RasterizerState			rasterizerState;
	DepthStencilState		depthStencilStates[STENCILMODE_COUNT][STENCILREFMODE_COUNT];
	PipelineState			imagePSO[BLENDMODE_COUNT][STENCILMODE_COUNT][STENCILREFMODE_COUNT];
	Texture					backgroundTextures[COMMANDLIST_COUNT];
	wi::Canvas				canvases[COMMANDLIST_COUNT];

	void SetBackground(const Texture& texture, CommandList cmd)
	{
		backgroundTextures[cmd] = texture;
	}

	void SetCanvas(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
	{
		canvases[cmd] = canvas;
	}

	void Draw(const Texture* texture, const Params& params, CommandList cmd)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("Image", cmd);

		uint32_t stencilRef = params.stencilRef;
		if (params.stencilRefMode == STENCILREFMODE_USER)
		{
			stencilRef = wi::renderer::CombineStencilrefs(STENCILREF_EMPTY, (uint8_t)stencilRef);
		}
		device->BindStencilRef(stencilRef, cmd);

		const Sampler* sampler = &samplers[SAMPLER_LINEAR_CLAMP];

		if (params.quality == QUALITY_NEAREST)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				sampler = &samplers[SAMPLER_POINT_MIRROR];
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				sampler = &samplers[SAMPLER_POINT_WRAP];
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				sampler = &samplers[SAMPLER_POINT_CLAMP];
		}
		else if (params.quality == QUALITY_LINEAR)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				sampler = &samplers[SAMPLER_LINEAR_MIRROR];
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				sampler = &samplers[SAMPLER_LINEAR_WRAP];
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				sampler = &samplers[SAMPLER_LINEAR_CLAMP];
		}
		else if (params.quality == QUALITY_ANISOTROPIC)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				sampler = &samplers[SAMPLER_ANISO_MIRROR];
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				sampler = &samplers[SAMPLER_ANISO_WRAP];
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				sampler = &samplers[SAMPLER_ANISO_CLAMP];
		}

		ImageConstants image;
		ImagePushConstants image_push;
		image_push.texture_base_index = device->GetDescriptorIndex(texture, SubresourceType::SRV);
		image_push.texture_mask_index = device->GetDescriptorIndex(params.maskMap, SubresourceType::SRV);
		if (params.isBackgroundEnabled())
		{
			image_push.texture_background_index = device->GetDescriptorIndex(&backgroundTextures[cmd], SubresourceType::SRV);
		}
		else
		{
			image_push.texture_background_index = -1;
		}
		image_push.sampler_index = device->GetDescriptorIndex(sampler);

		const RenderPass* renderpass = device->GetCurrentRenderPass(cmd);
		assert(renderpass != nullptr); // image renderer must draw inside render pass!
		assert(!renderpass->GetDesc().attachments.empty());
		assert(renderpass->GetDesc().attachments.front().texture != nullptr);
		image.output_resolution.x = renderpass->GetDesc().attachments.front().texture->GetDesc().width;
		image.output_resolution.y = renderpass->GetDesc().attachments.front().texture->GetDesc().height;
		image.output_resolution_rcp.x = 1.0f / image.output_resolution.x;
		image.output_resolution_rcp.y = 1.0f / image.output_resolution.y;

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

		image_push.packed_color.x = uint(packed_color.v);
		image_push.packed_color.y = uint(packed_color.v >> 32ull);

		image_push.flags = 0;
		if (params.isExtractNormalMapEnabled())
		{
			image_push.flags |= IMAGE_FLAG_EXTRACT_NORMALMAP;
		}
		if (params.isHDR10OutputMappingEnabled())
		{
			image_push.flags |= IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084;
		}
		if (params.isLinearOutputMappingEnabled())
		{
			image_push.flags |= IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR;
			image_push.hdr_scaling = params.hdr_scaling;
		}
		if (params.isFullScreenEnabled())
		{
			image_push.flags |= IMAGE_FLAG_FULLSCREEN;
		}

		XMMATRIX M = XMMatrixScaling(params.scale.x * params.siz.x, params.scale.y * params.siz.y, 1);
		M = M * XMMatrixRotationZ(params.rotation);

		if (params.customRotation != nullptr)
		{
			M = M * (*params.customRotation);
		}

		M = M * XMMatrixTranslation(params.pos.x, params.pos.y, params.pos.z);

		if (!params.isFullScreenEnabled())
		{
			if (params.customProjection != nullptr)
			{
				M = XMMatrixScaling(1, -1, 1) * M; // reason: screen projection is Y down (like UV-space) and that is the common case for image rendering. But custom projections will use the "world space"
				M = M * (*params.customProjection);
			}
			else
			{
				const wi::Canvas& canvas = canvases[cmd];
				// Asserts will check that a proper canvas was set for this cmd with wi::image::SetCanvas()
				//	The canvas must be set to have dpi aware rendering
				assert(canvas.width > 0);
				assert(canvas.height > 0);
				assert(canvas.dpi > 0);
				M = M * canvas.GetProjection();
			}
		}

		XMVECTOR V = XMVectorSet(params.corners[0].x - params.pivot.x, params.corners[0].y - params.pivot.y, 0, 1);
		V = XMVector2Transform(V, M); // division by w will happen on GPU
		XMStoreFloat4(&image.corners0, V);

		V = XMVectorSet(params.corners[1].x - params.pivot.x, params.corners[1].y - params.pivot.y, 0, 1);
		V = XMVector2Transform(V, M); // division by w will happen on GPU
		XMStoreFloat4(&image.corners1, V);

		V = XMVectorSet(params.corners[2].x - params.pivot.x, params.corners[2].y - params.pivot.y, 0, 1);
		V = XMVector2Transform(V, M); // division by w will happen on GPU
		XMStoreFloat4(&image.corners2, V);

		V = XMVectorSet(params.corners[3].x - params.pivot.x, params.corners[3].y - params.pivot.y, 0, 1);
		V = XMVector2Transform(V, M); // division by w will happen on GPU
		XMStoreFloat4(&image.corners3, V);

		if (params.isMirrorEnabled())
		{
			std::swap(image.corners0, image.corners1);
			std::swap(image.corners2, image.corners3);
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
		image.texMulAdd.x = uint(half_texMulAdd.v);
		image.texMulAdd.y = uint(half_texMulAdd.v >> 32ull);

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
		image.texMulAdd2.x = uint(half_texMulAdd2.v);
		image.texMulAdd2.y = uint(half_texMulAdd2.v >> 32ull);

		device->BindPipelineState(&imagePSO[params.blendFlag][params.stencilComp][params.stencilRefMode], cmd);

		device->BindDynamicConstantBuffer(image, CBSLOT_IMAGE, cmd);
		device->PushConstants(&image_push, sizeof(image_push), cmd);

		if (params.isFullScreenEnabled())
		{
			device->Draw(3, 0, cmd);
		}
		else
		{
			device->Draw(4, 0, cmd);
		}

		device->EventEnd(cmd);
	}


	void LoadShaders()
	{
		wi::renderer::LoadShader(ShaderStage::VS, vertexShader, "imageVS.cso");
		wi::renderer::LoadShader(ShaderStage::PS, pixelShader, "imagePS.cso");

		GraphicsDevice* device = wi::graphics::GetDevice();

		PipelineStateDesc desc;
		desc.vs = &vertexShader;
		desc.ps = &pixelShader;
		desc.rs = &rasterizerState;
		desc.pt = PrimitiveTopology::TRIANGLESTRIP;

		for (int j = 0; j < BLENDMODE_COUNT; ++j)
		{
			desc.bs = &blendStates[j];
			for (int k = 0; k < STENCILMODE_COUNT; ++k)
			{
				for (int m = 0; m < STENCILREFMODE_COUNT; ++m)
				{
					desc.dss = &depthStencilStates[k][m];

					device->CreatePipelineState(&desc, &imagePSO[j][k][m]);

				}
			}
		}
	}

	void Initialize()
	{
		wi::Timer timer;

		GraphicsDevice* device = wi::graphics::GetDevice();

		RasterizerState rs;
		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::NONE;
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
			dsd.front_face.stencil_pass_op = StencilOp::KEEP;
			dsd.front_face.stencil_fail_op = StencilOp::KEEP;
			dsd.front_face.stencil_depth_fail_op = StencilOp::KEEP;
			dsd.back_face.stencil_pass_op = StencilOp::KEEP;
			dsd.back_face.stencil_fail_op = StencilOp::KEEP;
			dsd.back_face.stencil_depth_fail_op = StencilOp::KEEP;

			dsd.front_face.stencil_func = ComparisonFunc::EQUAL;
			dsd.back_face.stencil_func = ComparisonFunc::EQUAL;
			depthStencilStates[STENCILMODE_EQUAL][i] = dsd;

			dsd.front_face.stencil_func = ComparisonFunc::LESS;
			dsd.back_face.stencil_func = ComparisonFunc::LESS;
			depthStencilStates[STENCILMODE_LESS][i] = dsd;

			dsd.front_face.stencil_func = ComparisonFunc::LESS_EQUAL;
			dsd.back_face.stencil_func = ComparisonFunc::LESS_EQUAL;
			depthStencilStates[STENCILMODE_LESSEQUAL][i] = dsd;

			dsd.front_face.stencil_func = ComparisonFunc::GREATER;
			dsd.back_face.stencil_func = ComparisonFunc::GREATER;
			depthStencilStates[STENCILMODE_GREATER][i] = dsd;

			dsd.front_face.stencil_func = ComparisonFunc::GREATER_EQUAL;
			dsd.back_face.stencil_func = ComparisonFunc::GREATER_EQUAL;
			depthStencilStates[STENCILMODE_GREATEREQUAL][i] = dsd;

			dsd.front_face.stencil_func = ComparisonFunc::NOT_EQUAL;
			dsd.back_face.stencil_func = ComparisonFunc::NOT_EQUAL;
			depthStencilStates[STENCILMODE_NOT][i] = dsd;

			dsd.front_face.stencil_func = ComparisonFunc::ALWAYS;
			dsd.back_face.stencil_func = ComparisonFunc::ALWAYS;
			depthStencilStates[STENCILMODE_ALWAYS][i] = dsd;
		}


		BlendState bd;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_ALPHA] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::ONE;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_PREMULTIPLIED] = bd;

		bd.render_target[0].blend_enable = false;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_OPAQUE] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::ONE;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ZERO;
		bd.render_target[0].dest_blend_alpha = Blend::ONE;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_ADDITIVE] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::ZERO;
		bd.render_target[0].dest_blend = Blend::SRC_COLOR;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ZERO;
		bd.render_target[0].dest_blend_alpha = Blend::SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendStates[BLENDMODE_MULTIPLY] = bd;

		SamplerDesc samplerDesc;
		samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
		samplerDesc.address_u = TextureAddressMode::MIRROR;
		samplerDesc.address_v = TextureAddressMode::MIRROR;
		samplerDesc.address_w = TextureAddressMode::MIRROR;
		samplerDesc.mip_lod_bias = 0.0f;
		samplerDesc.max_anisotropy = 0;
		samplerDesc.comparison_func = ComparisonFunc::NEVER;
		samplerDesc.border_color = SamplerBorderColor::TRANSPARENT_BLACK;
		samplerDesc.min_lod = 0;
		samplerDesc.max_lod = std::numeric_limits<float>::max();
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_LINEAR_MIRROR]);

		samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
		samplerDesc.address_u = TextureAddressMode::CLAMP;
		samplerDesc.address_v = TextureAddressMode::CLAMP;
		samplerDesc.address_w = TextureAddressMode::CLAMP;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_LINEAR_CLAMP]);

		samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
		samplerDesc.address_u = TextureAddressMode::WRAP;
		samplerDesc.address_v = TextureAddressMode::WRAP;
		samplerDesc.address_w = TextureAddressMode::WRAP;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_LINEAR_WRAP]);

		samplerDesc.filter = Filter::MIN_MAG_MIP_POINT;
		samplerDesc.address_u = TextureAddressMode::MIRROR;
		samplerDesc.address_v = TextureAddressMode::MIRROR;
		samplerDesc.address_w = TextureAddressMode::MIRROR;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_POINT_MIRROR]);

		samplerDesc.filter = Filter::MIN_MAG_MIP_POINT;
		samplerDesc.address_u = TextureAddressMode::WRAP;
		samplerDesc.address_v = TextureAddressMode::WRAP;
		samplerDesc.address_w = TextureAddressMode::WRAP;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_POINT_WRAP]);


		samplerDesc.filter = Filter::MIN_MAG_MIP_POINT;
		samplerDesc.address_u = TextureAddressMode::CLAMP;
		samplerDesc.address_v = TextureAddressMode::CLAMP;
		samplerDesc.address_w = TextureAddressMode::CLAMP;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_POINT_CLAMP]);

		samplerDesc.filter = Filter::ANISOTROPIC;
		samplerDesc.address_u = TextureAddressMode::CLAMP;
		samplerDesc.address_v = TextureAddressMode::CLAMP;
		samplerDesc.address_w = TextureAddressMode::CLAMP;
		samplerDesc.max_anisotropy = 16;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_ANISO_CLAMP]);

		samplerDesc.filter = Filter::ANISOTROPIC;
		samplerDesc.address_u = TextureAddressMode::WRAP;
		samplerDesc.address_v = TextureAddressMode::WRAP;
		samplerDesc.address_w = TextureAddressMode::WRAP;
		samplerDesc.max_anisotropy = 16;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_ANISO_WRAP]);

		samplerDesc.filter = Filter::ANISOTROPIC;
		samplerDesc.address_u = TextureAddressMode::MIRROR;
		samplerDesc.address_v = TextureAddressMode::MIRROR;
		samplerDesc.address_w = TextureAddressMode::MIRROR;
		samplerDesc.max_anisotropy = 16;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_ANISO_MIRROR]);

		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wi::backlog::post("wi::image Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

}
