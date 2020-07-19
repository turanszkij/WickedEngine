#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "SamplerMapping.h"
#include "ResourceMapping.h"
#include "wiScene.h"
#include "ShaderInterop_Image.h"
#include "wiBackLog.h"
#include "wiEvent.h"

#include <atomic>

using namespace std;
using namespace wiGraphics;

namespace wiImage
{

	enum IMAGE_SHADER
	{
		IMAGE_SHADER_STANDARD,
		IMAGE_SHADER_SEPARATENORMALMAP,
		IMAGE_SHADER_MASKED,
		IMAGE_SHADER_BACKGROUNDBLUR,
		IMAGE_SHADER_BACKGROUNDBLUR_MASKED,
		IMAGE_SHADER_FULLSCREEN,
		IMAGE_SHADER_COUNT
	};

	GPUBuffer				constantBuffer;
	Shader					vertexShader;
	Shader					screenVS;
	Shader					imagePS[IMAGE_SHADER_COUNT];
	BlendState				blendStates[BLENDMODE_COUNT];
	RasterizerState			rasterizerState;
	DepthStencilState		depthStencilStates[STENCILMODE_COUNT][STENCILREFMODE_COUNT];
	PipelineState			imagePSO[IMAGE_SHADER_COUNT][BLENDMODE_COUNT][STENCILMODE_COUNT][STENCILREFMODE_COUNT];

	DescriptorTable descriptortables_sampler[COMMANDLIST_COUNT];
	DescriptorTable descriptortables_texture[COMMANDLIST_COUNT];
	RootSignature rootsig;

	std::atomic_bool initialized{ false };


