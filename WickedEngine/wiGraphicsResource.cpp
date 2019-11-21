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

	Texture::~Texture()
	{
		if (device != nullptr)
		{
			device->DestroyTexture(this);
		}
	}

	GPUQuery::~GPUQuery()
	{
		if (device != nullptr)
		{
			device->DestroyQuery(this);
		}
	}


	PipelineState::~PipelineState()
	{
		if (device != nullptr)
		{
			device->DestroyPipelineState(this);
		}
	}

	RenderPass::~RenderPass()
	{
		if (device != nullptr)
		{
			device->DestroyRenderPass(this);
		}
	}
}
