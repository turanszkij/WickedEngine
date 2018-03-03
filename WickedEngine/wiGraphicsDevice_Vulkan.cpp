#include "wiGraphicsDevice_Vulkan.h"
#include "wiHelper.h"
#include "ResourceMapping.h"

#include <sstream>

using namespace std;



#ifdef WICKEDENGINE_BUILD_VULKAN
#pragma comment(lib,"vulkan-1.lib")

namespace wiGraphicsTypes
{
	// Engine functions
	GraphicsDevice_Vulkan::GraphicsDevice_Vulkan(wiWindowRegistration::window_type window, bool fullscreen) : GraphicsDevice()
	{
	}
	GraphicsDevice_Vulkan::~GraphicsDevice_Vulkan()
	{
	}

	void GraphicsDevice_Vulkan::SetResolution(int width, int height)
	{
		if (width != SCREENWIDTH || height != SCREENHEIGHT)
		{
			SCREENWIDTH = width;
			SCREENHEIGHT = height;
			//swapChain->ResizeBuffers(2, width, height, _ConvertFormat(GetBackBufferFormat()), 0);
			RESOLUTIONCHANGED = true;
		}
	}

	Texture2D GraphicsDevice_Vulkan::GetBackBuffer()
	{
		return Texture2D();
	}

	HRESULT GraphicsDevice_Vulkan::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateTexture3D(const Texture3DDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
		const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso)
	{
		return E_FAIL;
	}


	void GraphicsDevice_Vulkan::PresentBegin()
	{
	}
	void GraphicsDevice_Vulkan::PresentEnd()
	{
	}

	void GraphicsDevice_Vulkan::ExecuteDeferredContexts()
	{
	}
	void GraphicsDevice_Vulkan::FinishCommandList(GRAPHICSTHREAD threadID)
	{
	}


	void GraphicsDevice_Vulkan::BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindRenderTargetsUAVs(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GPUResource* const *ppUAVs, int slotUAV, int countUAV,
		GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindRenderTargets(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::ClearRenderTarget(Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindResource(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindResources(SHADERSTAGE stage, GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindUnorderedAccessResource(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindUnorderedAccessResources(SHADERSTAGE stage, GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindUnorderedAccessResourceCS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindUnorderedAccessResourcesCS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::UnBindResources(int slot, int num, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindSampler(SHADERSTAGE stage, Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindConstantBuffer(SHADERSTAGE stage, GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindVertexBuffers(GPUBuffer* const *vertexBuffers, int slot, int count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindIndexBuffer(GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindStencilRef(UINT value, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindBlendFactor(XMFLOAT4 value, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindGraphicsPSO(GraphicsPSO* pso, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindComputePSO(ComputePSO* pso, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::Draw(int vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawIndexed(int indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawInstanced(int vertexCount, int instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstanced(int indexCount, int instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DispatchIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::GenerateMips(Texture* texture, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::CopyTexture2D(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::CopyTexture2D_Region(Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::MSAAResolve(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize)
	{
	}
	void* GraphicsDevice_Vulkan::AllocateFromRingBuffer(GPURingBuffer* buffer, size_t dataSize, UINT& offsetIntoBuffer, GRAPHICSTHREAD threadID)
	{
		offsetIntoBuffer = 0;
		return nullptr;
	}
	void GraphicsDevice_Vulkan::InvalidateBufferAccess(GPUBuffer* buffer, GRAPHICSTHREAD threadID)
	{
	}
	bool GraphicsDevice_Vulkan::DownloadBuffer(GPUBuffer* bufferToDownload, GPUBuffer* bufferDest, void* dataDest, GRAPHICSTHREAD threadID)
	{
		return false;
	}
	void GraphicsDevice_Vulkan::SetScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID)
	{
	}

	void GraphicsDevice_Vulkan::QueryBegin(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::QueryEnd(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	bool GraphicsDevice_Vulkan::QueryRead(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
		return true;
	}

	void GraphicsDevice_Vulkan::UAVBarrier(GPUResource *const* uavs, UINT NumBarriers, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::TransitionBarrier(GPUResource *const* resources, UINT NumBarriers, RESOURCE_STATES stateBefore, RESOURCE_STATES stateAfter, GRAPHICSTHREAD threadID)
	{
	}


	HRESULT GraphicsDevice_Vulkan::CreateTextureFromFile(const std::string& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::SaveTexturePNG(const std::string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::SaveTextureDDS(const std::string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID)
	{
		return E_FAIL;
	}

	void GraphicsDevice_Vulkan::EventBegin(const std::string& name, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::EventEnd(GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::SetMarker(const std::string& name, GRAPHICSTHREAD threadID)
	{
	}

}

#endif // WICKEDENGINE_BUILD_VULKAN