	void Draw(const Texture* texture, const wiImageParams& params, CommandList cmd)
	{
		if (!initialized.load())
		{
			return;
		}

		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin("Image", cmd);

		//device->BindResource(PS, texture, TEXSLOT_IMAGE_BASE, cmd);

		uint32_t stencilRef = params.stencilRef;
		if (params.stencilRefMode == STENCILREFMODE_USER)
		{
			stencilRef = wiRenderer::CombineStencilrefs(STENCILREF_EMPTY, (uint8_t)stencilRef);
		}
		device->BindStencilRef(stencilRef, cmd);

		device->BindRootSignatureGraphics(&rootsig, cmd);
		device->WriteDescriptorSRV(&descriptortables_texture[cmd], 0, texture, cmd);

		if (params.quality == QUALITY_NEAREST)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_POINT_MIRROR));
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_POINT_WRAP));
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_POINT_CLAMP));
		}
		else if (params.quality == QUALITY_LINEAR)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_LINEAR_MIRROR));
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_LINEAR_WRAP));
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_LINEAR_CLAMP));
		}
		else if (params.quality == QUALITY_ANISOTROPIC)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_ANISO_MIRROR));
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_ANISO_WRAP));
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				device->WriteDescriptorSampler(&descriptortables_sampler[cmd], 0, wiRenderer::GetSampler(SSLOT_ANISO_CLAMP));
		}

		ImageCB cb;
		cb.xColor = params.color;
		const float darken = 1 - params.fade;
		cb.xColor.x *= darken;
		cb.xColor.y *= darken;
		cb.xColor.z *= darken;
		cb.xColor.w *= params.opacity;

		if (params.isFullScreenEnabled())
		{
			device->BindPipelineState(&imagePSO[IMAGE_SHADER_FULLSCREEN][params.blendFlag][params.stencilComp][params.stencilRefMode], cmd);
			device->BindRootDescriptorTableGraphics(0, &descriptortables_sampler[cmd], cmd);
			device->BindRootDescriptorTableGraphics(1, &descriptortables_texture[cmd], cmd);
			GraphicsDevice::GPUAllocation cbmem = device->AllocateGPU(sizeof(ImageCB), cmd);
			memcpy(cbmem.data, &cb, sizeof(cb));
			device->BindRootCBVGraphics(2, cbmem.buffer, cbmem.offset, cmd);
			device->Draw(3, 0, cmd);
			device->EventEnd(cmd);
			return;
		}

		XMMATRIX M;
		if (params.typeFlag == SCREEN)
		{
			M =
				XMMatrixScaling(params.scale.x*params.siz.x, params.scale.y*params.siz.y, 1)
				* XMMatrixRotationZ(params.rotation)
				* XMMatrixTranslation(params.pos.x, params.pos.y, 0)
				* device->GetScreenProjection()
				;
		}
		else if (params.typeFlag == WORLD)
		{
			XMMATRIX faceRot = XMMatrixIdentity();
			if (params.lookAt.w)
			{
				XMVECTOR vvv = (params.lookAt.x == 1 && !params.lookAt.y && !params.lookAt.z) ? XMVectorSet(0, 1, 0, 0) : XMVectorSet(1, 0, 0, 0);
				faceRot =
					XMMatrixLookAtLH(XMVectorSet(0, 0, 0, 0)
						, XMLoadFloat4(&params.lookAt)
						, XMVector3Cross(
							vvv, XMLoadFloat4(&params.lookAt)
						)
					);
			}
			else
			{
				faceRot = XMLoadFloat3x3(&wiRenderer::GetCamera().rotationMatrix);
			}

			XMMATRIX view = wiRenderer::GetCamera().GetView();
			XMMATRIX projection = wiRenderer::GetCamera().GetProjection();
			// Remove possible jittering from temporal camera:
			projection.r[2] = XMVectorSetX(projection.r[2], 0);
			projection.r[2] = XMVectorSetY(projection.r[2], 0);

			M =
				XMMatrixScaling(params.scale.x*params.siz.x, -1 * params.scale.y*params.siz.y, 1)
				*XMMatrixRotationZ(params.rotation)
				*faceRot
				*XMMatrixTranslation(params.pos.x, params.pos.y, params.pos.z)
				*view * projection
				;
		}

		for (int i = 0; i < 4; ++i)
		{
			XMVECTOR V = XMVectorSet(params.corners[i].x - params.pivot.x, params.corners[i].y - params.pivot.y, 0, 1);
			V = XMVector2Transform(V, M);
			XMStoreFloat4(&cb.xCorners[i], V);
		}

		if (params.isMirrorEnabled())
		{
			std::swap(cb.xCorners[0], cb.xCorners[1]);
			std::swap(cb.xCorners[2], cb.xCorners[3]);
		}

		const TextureDesc& desc = texture->GetDesc();
		const float inv_width = 1.0f / float(desc.Width);
		const float inv_height = 1.0f / float(desc.Height);

		if (params.isDrawRectEnabled())
		{
			cb.xTexMulAdd.x = params.drawRect.z * inv_width;	// drawRec.width: mul
			cb.xTexMulAdd.y = params.drawRect.w * inv_height;	// drawRec.heigh: mul
			cb.xTexMulAdd.z = params.drawRect.x * inv_width;	// drawRec.x: add
			cb.xTexMulAdd.w = params.drawRect.y * inv_height;	// drawRec.y: add
		}
		else
		{
			cb.xTexMulAdd = XMFLOAT4(1, 1, 0, 0);	// disabled draw rect
		}
		cb.xTexMulAdd.z += params.texOffset.x * inv_width;	// texOffset.x: add
		cb.xTexMulAdd.w += params.texOffset.y * inv_height;	// texOffset.y: add

		if (params.isDrawRect2Enabled())
		{
			cb.xTexMulAdd2.x = params.drawRect2.z * inv_width;	// drawRec.width: mul
			cb.xTexMulAdd2.y = params.drawRect2.w * inv_height;	// drawRec.heigh: mul
			cb.xTexMulAdd2.z = params.drawRect2.x * inv_width;	// drawRec.x: add
			cb.xTexMulAdd2.w = params.drawRect2.y * inv_height;	// drawRec.y: add
		}
		else
		{
			cb.xTexMulAdd2 = XMFLOAT4(1, 1, 0, 0);	// disabled draw rect
		}
		cb.xTexMulAdd2.z += params.texOffset2.x * inv_width;	// texOffset.x: add
		cb.xTexMulAdd2.w += params.texOffset2.y * inv_height;	// texOffset.y: add

		device->UpdateBuffer(&constantBuffer, &cb, cmd);

		// Determine relevant image rendering pixel shader:
		IMAGE_SHADER targetShader;
		const bool NormalmapSeparate = params.isExtractNormalMapEnabled();
		const bool Mask = params.maskMap != nullptr;
		const bool background_blur = params.isBackgroundBlurEnabled();
		if (NormalmapSeparate)
		{
			targetShader = IMAGE_SHADER_SEPARATENORMALMAP;
		}
		else
		{
			if (Mask)
			{
				if (background_blur)
				{
					targetShader = IMAGE_SHADER_BACKGROUNDBLUR_MASKED;
				}
				else
				{
					targetShader = IMAGE_SHADER_MASKED;
				}
			}
			else
			{
				if (background_blur)
				{
					targetShader = IMAGE_SHADER_BACKGROUNDBLUR;
				}
				else
				{
					targetShader = IMAGE_SHADER_STANDARD;
				}
			}
		}

		device->BindPipelineState(&imagePSO[targetShader][params.blendFlag][params.stencilComp][params.stencilRefMode], cmd);

		device->WriteDescriptorSRV(&descriptortables_texture[cmd], 1, params.maskMap, cmd);

		device->BindRootDescriptorTableGraphics(0, &descriptortables_sampler[cmd], cmd);
		device->BindRootDescriptorTableGraphics(1, &descriptortables_texture[cmd], cmd);
		GraphicsDevice::GPUAllocation cbmem = device->AllocateGPU(sizeof(ImageCB), cmd);
		memcpy(cbmem.data, &cb, sizeof(cb));
		device->BindRootCBVGraphics(2, cbmem.buffer, cbmem.offset, cmd);

		device->Draw(4, 0, cmd);

		device->EventEnd(cmd);
	}
	void SetBackground(const Texture* texture, CommandList cmd)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		device->WriteDescriptorSRV(&descriptortables_texture[cmd], 2, texture, cmd);
	}


	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		wiRenderer::LoadShader(VS, vertexShader, "imageVS.cso");
		wiRenderer::LoadShader(VS, screenVS, "screenVS.cso");

		wiRenderer::LoadShader(PS, imagePS[IMAGE_SHADER_STANDARD], "imagePS.cso");
		wiRenderer::LoadShader(PS, imagePS[IMAGE_SHADER_SEPARATENORMALMAP], "imagePS_separatenormalmap.cso");
		wiRenderer::LoadShader(PS, imagePS[IMAGE_SHADER_MASKED], "imagePS_masked.cso");
		wiRenderer::LoadShader(PS, imagePS[IMAGE_SHADER_BACKGROUNDBLUR], "imagePS_backgroundblur.cso");
		wiRenderer::LoadShader(PS, imagePS[IMAGE_SHADER_BACKGROUNDBLUR_MASKED], "imagePS_backgroundblur_masked.cso");
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

		{
			GPUBufferDesc bd;
			bd.Usage = USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(ImageCB);
			bd.BindFlags = BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = CPU_ACCESS_WRITE;
			device->CreateBuffer(&bd, nullptr, &constantBuffer);
		}

		RasterizerStateDesc rs;
		rs.FillMode = FILL_SOLID;
		rs.CullMode = CULL_NONE;
		rs.FrontCounterClockwise = false;
		rs.DepthBias = 0;
		rs.DepthBiasClamp = 0;
		rs.SlopeScaledDepthBias = 0;
		rs.DepthClipEnable = false;
		rs.MultisampleEnable = false;
		rs.AntialiasedLineEnable = false;
		device->CreateRasterizerState(&rs, &rasterizerState);




		for (int i = 0; i < STENCILREFMODE_COUNT; ++i)
		{
			DepthStencilStateDesc dsd;
			dsd.DepthEnable = false;
			dsd.StencilEnable = false;
			device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_DISABLED][i]);

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
			device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_EQUAL][i]);

			dsd.FrontFace.StencilFunc = COMPARISON_LESS;
			dsd.BackFace.StencilFunc = COMPARISON_LESS;
			device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_LESS][i]);

			dsd.FrontFace.StencilFunc = COMPARISON_LESS_EQUAL;
			dsd.BackFace.StencilFunc = COMPARISON_LESS_EQUAL;
			device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_LESSEQUAL][i]);

			dsd.FrontFace.StencilFunc = COMPARISON_GREATER;
			dsd.BackFace.StencilFunc = COMPARISON_GREATER;
			device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_GREATER][i]);

			dsd.FrontFace.StencilFunc = COMPARISON_GREATER_EQUAL;
			dsd.BackFace.StencilFunc = COMPARISON_GREATER_EQUAL;
			device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_GREATEREQUAL][i]);

			dsd.FrontFace.StencilFunc = COMPARISON_NOT_EQUAL;
			dsd.BackFace.StencilFunc = COMPARISON_NOT_EQUAL;
			device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_NOT][i]);

			dsd.FrontFace.StencilFunc = COMPARISON_ALWAYS;
			dsd.BackFace.StencilFunc = COMPARISON_ALWAYS;
			device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_ALWAYS][i]);
		}


		BlendStateDesc bd;
		bd.RenderTarget[0].BlendEnable = true;
		bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		device->CreateBlendState(&bd, &blendStates[BLENDMODE_ALPHA]);

		bd.RenderTarget[0].BlendEnable = true;
		bd.RenderTarget[0].SrcBlend = BLEND_ONE;
		bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		device->CreateBlendState(&bd, &blendStates[BLENDMODE_PREMULTIPLIED]);

		bd.RenderTarget[0].BlendEnable = false;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		device->CreateBlendState(&bd, &blendStates[BLENDMODE_OPAQUE]);

		bd.RenderTarget[0].BlendEnable = true;
		bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = BLEND_ONE;
		bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
		bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
		bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
		bd.IndependentBlendEnable = false;
		device->CreateBlendState(&bd, &blendStates[BLENDMODE_ADDITIVE]);

		static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		for (CommandList cmd = 0; cmd < COMMANDLIST_COUNT; ++cmd)
		{
			descriptortables_sampler[cmd].ranges.emplace_back();
			descriptortables_sampler[cmd].ranges.back().binding = SAMPLER;
			descriptortables_sampler[cmd].ranges.back().slot = SSLOT_ONDEMAND0;
			descriptortables_sampler[cmd].ranges.back().count = 1;
			descriptortables_sampler[cmd].ranges.back().offset_from_table_start = 0;
			device->CreateDescriptorTable(&descriptortables_sampler[cmd]);

			descriptortables_texture[cmd].ranges.emplace_back();
			descriptortables_texture[cmd].ranges.back().binding = TEXTURE2D;
			descriptortables_texture[cmd].ranges.back().slot = TEXSLOT_IMAGE_BASE;
			descriptortables_texture[cmd].ranges.back().count = 1;
			descriptortables_texture[cmd].ranges.back().offset_from_table_start = 0;

			descriptortables_texture[cmd].ranges.emplace_back();
			descriptortables_texture[cmd].ranges.back().binding = TEXTURE2D;
			descriptortables_texture[cmd].ranges.back().slot = TEXSLOT_IMAGE_MASK;
			descriptortables_texture[cmd].ranges.back().count = 1;
			descriptortables_texture[cmd].ranges.back().offset_from_table_start = 1;

			descriptortables_texture[cmd].ranges.emplace_back();
			descriptortables_texture[cmd].ranges.back().binding = TEXTURE2D;
			descriptortables_texture[cmd].ranges.back().slot = TEXSLOT_IMAGE_BACKGROUND;
			descriptortables_texture[cmd].ranges.back().count = 1;
			descriptortables_texture[cmd].ranges.back().offset_from_table_start = 2;

			device->CreateDescriptorTable(&descriptortables_texture[cmd]);
		}

		{
			rootsig._flags |= RootSignature::FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			rootsig.parameters.emplace_back();
			rootsig.parameters.back().stage = PS;
			rootsig.parameters.back().range.binding = DESCRIPTORTABLE;
			rootsig.parameters.back().range.offset_from_table_start = 0;
			rootsig.parameters.back().table_template = &descriptortables_sampler[0];

			rootsig.parameters.emplace_back();
			rootsig.parameters.back().stage = PS;
			rootsig.parameters.back().range.binding = DESCRIPTORTABLE;
			rootsig.parameters.back().range.offset_from_table_start = 0;
			rootsig.parameters.back().table_template = &descriptortables_texture[0];

			rootsig.parameters.emplace_back();
			rootsig.parameters.back().stage = SHADERSTAGE_COUNT;
			rootsig.parameters.back().range.binding = CONSTANTBUFFER;
			rootsig.parameters.back().range.slot = CBSLOT_FONT;
			device->CreateRootSignature(&rootsig);
		}

		wiBackLog::post("wiImage Initialized");
		initialized.store(true);
	}

}
