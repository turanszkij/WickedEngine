#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "shaders/ShaderInterop_Image.h"
#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiTimer.h"

using namespace wi::enums;
using namespace wi::graphics;

namespace wi::image
{
	static Sampler samplers[SAMPLER_COUNT];
	static Shader vertexShader;
	static Shader pixelShader;
	static BlendState blendStates[BLENDMODE_COUNT];
	static RasterizerState rasterizerState;
	enum DEPTH_TEST_MODE
	{
		DEPTH_TEST_OFF,
		DEPTH_TEST_ON,
		DEPTH_TEST_MODE_COUNT
	};
	static DepthStencilState depthStencilStates[STENCILMODE_COUNT][STENCILREFMODE_COUNT][DEPTH_TEST_MODE_COUNT];
	enum STRIP_MODE
	{
		STRIP_OFF,
		STRIP_ON,
		STRIP_MODE_COUNT,
	};
	static PipelineState imagePSO[BLENDMODE_COUNT][STENCILMODE_COUNT][STENCILREFMODE_COUNT][DEPTH_TEST_MODE_COUNT][STRIP_MODE_COUNT];
	static thread_local Texture backgroundTexture;
	static thread_local wi::Canvas canvas;

	void SetBackground(const Texture& texture)
	{
		backgroundTexture = texture;
	}

	void SetCanvas(const wi::Canvas& current_canvas)
	{
		canvas = current_canvas;
	}

	void Draw(const Texture* texture, const Params& params, CommandList cmd)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

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

		ImageConstants image = {};
		image.buffer_index = -1;
		image.buffer_offset = 0;
		image.texture_base_index = device->GetDescriptorIndex(texture, SubresourceType::SRV, params.image_subresource);
		image.texture_mask_index = device->GetDescriptorIndex(params.maskMap, SubresourceType::SRV, params.mask_subresource);
		if (params.isBackgroundEnabled())
		{
			if (params.backgroundMap != nullptr)
			{
				image.texture_background_index = device->GetDescriptorIndex(params.backgroundMap, SubresourceType::SRV, params.background_subresource);
			}
			else
			{
				image.texture_background_index = device->GetDescriptorIndex(&backgroundTexture, SubresourceType::SRV);
			}
		}
		else
		{
			image.texture_background_index = -1;
		}
		image.sampler_index = device->GetDescriptorIndex(sampler);
		if (image.sampler_index < 0)
			return;

		XMFLOAT4 color = params.color;
		const float darken = 1 - params.fade;
		color.x *= darken;
		color.y *= darken;
		color.z *= darken;
		color.x *= params.intensity;
		color.y *= params.intensity;
		color.z *= params.intensity;
		color.w *= params.opacity;

		XMHALF4 packed_color;
		packed_color.x = XMConvertFloatToHalf(color.x);
		packed_color.y = XMConvertFloatToHalf(color.y);
		packed_color.z = XMConvertFloatToHalf(color.z);
		packed_color.w = XMConvertFloatToHalf(color.w);

		image.packed_color.x = uint(packed_color.v);
		image.packed_color.y = uint(packed_color.v >> 32ull);

		if (params.angular_softness_outer_angle > 0)
		{
			const float innerConeAngleCos = std::cos(params.angular_softness_inner_angle);
			const float outerConeAngleCos = std::cos(params.angular_softness_outer_angle);
			// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#inner-and-outer-cone-angles
			const float lightAngleScale = 1.0f / std::max(0.001f, innerConeAngleCos - outerConeAngleCos);
			const float lightAngleOffset = -outerConeAngleCos * lightAngleScale;
			image.angular_softness_direction = params.angular_softness_direction;
			image.angular_softness_scale = lightAngleScale;
			image.angular_softness_offset = lightAngleOffset;
		}

		image.flags = 0;
		if (params.isExtractNormalMapEnabled())
		{
			image.flags |= IMAGE_FLAG_EXTRACT_NORMALMAP;
		}
		if (params.isHDR10OutputMappingEnabled())
		{
			image.flags |= IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084;
		}
		if (params.isLinearOutputMappingEnabled())
		{
			image.flags |= IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR;
			image.hdr_scaling = params.hdr_scaling;
		}
		if (params.isFullScreenEnabled())
		{
			image.flags |= IMAGE_FLAG_FULLSCREEN;
		}
		if (params.isAngularSoftnessDoubleSided())
		{
			image.flags |= IMAGE_FLAG_ANGULAR_DOUBLESIDED;
		}
		if (params.isAngularSoftnessInverse())
		{
			image.flags |= IMAGE_FLAG_ANGULAR_INVERSE;
		}
		if (params.isDistortionMaskEnabled())
		{
			image.flags |= IMAGE_FLAG_DISTORTION_MASK;
		}

