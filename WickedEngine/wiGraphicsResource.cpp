#include "wiGraphicsResource.h"

#include <d3d11_2.h>
#include <DXGI1_2.h>

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
	}
	Sampler::~Sampler()
	{
		SAFE_RELEASE(resource_DX11);
	}

	GPUResource::GPUResource()
	{
		SAFE_INIT(shaderResourceView_DX11);
	}
	GPUResource::~GPUResource()
	{
		SAFE_RELEASE(shaderResourceView_DX11);
	}

	GPUUnorderedResource::GPUUnorderedResource()
	{
		SAFE_INIT(unorderedAccessView_DX11);
	}
	GPUUnorderedResource::~GPUUnorderedResource()
	{
		SAFE_RELEASE(unorderedAccessView_DX11);
	}

	GPUBuffer::GPUBuffer() : GPUResource(), GPUUnorderedResource()
	{
		SAFE_INIT(resource_DX11);
	}
	GPUBuffer::~GPUBuffer()
	{
		SAFE_RELEASE(resource_DX11);
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

	ClassLinkage::ClassLinkage()
	{
		SAFE_INIT(resource_DX11);
	}
	ClassLinkage::~ClassLinkage()
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

	Texture::Texture() : GPUResource(), GPUUnorderedResource(), independentRTVArraySlices(false), independentRTVCubemapFaces(false)
	{
	}
	Texture::~Texture()
	{
		for (auto& x : renderTargetViews_DX11)
		{
			SAFE_RELEASE(x);
		}
		for (auto& x : depthStencilViews_DX11)
		{
			SAFE_RELEASE(x);
		}
	}
	void Texture::RequestIndepententRenderTargetArraySlices(bool value)
	{
		independentRTVArraySlices = value;
	}
	bool Texture::IsIndepententRenderTargetArraySlices()
	{
		return independentRTVArraySlices;
	}
	void Texture::RequestIndepententRenderTargetCubemapFaces(bool value)
	{
		independentRTVCubemapFaces = value;
	}
	bool Texture::IsIndepententRenderTargetCubemapFaces()
	{
		return independentRTVCubemapFaces;
	}

	Texture2D::Texture2D() :Texture()
	{
		SAFE_INIT(texture2D_DX11);
	}
	Texture2D::~Texture2D()
	{
		SAFE_RELEASE(texture2D_DX11);
	}

	GPUQuery::GPUQuery()
	{
		for (int i = 0; i < ASYNC_LATENCY; ++i)
		{
			SAFE_INIT(resource_DX11[i]);
			active[i] = false;
		}
		async = false;
	}
	GPUQuery::~GPUQuery()
	{
		for (int i = 0; i < ASYNC_LATENCY; ++i)
		{
			SAFE_RELEASE(resource_DX11[i]);
		}
	}
}
