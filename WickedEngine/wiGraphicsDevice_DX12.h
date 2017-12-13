#ifndef _GRAPHICSDEVICE_DX12_H_
#define _GRAPHICSDEVICE_DX12_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"

struct IDXGISwapChain3;
enum D3D_DRIVER_TYPE;
enum D3D_FEATURE_LEVEL;
enum D3D12_DESCRIPTOR_HEAP_TYPE;

struct ID3D12Device;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;
struct ID3D12DescriptorHeap;
struct ID3D12CommandQueue;
struct ID3D12RootSignature;

namespace wiGraphicsTypes
{

	class GraphicsDevice_DX12 : public GraphicsDevice
	{
	private:
		ID3D12Device*				device;
		ID3D12CommandQueue*			commandQueue;
		ID3D12Fence*				commandFences[GRAPHICSTHREAD_COUNT];
		HANDLE						commandFenceEvents[GRAPHICSTHREAD_COUNT];
		UINT64						commandFenceValues[GRAPHICSTHREAD_COUNT];

		ID3D12CommandQueue*			copyQueue;
		ID3D12CommandAllocator*		copyAllocator;
		ID3D12CommandList*			copyCommandList;
		ID3D12Fence*				copyFence;
		HANDLE						copyFenceEvent;
		UINT64						copyFenceValue;
		wiSpinLock					copyQueueLock;

		ID3D12RootSignature*		graphicsRootSig;
		ID3D12RootSignature*		computeRootSig;

		struct DescriptorAllocator
		{
			ID3D12DescriptorHeap*	heap;
			std::atomic<uint32_t>	itemCount;
			UINT					maxCount;
			UINT					itemSize;

			DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxCount);
			~DescriptorAllocator();

			size_t allocate();
		};
		DescriptorAllocator*		RTAllocator;
		DescriptorAllocator*		DSAllocator;
		DescriptorAllocator*		ResourceAllocator;
		DescriptorAllocator*		SamplerAllocator;


		struct FrameResources
		{
			ID3D12Resource*					backBuffer;
			D3D12_CPU_DESCRIPTOR_HANDLE*	backBufferRTV;
			ID3D12CommandAllocator*			commandAllocators[GRAPHICSTHREAD_COUNT];
			ID3D12CommandList*				commandLists[GRAPHICSTHREAD_COUNT];

			struct DescriptorTableFrameAllocator
			{
				ID3D12DescriptorHeap*	heap_CPU;
				ID3D12DescriptorHeap*	heap_GPU;
				UINT descriptorType;
				UINT itemSize;
				UINT itemCount;
				UINT ringOffset;
				bool dirty[SHADERSTAGE_COUNT];
				D3D12_CPU_DESCRIPTOR_HANDLE** boundDescriptors;

				DescriptorTableFrameAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxRenameCount);
				~DescriptorTableFrameAllocator();

				void reset(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE* nullDescriptorsSamplerCBVSRVUAV);
				void update(SHADERSTAGE stage, UINT slot, D3D12_CPU_DESCRIPTOR_HANDLE* descriptor, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
				void validate(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
			};
			DescriptorTableFrameAllocator*		ResourceDescriptorsGPU[GRAPHICSTHREAD_COUNT];
			DescriptorTableFrameAllocator*		SamplerDescriptorsGPU[GRAPHICSTHREAD_COUNT];

			struct ResourceFrameAllocator
			{
				ID3D12Resource*			resource;
				uint8_t*				dataBegin;
				uint8_t*				dataCur;
				uint8_t*				dataEnd;

				ResourceFrameAllocator(ID3D12Device* device, size_t size);
				~ResourceFrameAllocator();

				uint8_t* allocate(size_t dataSize, size_t alignment);
				void clear();
				uint64_t calculateOffset(uint8_t* address);
			};
			ResourceFrameAllocator* resourceBuffer[GRAPHICSTHREAD_COUNT];
		};
		FrameResources frames[BACKBUFFER_COUNT];
		FrameResources& GetFrameResources() { return frames[GetFrameCount() % BACKBUFFER_COUNT]; }
		ID3D12GraphicsCommandList* GetDirectCommandList(GRAPHICSTHREAD threadID);


		D3D12_CPU_DESCRIPTOR_HANDLE* nullSampler;
		D3D12_CPU_DESCRIPTOR_HANDLE* nullCBV;
		D3D12_CPU_DESCRIPTOR_HANDLE* nullSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE* nullUAV;

		struct UploadBuffer : wiThreadSafeManager
		{
			ID3D12Resource*			resource;
			uint8_t*				dataBegin;
			uint8_t*				dataCur;
			uint8_t*				dataEnd;

			UploadBuffer(ID3D12Device* device, size_t size);
			~UploadBuffer();

			uint8_t* allocate(size_t dataSize, size_t alignment);
			void clear();
			uint64_t calculateOffset(uint8_t* address);
		};
		UploadBuffer* bufferUploader;
		UploadBuffer* textureUploader;

		IDXGISwapChain3*			swapChain;
		ViewPort					viewPort;


