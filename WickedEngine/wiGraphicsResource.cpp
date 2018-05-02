#include "wiGraphicsResource.h"
#include "Include_DX11.h"
#include "Include_DX12.h"
#include "Include_Vulkan.h"

namespace wiGraphicsTypes
{
	VertexShader::VertexShader()
	{
		SAFE_INIT(resource_DX11);
	}
	VertexShader::~VertexShader()
	{
		SAFE_RELEASE(resource_DX11);
	}

	PixelShader::PixelShader()
	{
		SAFE_INIT(resource_DX11);
	}
	PixelShader::~PixelShader()
	{
		SAFE_RELEASE(resource_DX11);
	}

	GeometryShader::GeometryShader()
	{
		SAFE_INIT(resource_DX11);
	}
	GeometryShader::~GeometryShader()
	{
		SAFE_RELEASE(resource_DX11);
	}

	DomainShader::DomainShader()
	{
		SAFE_INIT(resource_DX11);
	}
	DomainShader::~DomainShader()
	{
		SAFE_RELEASE(resource_DX11);
	}

	HullShader::HullShader()
	{
		SAFE_INIT(resource_DX11);
	}
	HullShader::~HullShader()
	{
		SAFE_RELEASE(resource_DX11);
	}

	ComputeShader::ComputeShader()
	{
		SAFE_INIT(resource_DX11);
	}
	ComputeShader::~ComputeShader()
	{
		SAFE_RELEASE(resource_DX11);
	}

	Sampler::Sampler()
	{
		SAFE_INIT(resource_DX11);
		SAFE_INIT(resource_DX12);
		resource_Vulkan = WI_NULL_HANDLE;
	}
	Sampler::~Sampler()
	{
		SAFE_RELEASE(resource_DX11);
		SAFE_DELETE(resource_DX12);
	}

	GPUResource::GPUResource()
	{
		SAFE_INIT(SRV_DX11);
		SAFE_INIT(SRV_DX12);
		SRV_Vulkan = WI_NULL_HANDLE;
		SAFE_INIT(UAV_DX11);
		SAFE_INIT(UAV_DX12);
		UAV_Vulkan = WI_NULL_HANDLE;
		SAFE_INIT(resource_DX12);
		resource_Vulkan = WI_NULL_HANDLE;
		resourceMemory_Vulkan = WI_NULL_HANDLE;
	}
	GPUResource::~GPUResource()
	{
		SAFE_RELEASE(SRV_DX11);
		for (auto& x : additionalSRVs_DX11)
		{
			SAFE_RELEASE(x);
		}

		SAFE_DELETE(SRV_DX12);
		for (auto& x : additionalSRVs_DX12)
		{
			SAFE_DELETE(x);
		}


		SAFE_RELEASE(UAV_DX11);
		for (auto& x : additionalUAVs_DX11)
		{
			SAFE_RELEASE(x);
		}

		SAFE_DELETE(UAV_DX12);
		for (auto& x : additionalUAVs_DX12)
		{
			SAFE_DELETE(x);
		}


		SAFE_RELEASE(resource_DX12);
	}

	GPUBuffer::GPUBuffer() : GPUResource()
	{
		SAFE_INIT(resource_DX11);
		SAFE_INIT(CBV_DX12);
	}
	GPUBuffer::~GPUBuffer()
	{
		SAFE_RELEASE(resource_DX11);
		SAFE_DELETE(CBV_DX12);
	}

	VertexLayout::VertexLayout()
	{
		SAFE_INIT(resource_DX11);
	}
	VertexLayout::~VertexLayout()
	{
		SAFE_RELEASE(resource_DX11);
	}

	BlendState::BlendState()
	{
		SAFE_INIT(resource_DX11);
	}
	BlendState::~BlendState()
	{
		SAFE_RELEASE(resource_DX11);
	}

	DepthStencilState::DepthStencilState()
	{
		SAFE_INIT(resource_DX11);
	}
	DepthStencilState::~DepthStencilState()
	{
		SAFE_RELEASE(resource_DX11);
	}

