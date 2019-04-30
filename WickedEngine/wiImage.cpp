#include "wiImage.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "SamplerMapping.h"
#include "ResourceMapping.h"
#include "wiSceneSystem.h"
#include "ShaderInterop_Image.h"
#include "wiBackLog.h"

#include <atomic>

using namespace std;
using namespace wiGraphics;

namespace wiImage
{

	enum IMAGE_SHADER
	{
		IMAGE_SHADER_STANDARD,
		IMAGE_SHADER_SEPARATENORMALMAP,
		IMAGE_SHADER_DISTORTION,
		IMAGE_SHADER_DISTORTION_MASKED,
		IMAGE_SHADER_MASKED,
		IMAGE_SHADER_FULLSCREEN,
		IMAGE_SHADER_COUNT
	};
	enum IMAGE_HDR
	{
		IMAGE_HDR_DISABLED,
		IMAGE_HDR_ENABLED,
		IMAGE_HDR_COUNT
	};
	enum IMAGE_SAMPLING
	{
		IMAGE_SAMPLING_SIMPLE,
		IMAGE_SAMPLING_BICUBIC,
		IMAGE_SAMPLING_COUNT,
	};

	GPUBuffer				constantBuffer;
	GPUBuffer				processCb;
	const VertexShader*		vertexShader = nullptr;
	const VertexShader*		screenVS = nullptr;
	const PixelShader*		imagePS[IMAGE_SHADER_COUNT][IMAGE_SAMPLING_COUNT];
	const PixelShader*		postprocessPS[wiImageParams::PostProcess::POSTPROCESS_COUNT];
	const PixelShader*		deferredPS = nullptr;
	BlendState				blendStates[BLENDMODE_COUNT];
	RasterizerState			rasterizerState;
	DepthStencilState		depthStencilStates[STENCILMODE_COUNT];
	BlendState				blendStateDisableColor;
	DepthStencilState		depthStencilStateDepthWrite;
	GraphicsPSO				imagePSO[IMAGE_SHADER_COUNT][BLENDMODE_COUNT][STENCILMODE_COUNT][IMAGE_HDR_COUNT][IMAGE_SAMPLING_COUNT];
	GraphicsPSO				postprocessPSO[wiImageParams::PostProcess::POSTPROCESS_COUNT];
	GraphicsPSO				deferredPSO;

	std::atomic_bool initialized = false;