		image.border_soften = params.border_soften;
		image.mask_alpha_range =  XMConvertFloatToHalf(params.mask_alpha_range_start) | (XMConvertFloatToHalf(params.mask_alpha_range_end) << 16u);

		STRIP_MODE strip_mode = STRIP_ON;
		uint32_t index_count = 0;

		if (params.isFullScreenEnabled())
		{
			// full screen triangle, no vertex buffer:
			image.buffer_index = -1;
			image.buffer_offset = 0;
		}
		else
		{
			// vertex buffer:
			XMMATRIX S = XMMatrixScaling(params.scale.x * params.siz.x, params.scale.y * params.siz.y, 1);
			XMMATRIX M = XMMatrixRotationZ(params.rotation);

			if (params.customRotation != nullptr)
			{
				M = M * (*params.customRotation);
			}

			M = M * XMMatrixTranslation(params.pos.x, params.pos.y, params.pos.z);

			if (params.customProjection != nullptr)
			{
				S = XMMatrixScaling(1, -1, 1) * S; // reason: screen projection is Y down (like UV-space) and that is the common case for image rendering. But custom projections will use the "world space"
				M = M * (*params.customProjection);
			}
			else
			{
				// Asserts will check that a proper canvas was set for this cmd with wi::image::SetCanvas()
				//	The canvas must be set to have dpi aware rendering
				assert(canvas.width > 0);
				assert(canvas.height > 0);
				assert(canvas.dpi > 0);
				M = M * canvas.GetProjection();
			}

			XMVECTOR V[4];
			float4 corners[4];
			for (int i = 0; i < arraysize(params.corners); ++i)
			{
				V[i] = XMVectorSet(params.corners[i].x - params.pivot.x, params.corners[i].y - params.pivot.y, 0, 1);
				V[i] = XMVector2Transform(V[i], S);
				XMStoreFloat4(corners + i, XMVector2Transform(V[i], M)); // division by w will happen on GPU
			}

			image.b0 = float2(corners[0].x, corners[0].y);
			image.b1 = float2(corners[1].x - corners[0].x, corners[1].y - corners[0].y);
			image.b2 = float2(corners[2].x - corners[0].x, corners[2].y - corners[0].y);
			image.b3 = float2(corners[0].x - corners[1].x - corners[2].x + corners[3].x, corners[0].y - corners[1].y - corners[2].y + corners[3].y);

			if (params.isCornerRoundingEnabled())
			{
				// The rounded corner mode will use a triangle fan structure (implemrnted by indexed triangle list):
				strip_mode = STRIP_OFF;
				image.flags |= IMAGE_FLAG_CORNER_ROUNDING;
				size_t vertex_count = 1; // start with center vertex
				const int min_segment_count = 2;
				for (int i = 0; i < arraysize(params.corners_rounding); ++i)
				{
					int segments = std::max(min_segment_count, params.corners_rounding[i].segments);
					vertex_count += segments;
					index_count += segments * 3;
				}
				index_count += 3; // closing triangle
				const size_t vb_size = sizeof(float4) * vertex_count;
				const size_t ib_size = sizeof(uint16_t) * index_count;
				GraphicsDevice::GPUAllocation mem = device->AllocateGPU(vb_size + ib_size, cmd);
				image.buffer_index = device->GetDescriptorIndex(&mem.buffer, SubresourceType::SRV);
				image.buffer_offset = (uint)mem.offset;
				device->BindIndexBuffer(&mem.buffer, IndexBufferFormat::UINT16, mem.offset + vb_size, cmd);
				float4* vertices = (float4*)mem.data;
				uint16_t* indices = (uint16_t*)((uint8_t*)mem.data + vb_size);
				uint32_t vi = 0;
				uint32_t ii = 0;
				XMStoreFloat4(vertices + vi, XMVector2Transform((V[0] + V[1] + V[2] + V[3]) * 0.25f, M)); // center vertex
				vi++;
				for (int i = 0; i < arraysize(params.corners_rounding); ++i)
				{
					Params::Rounding rounding;
					XMVECTOR A;
					XMVECTOR B;
					XMVECTOR C;
					switch (i)
					{
					default:
					case 0:
						rounding = params.corners_rounding[0];
						A = V[2];
						B = V[0];
						C = V[1];
						break;
					case 1:
						rounding = params.corners_rounding[1];
						A = V[0];
						B = V[1];
						C = V[3];
						break;
					case 2:
						rounding = params.corners_rounding[3];
						A = V[1];
						B = V[3];
						C = V[2];
						break;
					case 3:
						rounding = params.corners_rounding[2];
						A = V[3];
						B = V[2];
						C = V[0];
						break;
					}
					rounding.segments = std::max(min_segment_count, rounding.segments);
					const XMVECTOR BA = A - B;
					const XMVECTOR BC = C - B;
					const XMVECTOR BA_length = XMVector2Length(BA);
					const XMVECTOR BC_length = XMVector2Length(BC);
					const XMVECTOR radius = XMVectorReplicate(rounding.radius);
					const XMVECTOR start = B + BA / BA_length * XMVectorMin(radius, BA_length * 0.5f);
					const XMVECTOR end = B + BC / BC_length * XMVectorMin(radius, BC_length * 0.5f);

					for (int j = 0; j < rounding.segments; ++j)
					{
						float t = float(j) / float(rounding.segments - 1);
						XMVECTOR bezier = wi::math::GetQuadraticBezierPos(start, B, end, t);
						XMStoreFloat4(vertices + vi, XMVector2Transform(bezier, M));
						indices[ii++] = 0;
						indices[ii++] = vi - 1;
						indices[ii++] = vi;
						vi++;
					}
				}
				// closing the triangle fan:
				indices[ii++] = 0;
				indices[ii++] = vi - 1;
				indices[ii++] = 1;
			}
			else
			{
				// Non rounded image will simply use a 4 vertex triangle strip (simple quad)
				GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(float4) * 4, cmd);
				image.buffer_index = device->GetDescriptorIndex(&mem.buffer, SubresourceType::SRV);
				image.buffer_offset = (uint)mem.offset;
				std::memcpy(mem.data, corners, sizeof(corners));
			}
		}

		if (params.isMirrorEnabled())
		{
			image.flags |= IMAGE_FLAG_MIRROR;
		}

		float inv_width = 1.0f;
		float inv_height = 1.0f;
		if (texture != nullptr)
		{
			const TextureDesc& desc = texture->GetDesc();
			inv_width = 1.0f / float(desc.width);
			inv_height = 1.0f / float(desc.height);
		}

		if (params.isDrawRectEnabled())
		{
			image.texMulAdd.x = params.drawRect.z * inv_width;	// drawRec.width: mul
			image.texMulAdd.y = params.drawRect.w * inv_height;	// drawRec.heigh: mul
			image.texMulAdd.z = params.drawRect.x * inv_width;	// drawRec.x: add
			image.texMulAdd.w = params.drawRect.y * inv_height;	// drawRec.y: add
		}
		else
		{
			image.texMulAdd = XMFLOAT4(1, 1, 0, 0);	// disabled draw rect
		}
		image.texMulAdd.z += params.texOffset.x * inv_width;	// texOffset.x: add
		image.texMulAdd.w += params.texOffset.y * inv_height;	// texOffset.y: add

		if (params.isDrawRect2Enabled())
		{
			image.texMulAdd2.x = params.drawRect2.z * inv_width;	// drawRec.width: mul
			image.texMulAdd2.y = params.drawRect2.w * inv_height;	// drawRec.heigh: mul
			image.texMulAdd2.z = params.drawRect2.x * inv_width;	// drawRec.x: add
			image.texMulAdd2.w = params.drawRect2.y * inv_height;	// drawRec.y: add
		}
		else
		{
			image.texMulAdd2 = XMFLOAT4(1, 1, 0, 0);	// disabled draw rect
		}
		image.texMulAdd2.z += params.texOffset2.x * inv_width;	// texOffset.x: add
		image.texMulAdd2.w += params.texOffset2.y * inv_height;	// texOffset.y: add

		device->EventBegin("Image", cmd);

		uint32_t stencilRef = params.stencilRef;
		if (params.stencilRefMode == STENCILREFMODE_USER)
		{
			stencilRef = wi::renderer::CombineStencilrefs(STENCILREF_EMPTY, (uint8_t)stencilRef);
		}
		device->BindStencilRef(stencilRef, cmd);

		device->BindPipelineState(&imagePSO[params.blendFlag][params.stencilComp][params.stencilRefMode][params.isDepthTestEnabled()][strip_mode], cmd);

		device->BindDynamicConstantBuffer(image, CBSLOT_IMAGE, cmd);

		if (params.isFullScreenEnabled())
		{
			device->Draw(3, 0, cmd); // full screen triangle
		}
		else
		{
			switch (strip_mode)
			{
			case wi::image::STRIP_OFF:
				device->DrawIndexed(index_count, 0, 0, cmd); // corner rounding with indexed geometry
				break;
			case wi::image::STRIP_ON:
			default:
				device->Draw(4, 0, cmd); // simple quad
				break;
			}
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

		for (int j = 0; j < BLENDMODE_COUNT; ++j)
		{
			desc.bs = &blendStates[j];
			for (int k = 0; k < STENCILMODE_COUNT; ++k)
			{
				for (int m = 0; m < STENCILREFMODE_COUNT; ++m)
				{
					for (int d = 0; d < DEPTH_TEST_MODE_COUNT; ++d)
					{
						desc.dss = &depthStencilStates[k][m][d];

						for (int n = 0; n < STRIP_MODE_COUNT; ++n)
						{
							switch (n)
							{
							default:
							case STRIP_ON:
								desc.pt = PrimitiveTopology::TRIANGLESTRIP;
								break;
							case STRIP_OFF:
								desc.pt = PrimitiveTopology::TRIANGLELIST;
								break;
							}
							device->CreatePipelineState(&desc, &imagePSO[j][k][m][d][n]);
						}
					}
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
			for (int d = 0; d < DEPTH_TEST_MODE_COUNT; ++d)
			{
				DepthStencilState dsd;
				dsd.depth_write_mask = DepthWriteMask::ZERO;

				switch (d)
				{
				default:
				case DEPTH_TEST_OFF:
					dsd.depth_enable = false;
					break;
				case DEPTH_TEST_ON:
					dsd.depth_enable = true;
					dsd.depth_func = ComparisonFunc::GREATER_EQUAL;
					break;
				}

				dsd.stencil_enable = false;
				depthStencilStates[STENCILMODE_DISABLED][i][d] = dsd;

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
				depthStencilStates[STENCILMODE_EQUAL][i][d] = dsd;

				dsd.front_face.stencil_func = ComparisonFunc::LESS;
				dsd.back_face.stencil_func = ComparisonFunc::LESS;
				depthStencilStates[STENCILMODE_LESS][i][d] = dsd;

				dsd.front_face.stencil_func = ComparisonFunc::LESS_EQUAL;
				dsd.back_face.stencil_func = ComparisonFunc::LESS_EQUAL;
				depthStencilStates[STENCILMODE_LESSEQUAL][i][d] = dsd;

				dsd.front_face.stencil_func = ComparisonFunc::GREATER;
				dsd.back_face.stencil_func = ComparisonFunc::GREATER;
				depthStencilStates[STENCILMODE_GREATER][i][d] = dsd;

				dsd.front_face.stencil_func = ComparisonFunc::GREATER_EQUAL;
				dsd.back_face.stencil_func = ComparisonFunc::GREATER_EQUAL;
				depthStencilStates[STENCILMODE_GREATEREQUAL][i][d] = dsd;

				dsd.front_face.stencil_func = ComparisonFunc::NOT_EQUAL;
				dsd.back_face.stencil_func = ComparisonFunc::NOT_EQUAL;
				depthStencilStates[STENCILMODE_NOT][i][d] = dsd;

				dsd.front_face.stencil_func = ComparisonFunc::ALWAYS;
				dsd.back_face.stencil_func = ComparisonFunc::ALWAYS;
				depthStencilStates[STENCILMODE_ALWAYS][i][d] = dsd;
			}
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