	RasterizerState::RasterizerState()
	{
		SAFE_INIT(resource_DX11);
	}
	RasterizerState::~RasterizerState()
	{
		SAFE_RELEASE(resource_DX11);
	}

	VertexShaderInfo::VertexShaderInfo()
	{
		SAFE_INIT(vertexShader);
		SAFE_INIT(vertexLayout);
	}
	VertexShaderInfo::~VertexShaderInfo()
	{
		SAFE_DELETE(vertexShader);
		SAFE_DELETE(vertexLayout);
	}

	Texture::Texture() : GPUResource()
		, independentRTVArraySlices(false), independentRTVCubemapFaces(false), independentSRVArraySlices(false)
		, independentSRVMIPs(false), independentUAVMIPs(false)
	{
		SAFE_INIT(RTV_DX11);
		SAFE_INIT(RTV_DX12);
		RTV_Vulkan = WI_NULL_HANDLE;
	}
	Texture::~Texture()
	{
		SAFE_RELEASE(RTV_DX11);
		for (auto& x : additionalRTVs_DX11)
		{
			SAFE_RELEASE(x);
		}

		SAFE_DELETE(RTV_DX12);
		for (auto& x : additionalRTVs_DX12)
		{
			SAFE_DELETE(x);
		}
	}
	void Texture::RequestIndependentRenderTargetArraySlices(bool value)
	{
		independentRTVArraySlices = value;
	}
	void Texture::RequestIndependentRenderTargetCubemapFaces(bool value)
	{
		independentRTVCubemapFaces = value;
	}
	void Texture::RequestIndependentShaderResourceArraySlices(bool value)
	{
		independentSRVArraySlices = value;
	}
	void Texture::RequestIndependentShaderResourcesForMIPs(bool value)
	{
		independentSRVMIPs = value;
	}
	void Texture::RequestIndependentUnorderedAccessResourcesForMIPs(bool value)
	{
		independentUAVMIPs = value;
	}

	Texture1D::Texture1D() :Texture()
	{
		SAFE_INIT(texture1D_DX11);
	}
	Texture1D::~Texture1D()
	{
		SAFE_RELEASE(texture1D_DX11);
	}

	Texture2D::Texture2D() :Texture()
	{
		SAFE_INIT(texture2D_DX11);
		SAFE_INIT(DSV_DX11);
		SAFE_INIT(DSV_DX12);
		DSV_Vulkan = WI_NULL_HANDLE;
	}
	Texture2D::~Texture2D()
	{
		SAFE_RELEASE(texture2D_DX11);
		SAFE_RELEASE(DSV_DX11);
		for (auto& x : additionalDSVs_DX11)
		{
			SAFE_RELEASE(x);
		}

		SAFE_DELETE(DSV_DX12);
		for (auto& x : additionalDSVs_DX12)
		{
			SAFE_DELETE(x);
		}
	}

	Texture3D::Texture3D() :Texture()
	{
		SAFE_INIT(texture3D_DX11);
	}
	Texture3D::~Texture3D()
	{
		SAFE_RELEASE(texture3D_DX11);
	}

	GPUQuery::GPUQuery()
	{
		async_frameshift = 0;
	}
	GPUQuery::~GPUQuery()
	{
		for (auto& x : resource_DX11)
		{
			SAFE_RELEASE(x);
		}
	}


	GraphicsPSO::GraphicsPSO()
	{
		SAFE_INIT(resource_DX12);
		pipeline_Vulkan = WI_NULL_HANDLE;
		renderPass_Vulkan = WI_NULL_HANDLE;
	}
	GraphicsPSO::~GraphicsPSO()
	{
		SAFE_RELEASE(resource_DX12);
	}

	ComputePSO::ComputePSO()
	{
		SAFE_INIT(resource_DX12);
		pipeline_Vulkan = WI_NULL_HANDLE;
	}
	ComputePSO::~ComputePSO()
	{
		SAFE_RELEASE(resource_DX12);
	}
}
