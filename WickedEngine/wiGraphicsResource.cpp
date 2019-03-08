#include "wiGraphicsResource.h"
#include "wiGraphicsDevice.h"

namespace wiGraphics
{
	VertexShader::~VertexShader()
	{
		if (device != nullptr) 
		{
			device->DestroyVertexShader(this);
		}
	}

	PixelShader::~PixelShader()
	{
		if (device != nullptr)
		{
			device->DestroyPixelShader(this);
		}
	}

	GeometryShader::~GeometryShader()
	{
		if (device != nullptr)
		{
			device->DestroyGeometryShader(this);
		}
	}

	DomainShader::~DomainShader()
	{
		if (device != nullptr)
		{
			device->DestroyDomainShader(this);
		}
	}

	HullShader::~HullShader()
	{
		if (device != nullptr)
		{
			device->DestroyHullShader(this);
		}
	}

	ComputeShader::~ComputeShader()
	{
		if (device != nullptr)
		{
			device->DestroyComputeShader(this);
		}
	}

	Sampler::~Sampler()
	{
		if (device != nullptr)
		{
			device->DestroySamplerState(this);
		}
	}

	GPUResource::~GPUResource()
	{
		if (device != nullptr)
		{
			device->DestroyResource(this);
		}
	}

	GPUBuffer::~GPUBuffer()
	{
		if (device != nullptr)
		{
			device->DestroyBuffer(this);
		}
	}

	VertexLayout::~VertexLayout()
	{
		if (device != nullptr)
		{
			device->DestroyInputLayout(this);
		}
	}

	BlendState::~BlendState()
	{
		if (device != nullptr)
		{
			device->DestroyBlendState(this);
		}
	}

	DepthStencilState::~DepthStencilState()
	{
		if (device != nullptr)
		{
			device->DestroyDepthStencilState(this);
		}
	}

	RasterizerState::~RasterizerState()
	{
		if (device != nullptr)
		{
			device->DestroyRasterizerState(this);
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

	Texture1D::~Texture1D()
	{
		if (device != nullptr)
		{
			device->DestroyTexture1D(this);
		}
	}

	Texture2D::~Texture2D()
	{
		if (device != nullptr)
		{
			device->DestroyTexture2D(this);
		}
	}

	Texture3D::~Texture3D()
	{
		if (device != nullptr)
		{
			device->DestroyTexture3D(this);
		}
	}

	GPUQuery::~GPUQuery()
	{
		if (device != nullptr)
		{
			device->DestroyQuery(this);
		}
	}


	GraphicsPSO::~GraphicsPSO()
	{
		if (device != nullptr)
		{
			device->DestroyGraphicsPSO(this);
		}
	}

	ComputePSO::~ComputePSO()
	{
		if (device != nullptr)
		{
			device->DestroyComputePSO(this);
		}
	}
}