	void Draw(const Texture2D* texture, const wiImageParams& params, GRAPHICSTHREAD threadID)
	{
		if (!initialized.load())
		{
			return;
		}

		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin("Image", threadID);

		bool fullScreenEffect = false;

		device->BindResource(PS, texture, TEXSLOT_ONDEMAND0, threadID);

		device->BindStencilRef(params.stencilRef, threadID);

		IMAGE_SAMPLING sampling_type = params.quality == QUALITY_BICUBIC ? IMAGE_SAMPLING_BICUBIC : IMAGE_SAMPLING_SIMPLE;

		if (params.quality == QUALITY_NEAREST)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_POINT_MIRROR), SSLOT_ONDEMAND0, threadID);
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_POINT_WRAP), SSLOT_ONDEMAND0, threadID);
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_POINT_CLAMP), SSLOT_ONDEMAND0, threadID);
		}
		else if (params.quality == QUALITY_LINEAR)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_LINEAR_MIRROR), SSLOT_ONDEMAND0, threadID);
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_LINEAR_WRAP), SSLOT_ONDEMAND0, threadID);
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_LINEAR_CLAMP), SSLOT_ONDEMAND0, threadID);
		}
		else if (params.quality == QUALITY_ANISOTROPIC)
		{
			if (params.sampleFlag == SAMPLEMODE_MIRROR)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_ANISO_MIRROR), SSLOT_ONDEMAND0, threadID);
			else if (params.sampleFlag == SAMPLEMODE_WRAP)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_ANISO_WRAP), SSLOT_ONDEMAND0, threadID);
			else if (params.sampleFlag == SAMPLEMODE_CLAMP)
				device->BindSampler(PS, wiRenderer::GetSampler(SSLOT_ANISO_CLAMP), SSLOT_ONDEMAND0, threadID);
		}

		if (params.isFullScreenEnabled())
		{
			device->BindGraphicsPSO(&imagePSO[IMAGE_SHADER_FULLSCREEN][params.blendFlag][params.stencilComp][params.isHDREnabled()][sampling_type], threadID);
			device->Draw(3, 0, threadID);
			device->EventEnd(threadID);
			return;
		}


		if (!params.process.isActive()) // not post process, just regular image
		{

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

			ImageCB cb;
			
			for (int i = 0; i < 4; ++i)
			{
				XMVECTOR V = XMVectorSet(params.corners[i].x - params.pivot.x, params.corners[i].y - params.pivot.y, 0, 1);
				V = XMVector2Transform(V, M);
				XMStoreFloat4(&cb.xCorners[i], V);
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

			cb.xColor = params.col;
			const float darken = 1 - params.fade;
			cb.xColor.x *= darken;
			cb.xColor.y *= darken;
			cb.xColor.z *= darken;
			cb.xColor.w *= params.opacity;
			cb.xMirror = params.isMirrorEnabled() ? 1 : 0;
			cb.xMipLevel = params.mipLevel;

			device->UpdateBuffer(&constantBuffer, &cb, threadID);

			// Determine relevant image rendering pixel shader:
			IMAGE_SHADER targetShader;
			bool NormalmapSeparate = params.isExtractNormalMapEnabled();
			bool Mask = params.maskMap != nullptr;
			bool Distort = params.distortionMap != nullptr;
			if (NormalmapSeparate)
			{
				targetShader = IMAGE_SHADER_SEPARATENORMALMAP;
			}
			else
			{
				if (Mask)
				{
					if (Distort)
					{
						targetShader = IMAGE_SHADER_DISTORTION_MASKED;
					}
					else
					{
						targetShader = IMAGE_SHADER_MASKED;
					}
				}
				else if (Distort)
				{
					targetShader = IMAGE_SHADER_DISTORTION;
				}
				else
				{
					targetShader = IMAGE_SHADER_STANDARD;
				}
			}

			device->BindGraphicsPSO(&imagePSO[targetShader][params.blendFlag][params.stencilComp][params.isHDREnabled()][sampling_type], threadID);

			device->BindConstantBuffer(VS, &constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);
			device->BindConstantBuffer(PS, &constantBuffer, CB_GETBINDSLOT(ImageCB), threadID);

			fullScreenEffect = false;
		}
		else // Post process
		{
			fullScreenEffect = true;

			device->BindGraphicsPSO(&postprocessPSO[params.process.type], threadID);

			PostProcessCB prcb;

			switch (params.process.type)
			{
			case wiImageParams::PostProcess::BLUR_LDR:
			case wiImageParams::PostProcess::BLUR_HDR:
				prcb.xPPParams0.x = params.process.params.blur.x / params.siz.x;
				prcb.xPPParams0.y = params.process.params.blur.y / params.siz.y;
				prcb.xPPParams0.z = params.mipLevel;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::LIGHTSHAFT:
				prcb.xPPParams0.x = 0.65f;	// density
				prcb.xPPParams0.y = 0.25f;	// weight
				prcb.xPPParams0.z = 0.945f;	// decay
				prcb.xPPParams0.w = 0.2f;	// exposure
				prcb.xPPParams1.x = params.process.params.sun.x;
				prcb.xPPParams1.y = params.process.params.sun.y;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::OUTLINE:
				prcb.xPPParams0.x = params.process.params.outline.threshold;
				prcb.xPPParams0.y = params.process.params.outline.thickness;
				prcb.xPPParams1.x = params.process.params.outline.colorR;
				prcb.xPPParams1.y = params.process.params.outline.colorG;
				prcb.xPPParams1.z = params.process.params.outline.colorB;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::DEPTHOFFIELD:
				prcb.xPPParams0.z = params.process.params.dofFocus;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::MOTIONBLUR:
				break;
			case wiImageParams::PostProcess::BLOOMSEPARATE:
				prcb.xPPParams0.x = params.process.params.bloomThreshold;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::FXAA:
				break;
			case wiImageParams::PostProcess::SSAO:
				prcb.xPPParams0.x = params.process.params.ssao.range;
				prcb.xPPParams0.y = (float)params.process.params.ssao.sampleCount;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::SSSS:
				prcb.xPPParams0.x = params.process.params.ssss.x;
				prcb.xPPParams0.y = params.process.params.ssss.y;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::SSR:
				break;
			case wiImageParams::PostProcess::COLORGRADE:
				break;
			case wiImageParams::PostProcess::TONEMAP:
				prcb.xPPParams0.x = params.process.params.exposure;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::REPROJECTDEPTHBUFFER:
				break;
			case wiImageParams::PostProcess::DOWNSAMPLEDEPTHBUFFER:
				break;
			case wiImageParams::PostProcess::TEMPORALAA:
				break;
			case wiImageParams::PostProcess::SHARPEN:
				prcb.xPPParams0.x = params.process.params.sharpen;
				device->UpdateBuffer(&processCb, &prcb, threadID);
				break;
			case wiImageParams::PostProcess::LINEARDEPTH:
				break;
			default:
				assert(0); // shouldn't reach here
				break;
			}

			device->BindConstantBuffer(PS, &processCb, CB_GETBINDSLOT(PostProcessCB), threadID);

		}

		const GPUResource* res[] = {
			params.maskMap,
			params.distortionMap,
			params.refractionSource,
		};
		device->BindResources(PS, res, TEXSLOT_ONDEMAND1, ARRAYSIZE(res), threadID);

		device->Draw((fullScreenEffect ? 3 : 4), 0, threadID);

		device->EventEnd(threadID);
	}

	void DrawDeferred(
		const Texture2D* lightmap_diffuse, 
		const Texture2D* lightmap_specular, 
		const Texture2D* ao,
		GRAPHICSTHREAD threadID, int stencilRef)
	{
		if (!initialized.load())
		{
			return;
		}

		GraphicsDevice* device = wiRenderer::GetDevice();

		device->EventBegin("DeferredComposition", threadID);

		device->BindStencilRef(stencilRef, threadID);

		device->BindResource(PS, lightmap_diffuse, TEXSLOT_ONDEMAND0, threadID);
		device->BindResource(PS, lightmap_specular, TEXSLOT_ONDEMAND1, threadID);
		device->BindResource(PS, ao, TEXSLOT_ONDEMAND2, threadID);

		device->BindGraphicsPSO(&deferredPSO, threadID);

		device->Draw(3, 0, threadID);

		device->EventEnd(threadID);
	}


	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		vertexShader = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(path + "imageVS.cso", wiResourceManager::VERTEXSHADER));
		screenVS = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(path + "screenVS.cso", wiResourceManager::VERTEXSHADER));

		imagePS[IMAGE_SHADER_STANDARD][IMAGE_SAMPLING_SIMPLE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_SEPARATENORMALMAP][IMAGE_SAMPLING_SIMPLE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_separatenormalmap.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_DISTORTION][IMAGE_SAMPLING_SIMPLE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_distortion.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_DISTORTION_MASKED][IMAGE_SAMPLING_SIMPLE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_distortion_masked.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_MASKED][IMAGE_SAMPLING_SIMPLE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_masked.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_FULLSCREEN][IMAGE_SAMPLING_SIMPLE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "screenPS.cso", wiResourceManager::PIXELSHADER));

		imagePS[IMAGE_SHADER_STANDARD][IMAGE_SAMPLING_BICUBIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_bicubic.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_SEPARATENORMALMAP][IMAGE_SAMPLING_BICUBIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_separatenormalmap_bicubic.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_DISTORTION][IMAGE_SAMPLING_BICUBIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_distortion_bicubic.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_DISTORTION_MASKED][IMAGE_SAMPLING_BICUBIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_distortion_masked_bicubic.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_MASKED][IMAGE_SAMPLING_BICUBIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "imagePS_masked_bicubic.cso", wiResourceManager::PIXELSHADER));
		imagePS[IMAGE_SHADER_FULLSCREEN][IMAGE_SAMPLING_BICUBIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "screenPS_bicubic.cso", wiResourceManager::PIXELSHADER));

		postprocessPS[wiImageParams::PostProcess::BLUR_LDR] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "blurPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::BLUR_HDR] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "blurPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::LIGHTSHAFT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "lightShaftPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::OUTLINE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "outlinePS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::DEPTHOFFIELD] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "depthofFieldPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::MOTIONBLUR] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "motionBlurPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::BLOOMSEPARATE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "bloomSeparatePS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::FXAA] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "fxaa.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::SSAO] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "ssao.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::SSSS] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "ssss.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::LINEARDEPTH] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "linDepthPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::COLORGRADE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "colorGradePS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::SSR] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "ssr.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::TONEMAP] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "toneMapPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::REPROJECTDEPTHBUFFER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "reprojectDepthBufferPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::DOWNSAMPLEDEPTHBUFFER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "downsampleDepthBuffer4xPS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::TEMPORALAA] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "temporalAAResolvePS.cso", wiResourceManager::PIXELSHADER));
		postprocessPS[wiImageParams::PostProcess::SHARPEN] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "sharpenPS.cso", wiResourceManager::PIXELSHADER));

		deferredPS = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "deferredPS.cso", wiResourceManager::PIXELSHADER));


		GraphicsDevice* device = wiRenderer::GetDevice();

		for (int i = 0; i < IMAGE_SHADER_COUNT; ++i)
		{
			GraphicsPSODesc desc;
			desc.vs = vertexShader;
			if (i == IMAGE_SHADER_FULLSCREEN)
			{
				desc.vs = screenVS;
			}
			desc.rs = &rasterizerState;
			desc.pt = TRIANGLESTRIP;

			for (int l = 0; l < IMAGE_SAMPLING_COUNT; ++l)
			{
				desc.ps = imagePS[i][l];

				for (int j = 0; j < BLENDMODE_COUNT; ++j)
				{
					desc.bs = &blendStates[j];
					for (int k = 0; k < STENCILMODE_COUNT; ++k)
					{
						desc.dss = &depthStencilStates[k];

						if (k == STENCILMODE_DISABLED)
						{
							desc.DSFormat = FORMAT_UNKNOWN;
						}
						else
						{
							desc.DSFormat = wiRenderer::DSFormat_full;
						}

						desc.numRTs = 1;

						desc.RTFormats[0] = device->GetBackBufferFormat();
						device->CreateGraphicsPSO(&desc, &imagePSO[i][j][k][0][l]);

						desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
						device->CreateGraphicsPSO(&desc, &imagePSO[i][j][k][1][l]);

					}
				}
			}
		}

		for (int i = 0; i < wiImageParams::PostProcess::POSTPROCESS_COUNT; ++i)
		{
			GraphicsPSODesc desc;
			desc.vs = screenVS;
			desc.ps = postprocessPS[i];
			desc.bs = &blendStates[BLENDMODE_OPAQUE];
			desc.dss = &depthStencilStates[STENCILMODE_DISABLED];
			desc.rs = &rasterizerState;
			desc.pt = TRIANGLELIST;

			if (i == wiImageParams::PostProcess::DOWNSAMPLEDEPTHBUFFER || i == wiImageParams::PostProcess::REPROJECTDEPTHBUFFER)
			{
				desc.dss = &depthStencilStateDepthWrite;
				desc.DSFormat = wiRenderer::DSFormat_small;
				desc.numRTs = 0;
			}
			else if (i == wiImageParams::PostProcess::SSSS)
			{
				desc.dss = &depthStencilStates[STENCILMODE_EQUAL];
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_deferred_lightbuffer;
				desc.DSFormat = wiRenderer::DSFormat_full;
			}
			else if (i == wiImageParams::PostProcess::SSAO)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_ssao;
			}
			else if (i == wiImageParams::PostProcess::LINEARDEPTH)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_lineardepth;
			}
			else if (i == wiImageParams::PostProcess::TONEMAP)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = device->GetBackBufferFormat();
			}
			else if (i == wiImageParams::PostProcess::OUTLINE)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
				desc.bs = &blendStates[BLENDMODE_ALPHA];
			}
			else if (i == wiImageParams::PostProcess::BLOOMSEPARATE)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = device->GetBackBufferFormat();
			}
			else if (i == wiImageParams::PostProcess::BLUR_LDR)
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = device->GetBackBufferFormat();
			}
			else
			{
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
			}

			device->CreateGraphicsPSO(&desc, &postprocessPSO[i]);
		}

		GraphicsPSODesc desc;
		desc.vs = screenVS;
		desc.ps = deferredPS;
		desc.bs = &blendStates[BLENDMODE_OPAQUE];
		desc.dss = &depthStencilStates[STENCILMODE_LESSEQUAL];
		desc.rs = &rasterizerState;
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
		desc.DSFormat = wiRenderer::DSFormat_full;
		desc.pt = TRIANGLELIST;
		device->CreateGraphicsPSO(&desc, &deferredPSO);


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
			HRESULT hr = device->CreateBuffer(&bd, nullptr, &constantBuffer);
			assert(SUCCEEDED(hr));
		}

		{
			GPUBufferDesc bd;
			bd.Usage = USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(PostProcessCB);
			bd.BindFlags = BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = CPU_ACCESS_WRITE;
			HRESULT hr = device->CreateBuffer(&bd, nullptr, &processCb);
			assert(SUCCEEDED(hr));
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





		DepthStencilStateDesc dsd;
		dsd.DepthEnable = false;
		dsd.StencilEnable = false;
		device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_DISABLED]);

		dsd.StencilEnable = true;
		dsd.StencilReadMask = 0xff;
		dsd.StencilWriteMask = 0;
		dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
		dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
		dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
		dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
		dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
		dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;

		dsd.FrontFace.StencilFunc = COMPARISON_EQUAL;
		dsd.BackFace.StencilFunc = COMPARISON_EQUAL;
		device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_EQUAL]);

		dsd.FrontFace.StencilFunc = COMPARISON_LESS;
		dsd.BackFace.StencilFunc = COMPARISON_LESS;
		device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_LESS]);

		dsd.FrontFace.StencilFunc = COMPARISON_LESS_EQUAL;
		dsd.BackFace.StencilFunc = COMPARISON_LESS_EQUAL;
		device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_LESSEQUAL]);

		dsd.FrontFace.StencilFunc = COMPARISON_GREATER;
		dsd.BackFace.StencilFunc = COMPARISON_GREATER;
		device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_GREATER]);

		dsd.FrontFace.StencilFunc = COMPARISON_GREATER_EQUAL;
		dsd.BackFace.StencilFunc = COMPARISON_GREATER_EQUAL;
		device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_GREATEREQUAL]);

		dsd.FrontFace.StencilFunc = COMPARISON_NOT_EQUAL;
		dsd.BackFace.StencilFunc = COMPARISON_NOT_EQUAL;
		device->CreateDepthStencilState(&dsd, &depthStencilStates[STENCILMODE_NOT]);


		dsd.DepthEnable = true;
		dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
		dsd.DepthFunc = COMPARISON_ALWAYS;
		dsd.StencilEnable = false;
		device->CreateDepthStencilState(&dsd, &depthStencilStateDepthWrite);


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

		bd.RenderTarget[0].BlendEnable = false;
		bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_DISABLE;
		bd.IndependentBlendEnable = false;
		device->CreateBlendState(&bd, &blendStateDisableColor);

		LoadShaders();

		wiBackLog::post("wiImage Initialized");
		initialized.store(true);
	}

}
