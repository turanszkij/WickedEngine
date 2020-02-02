#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDescriptors.h"

#include <memory>
#include <vector>

namespace wiGraphics
{
	class GraphicsDevice;

	struct GraphicsDeviceChild
	{
		std::shared_ptr<GraphicsDevice> device;
		inline void Register(std::shared_ptr<GraphicsDevice> dev) { device = dev; }
		inline bool IsValid() const { return device != nullptr; }
	};

	struct ShaderByteCode
	{
		uint8_t* data = nullptr;
		size_t size = 0;
		~ShaderByteCode() { SAFE_DELETE_ARRAY(data); }
	};

	struct Shader : public GraphicsDeviceChild
	{
		~Shader();

		SHADERSTAGE stage = SHADERSTAGE_COUNT;
		ShaderByteCode code;
		wiCPUHandle resource = WI_NULL_HANDLE;
	};

	struct Sampler : public GraphicsDeviceChild
	{
		wiCPUHandle resource = WI_NULL_HANDLE;
		SamplerDesc desc;

		~Sampler();

		const SamplerDesc& GetDesc() const { return desc; }
	};

	struct GPUResource : public GraphicsDeviceChild
	{
		enum class GPU_RESOURCE_TYPE
		{
			BUFFER,
			TEXTURE,
			UNKNOWN_TYPE,
		} type = GPU_RESOURCE_TYPE::UNKNOWN_TYPE;
		inline bool IsTexture() const { return type == GPU_RESOURCE_TYPE::TEXTURE; }
		inline bool IsBuffer() const { return type == GPU_RESOURCE_TYPE::BUFFER; }

		wiCPUHandle SRV = WI_NULL_HANDLE;
		std::vector<wiCPUHandle> subresourceSRVs;

		wiCPUHandle UAV = WI_NULL_HANDLE;
		std::vector<wiCPUHandle> subresourceUAVs;

		wiCPUHandle resource = WI_NULL_HANDLE;

		virtual ~GPUResource();
	};

	struct GPUBuffer : public GPUResource
	{
		wiCPUHandle CBV = WI_NULL_HANDLE;
		GPUBufferDesc desc;

		virtual ~GPUBuffer();

		const GPUBufferDesc& GetDesc() const { return desc; }
	};

	struct VertexLayout : public GraphicsDeviceChild
	{
		wiCPUHandle	resource = WI_NULL_HANDLE;

		std::vector<VertexLayoutDesc> desc;

		~VertexLayout();
	};

	struct BlendState : public GraphicsDeviceChild
	{
		wiCPUHandle resource = WI_NULL_HANDLE;
		BlendStateDesc desc;

		~BlendState();

		const BlendStateDesc& GetDesc() const { return desc; }
	};

	struct DepthStencilState : public GraphicsDeviceChild
	{
		wiCPUHandle resource = WI_NULL_HANDLE;
		DepthStencilStateDesc desc;

		~DepthStencilState();

		const DepthStencilStateDesc& GetDesc() const { return desc; }
	};

	struct RasterizerState : public GraphicsDeviceChild
	{
		wiCPUHandle resource = WI_NULL_HANDLE;
		RasterizerStateDesc desc;

		~RasterizerState();

		const RasterizerStateDesc& GetDesc() const { return desc; }
	};

	struct Texture : public GPUResource
	{
		TextureDesc	desc;
		wiCPUHandle	RTV = WI_NULL_HANDLE;
		std::vector<wiCPUHandle> subresourceRTVs;
		wiCPUHandle	DSV = WI_NULL_HANDLE;
		std::vector<wiCPUHandle> subresourceDSVs;

		~Texture();

		const TextureDesc& GetDesc() const { return desc; }
	};




	struct GPUQuery : public GraphicsDeviceChild
	{
		wiCPUHandle	resource = WI_NULL_HANDLE;
		GPUQueryDesc desc;

		~GPUQuery();

		const GPUQueryDesc& GetDesc() const { return desc; }
	};


	struct PipelineState : public GraphicsDeviceChild
	{
		size_t hash = 0;
		PipelineStateDesc desc;

		const PipelineStateDesc& GetDesc() const { return desc; }

		~PipelineState();
	};


	struct RenderPass : public GraphicsDeviceChild
	{
		size_t hash = 0;
		wiCPUHandle	framebuffer = WI_NULL_HANDLE;
		wiCPUHandle	renderpass = WI_NULL_HANDLE;
		RenderPassDesc desc;

		const RenderPassDesc& GetDesc() const { return desc; }

		~RenderPass();
	};

}