	public:
		GraphicsDevice_DX12(wiWindowRegistration::window_type window, bool fullscreen = false);

		~GraphicsDevice_DX12();

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) override;
		virtual HRESULT CreateTexture1D(const Texture1DDesc* pDesc, const SubresourceData *pInitialData, Texture1D **ppTexture1D) override;
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) override;
		virtual HRESULT CreateTexture3D(const Texture3DDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D) override;
		virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
			const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout) override;
		virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader) override;
		virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader) override;
		virtual HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader) override;
		virtual HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader) override;
		virtual HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader) override;
		virtual HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader) override;
		virtual HRESULT CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState) override;
		virtual HRESULT CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState) override;
		virtual HRESULT CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState) override;
		virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) override;
		virtual HRESULT CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery) override;
		virtual HRESULT CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso) override;
		virtual HRESULT CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso) override;

		virtual void PresentBegin() override;
		virtual void PresentEnd() override;

		virtual void ExecuteDeferredContexts() override;
		virtual void FinishCommandList(GRAPHICSTHREAD thread) override;

		virtual void SetResolution(int width, int height) override;

		virtual Texture2D GetBackBuffer() override;

		///////////////Thread-sensitive////////////////////////

		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID) override;
		virtual void BindRenderTargetsUAVs(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GPUResource* const *ppUAVs, int slotUAV, int countUAV,
			GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindRenderTargets(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void ClearRenderTarget(Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourcePS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceVS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceGS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceDS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceHS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceCS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourcesPS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesVS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesGS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesDS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesHS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesCS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindUnorderedAccessResourceCS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindUnorderedAccessResourcesCS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void UnBindResources(int slot, int num, GRAPHICSTHREAD threadID) override;
		virtual void UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerPS(Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerVS(Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerGS(Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerHS(Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerDS(Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerCS(Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferPS(GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferVS(GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferGS(GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferDS(GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferHS(GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferCS(GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindVertexBuffers(GPUBuffer* const *vertexBuffers, int slot, int count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID) override;
		virtual void BindIndexBuffer(GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID) override;
		virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID) override;
		virtual void BindStencilRef(UINT value, GRAPHICSTHREAD threadID) override;
		virtual void BindBlendFactor(XMFLOAT4 value, GRAPHICSTHREAD threadID) override;
		virtual void BindGraphicsPSO(GraphicsPSO* pso, GRAPHICSTHREAD threadID) override;
		virtual void BindComputePSO(ComputePSO* pso, GRAPHICSTHREAD threadID) override;
		virtual void Draw(int vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID) override;
		virtual void DrawIndexed(int indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID) override;
		virtual void DrawInstanced(int vertexCount, int instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) override;
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) override;
		virtual void DrawInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		virtual void DrawIndexedInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		virtual void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID) override;
		virtual void DispatchIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID) override;
		virtual void CopyTexture2D(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID) override;
		virtual void CopyTexture2D_Region(Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID) override;
		virtual void MSAAResolve(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID) override;
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize = -1) override;
		virtual void* AllocateFromRingBuffer(GPURingBuffer* buffer, size_t dataSize, UINT& offsetIntoBuffer, GRAPHICSTHREAD threadID) override;
		virtual void InvalidateBufferAccess(GPUBuffer* buffer, GRAPHICSTHREAD threadID) override;
		virtual bool DownloadBuffer(GPUBuffer* bufferToDownload, GPUBuffer* bufferDest, void* dataDest, GRAPHICSTHREAD threadID) override;
		virtual void SetScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID) override;
		virtual void QueryBegin(GPUQuery *query, GRAPHICSTHREAD threadID) override;
		virtual void QueryEnd(GPUQuery *query, GRAPHICSTHREAD threadID) override;
		virtual bool QueryRead(GPUQuery *query, GRAPHICSTHREAD threadID) override;
		virtual void UAVBarrier(GPUResource *const* uavs, UINT NumBarriers, GRAPHICSTHREAD threadID) override;
		virtual void TransitionBarrier(GPUResource *const* resources, UINT NumBarriers, RESOURCE_STATES stateBefore, RESOURCE_STATES stateAfter, GRAPHICSTHREAD threadID) override;

		virtual HRESULT CreateTextureFromFile(const std::string& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID) override;
		virtual HRESULT SaveTexturePNG(const std::string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID) override;
		virtual HRESULT SaveTextureDDS(const std::string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID) override;

		virtual void EventBegin(const std::string& name, GRAPHICSTHREAD threadID) override;
		virtual void EventEnd(GRAPHICSTHREAD threadID) override;
		virtual void SetMarker(const std::string& name, GRAPHICSTHREAD threadID) override;

	private:
		void BindResource(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1);
		void BindSampler(SHADERSTAGE stage, Sampler* sampler, int slot, GRAPHICSTHREAD threadID);
		void BindConstantBuffer(SHADERSTAGE stage, GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID);
	};

}

#endif // _GRAPHICSDEVICE_DX12_H_
