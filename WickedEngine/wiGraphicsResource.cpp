#include "wiGraphicsResource.h"
#include "wiGraphicsDevice.h"

namespace wiGraphicsTypes
{
	VertexShader::VertexShader()
	{
		SAFE_INIT(resource_DX11);
	}
	VertexShader::~VertexShader()
	{
		if (device != nullptr) 
		{
			device->DestroyVertexShader(this);
		}
	}

	PixelShader::PixelShader()
	{
		SAFE_INIT(resource_DX11);
	}
	PixelShader::~PixelShader()
	{
		if (device != nullptr)
		{
			device->DestroyPixelShader(this);
		}
	}

	GeometryShader::GeometryShader()
	{
		SAFE_INIT(resource_DX11);
	}
	GeometryShader::~GeometryShader()
	{
		if (device != nullptr)
		{
			device->DestroyGeometryShader(this);
		}
	}

	DomainShader::DomainShader()
	{
		SAFE_INIT(resource_DX11);
	}
	DomainShader::~DomainShader()
	{
		if (device != nullptr)
		{
			device->DestroyDomainShader(this);
		}
	}

	HullShader::HullShader()
	{
		SAFE_INIT(resource_DX11);
	}
	HullShader::~HullShader()
	{
		if (device != nullptr)
		{
			device->DestroyHullShader(this);
		}
	}

	ComputeShader::ComputeShader()
	{
		SAFE_INIT(resource_DX11);
	}
	ComputeShader::~ComputeShader()
	{
		if (device != nullptr)
		{
			device->DestroyComputeShader(this);
		}
	}

	Sampler::Sampler()
	{
		SAFE_INIT(resource_DX11);
		resource_DX12 = WI_NULL_HANDLE;
		resource_Vulkan = WI_NULL_HANDLE;
	}
	Sampler::~Sampler()
	{
		if (device != nullptr)
		{
			device->DestroySamplerState(this);
		}
	}

	GPUResource::GPUResource()
	{
		SAFE_INIT(SRV_DX11);
		SRV_DX12 = WI_NULL_HANDLE;
		SRV_Vulkan = WI_NULL_HANDLE;
		SAFE_INIT(UAV_DX11);
		UAV_DX12 = WI_NULL_HANDLE;
		UAV_Vulkan = WI_NULL_HANDLE;
		resource_DX12 = WI_NULL_HANDLE;
		resource_Vulkan = WI_NULL_HANDLE;
		resourceMemory_Vulkan = WI_NULL_HANDLE;
	}
	GPUResource::~GPUResource()
	{
		if (device != nullptr)
		{
			device->DestroyResource(this);
		}
	}

	GPUBuffer::GPUBuffer() : GPUResource()
	{
		SAFE_INIT(resource_DX11);
		CBV_DX12 = WI_NULL_HANDLE;
	}
	GPUBuffer::~GPUBuffer()
	{
		if (device != nullptr)
		{
			device->DestroyBuffer(this);
		}
	}

	VertexLayout::VertexLayout()
	{
		SAFE_INIT(resource_DX11);
	}
	VertexLayout::~VertexLayout()
	{
		if (device != nullptr)
		{
			device->DestroyInputLayout(this);
		}
	}

	BlendState::BlendState()
	{
		SAFE_INIT(resource_DX11);
	}
	BlendState::~BlendState()
	{
		if (device != nullptr)
		{
			device->DestroyBlendState(this);
		}
	}

	DepthStencilState::DepthStencilState()
	{
		SAFE_INIT(resource_DX11);
	}
	DepthStencilState::~DepthStencilState()
	{
		if (device != nullptr)
		{
			device->DestroyDepthStencilState(this);
		}
	}

	RasterizerState::RasterizerState()
	{
		SAFE_INIT(resource_DX11);
	}
	RasterizerState::~RasterizerState()
	{
		if (device != nullptr)
		{
			device->DestroyRasterizerState(this);
		}
	}

	Texture::Texture() : GPUResource()
		, independentRTVArraySlices(false), independentRTVCubemapFaces(false), independentSRVArraySlices(false)
		, independentSRVMIPs(false), independentUAVMIPs(false)
	{
		SAFE_INIT(RTV_DX11);
		RTV_DX12 = WI_NULL_HANDLE;
		RTV_Vulkan = WI_NULL_HANDLE;
	}
	Texture::~Texture()
	{
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
		if (device != nullptr)
		{
			device->DestroyTexture1D(this);
		}
	}

	Texture2D::Texture2D() :Texture()
	{
		SAFE_INIT(texture2D_DX11);
		SAFE_INIT(DSV_DX11);
		DSV_DX12 = WI_NULL_HANDLE;
		DSV_Vulkan = WI_NULL_HANDLE;
	}
	Texture2D::~Texture2D()
	{
		if (device != nullptr)
		{
			device->DestroyTexture2D(this);
		}
	}

	Texture3D::Texture3D() :Texture()
	{
		SAFE_INIT(texture3D_DX11);
	}
	Texture3D::~Texture3D()
	{
		if (device != nullptr)
		{
			device->DestroyTexture3D(this);
		}
	}

	GPUQuery::GPUQuery()
	{
		async_frameshift = 0;
	}
	GPUQuery::~GPUQuery()
	{
		if (device != nullptr)
		{
			device->DestroyQuery(this);
		}
	}


	GraphicsPSO::GraphicsPSO()
	{
		pipeline_DX12 = WI_NULL_HANDLE;
		pipeline_Vulkan = WI_NULL_HANDLE;
	}
	GraphicsPSO::~GraphicsPSO()
	{
		if (device != nullptr)
		{
			device->DestroyGraphicsPSO(this);
		}
	}

	ComputePSO::ComputePSO()
	{
		pipeline_DX12 = WI_NULL_HANDLE;
		pipeline_Vulkan = WI_NULL_HANDLE;
	}
	ComputePSO::~ComputePSO()
	{
		if (device != nullptr)
		{
			device->DestroyComputePSO(this);
		}
	}
}
