#ifndef _GRAPHICSRESOURCE_H_
#define _GRAPHICSRESOURCE_H_

#include "CommonInclude.h"
#include "wiGraphicsDescriptors.h"

#include <vector>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11DomainShader;
struct ID3D11HullShader;
struct ID3D11ComputeShader;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11Buffer;
struct ID3D11InputLayout;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11RasterizerState2;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Texture1D;
struct ID3D11Texture2D;
struct ID3D11Texture3D;
struct ID3D11Query;
struct ID3D11Predicate;

struct ID3D12Resource;
struct ID3D12PipelineState;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

#ifdef _WIN64
typedef void* wiHandle;
#define WI_NULL_HANDLE nullptr
#else
typedef uint64_t wiHandle;
#define WI_NULL_HANDLE 0
#endif


namespace wiGraphicsTypes
{
	struct ShaderByteCode
	{
		BYTE* data;
		size_t size;
		ShaderByteCode() :data(nullptr), size(0) {}
		~ShaderByteCode() { SAFE_DELETE_ARRAY(data); }
	};

	class VertexShader
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11VertexShader*		resource_DX11;
		ShaderByteCode			code;
	public:
		VertexShader();
		~VertexShader();
	};

	class PixelShader
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11PixelShader*		resource_DX11;
		ShaderByteCode			code;
	public:
		PixelShader();
		~PixelShader();
	};

	class GeometryShader
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11GeometryShader*	resource_DX11;
		ShaderByteCode			code;
	public:
		GeometryShader();
		~GeometryShader();
	};

	class HullShader
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11HullShader*		resource_DX11;
		ShaderByteCode			code;
	public:
		HullShader();
		~HullShader();
	};

	class DomainShader
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11DomainShader*		resource_DX11;
		ShaderByteCode			code;
	public:
		DomainShader();
		~DomainShader();
	};

	class ComputeShader
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11ComputeShader*	resource_DX11;
		ShaderByteCode			code;
	public:
		ComputeShader();
		~ComputeShader();
	};

	class Sampler
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11SamplerState*				resource_DX11;
		D3D12_CPU_DESCRIPTOR_HANDLE*	resource_DX12;
		wiHandle						resource_Vulkan;
		SamplerDesc desc;
	public:
		Sampler();
		~Sampler();

		bool IsValid() { return resource_DX11 != nullptr || resource_DX12 != nullptr || resource_Vulkan != WI_NULL_HANDLE; }
		SamplerDesc GetDesc() { return desc; }
	};

	class GPUResource
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	protected:
		ID3D11ShaderResourceView*					SRV_DX11;					// main resource SRV
		std::vector<ID3D11ShaderResourceView*>		additionalSRVs_DX11;		// can be used for sub-resources if requested
		D3D12_CPU_DESCRIPTOR_HANDLE*				SRV_DX12;					// main resource SRV
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE*>	additionalSRVs_DX12;		// can be used for sub-resources if requested
		wiHandle									SRV_Vulkan;					// main resource SRV
		std::vector<wiHandle>						additionalSRVs_Vulkan;		// can be used for sub-resources if requested

		ID3D11UnorderedAccessView*					UAV_DX11;					// main resource UAV
		std::vector<ID3D11UnorderedAccessView*>		additionalUAVs_DX11;		// can be used for sub-resources if requested
		D3D12_CPU_DESCRIPTOR_HANDLE*				UAV_DX12;					// main resource UAV
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE*>	additionalUAVs_DX12;		// can be used for sub-resources if requested
		wiHandle									UAV_Vulkan;					// main resource UAV
		std::vector<wiHandle>						additionalUAVs_Vulkan;		// can be used for sub-resources if requested

		ID3D12Resource*								resource_DX12;
		wiHandle									resource_Vulkan;
		wiHandle									resourceMemory_Vulkan;

		GPUResource();
		virtual ~GPUResource();
	};

	class GPUBuffer : public GPUResource
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11Buffer*								resource_DX11;
		D3D12_CPU_DESCRIPTOR_HANDLE*				CBV_DX12;
		GPUBufferDesc desc;
	public:
		GPUBuffer();
		virtual ~GPUBuffer();

		bool IsValid() { return resource_DX11 != nullptr || resource_DX12 != nullptr || resource_Vulkan != WI_NULL_HANDLE; }
		GPUBufferDesc GetDesc() { return desc; }
	};

	class GPURingBuffer : public GPUBuffer
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		size_t byteOffset;
		uint64_t residentFrame;
	public:
		GPURingBuffer() : byteOffset(0), residentFrame(0) {}
		virtual ~GPURingBuffer() {}

		// The next appending to buffer will start at this offset
		size_t GetByteOffset() { return byteOffset; }
	};

	class VertexLayout
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11InputLayout*	resource_DX11;

		std::vector<VertexLayoutDesc> desc;
	public:
		VertexLayout();
		~VertexLayout();
	};

	class BlendState
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11BlendState*	resource_DX11;
		BlendStateDesc desc;
	public:
		BlendState();
		~BlendState();

		BlendStateDesc GetDesc() { return desc; }
	};

	class DepthStencilState
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11DepthStencilState*	resource_DX11;
		DepthStencilStateDesc desc;
	public:
		DepthStencilState();
		~DepthStencilState();

		DepthStencilStateDesc GetDesc() { return desc; }
	};

	class RasterizerState
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11RasterizerState*	resource_DX11;
		RasterizerStateDesc desc;
	public:
		RasterizerState();
		~RasterizerState();

		RasterizerStateDesc GetDesc() { return desc; }
	};

	struct VertexShaderInfo 
	{
		VertexShader* vertexShader;
		VertexLayout* vertexLayout;

		VertexShaderInfo();
		~VertexShaderInfo();
	};

	class Texture : public GPUResource
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		TextureDesc									desc;
		ID3D11RenderTargetView*						RTV_DX11;
		std::vector<ID3D11RenderTargetView*>		additionalRTVs_DX11;
		D3D12_CPU_DESCRIPTOR_HANDLE*				RTV_DX12;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE*>	additionalRTVs_DX12;
		wiHandle									RTV_Vulkan;
		std::vector<wiHandle>						additionalRTVs_Vulkan;
		bool										independentRTVArraySlices;
		bool										independentRTVCubemapFaces;
		bool										independentSRVArraySlices;
		bool										independentSRVMIPs;
		bool										independentUAVMIPs;
	public:
		const TextureDesc& GetDesc() const { return desc; }

		Texture();
		virtual ~Texture();

		// if true, then each array slice will get a unique rendertarget
		void RequestIndependentRenderTargetArraySlices(bool value);
		// if true, then each face of the cubemap will get a unique rendertarget
		void RequestIndependentRenderTargetCubemapFaces(bool value);
		// if true, then each array slice will get a unique shader resource
		void RequestIndependentShaderResourceArraySlices(bool value);
		// if true, then each miplevel will get unique shader resource
		void RequestIndependentShaderResourcesForMIPs(bool value);
		// if true, then each miplevel will get unique unordered access resource
		void RequestIndependentUnorderedAccessResourcesForMIPs(bool value);
	};

	class Texture1D : public Texture
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11Texture1D*			texture1D_DX11;
	public:
		Texture1D();
		virtual ~Texture1D();
	};

	class Texture2D : public Texture
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11DepthStencilView*						DSV_DX11;
		std::vector<ID3D11DepthStencilView*>		additionalDSVs_DX11;
		D3D12_CPU_DESCRIPTOR_HANDLE*				DSV_DX12;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE*>	additionalDSVs_DX12;
		wiHandle									DSV_Vulkan;
		std::vector<wiHandle>						additionalDSVs_Vulkan;

		ID3D11Texture2D*							texture2D_DX11;
	public:
		Texture2D();
		virtual ~Texture2D();
	};

	class Texture3D : public Texture
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D11Texture3D*			texture3D_DX11;
	public:
		Texture3D();
		virtual ~Texture3D();
	};




	class GPUQuery
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		std::vector<ID3D11Query*>	resource_DX11;
		std::vector<int>			active;
		GPUQueryDesc				desc;
		int							async_frameshift;
	public:
		GPUQuery();
		virtual ~GPUQuery();

		bool IsValid() { return !resource_DX11.empty() && resource_DX11[0] != nullptr; }
		GPUQueryDesc GetDesc() const { return desc; }

		BOOL	result_passed;
		UINT64	result_passed_sample_count;
		UINT64	result_timestamp;
		UINT64	result_timestamp_frequency;
		BOOL	result_disjoint;
	};


	class GraphicsPSO
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D12PipelineState*			resource_DX12;
		wiHandle						renderPass_Vulkan;
		wiHandle						pipeline_Vulkan;
		GraphicsPSODesc desc;

	public:
		const GraphicsPSODesc& GetDesc() const { return desc; }

		GraphicsPSO();
		~GraphicsPSO();
	};
	class ComputePSO
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		ID3D12PipelineState*			resource_DX12;
		wiHandle						pipeline_Vulkan;
		ComputePSODesc desc;

	public:
		const ComputePSODesc& GetDesc() const { return desc; }

		ComputePSO();
		~ComputePSO();
	};

	class RenderPass
	{
		friend class GraphicsDevice_DX11;
		friend class GraphicsDevice_DX12;
		friend class GraphicsDevice_Vulkan;
	private:
		RenderPassDesc desc;

	public:
		const RenderPassDesc& GetDesc() const { return desc; }

		RenderPass();
		~RenderPass();

	};

}

#endif // _GRAPHICSRESOURCE_H_
