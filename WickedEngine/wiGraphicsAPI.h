#pragma once
#include "CommonInclude.h"

#include <d3d11_2.h>
#include <DXGI1_2.h>

#include "ConstantBufferMapping.h"


#ifndef WINSTORE_SUPPORT
typedef IDXGISwapChain*				SwapChain;
#else
typedef IDXGISwapChain1*			SwapChain;
#endif
typedef IUnknown*					APIInterface;
typedef ID3D11DeviceContext*		DeviceContext;
typedef	ID3D11Device*				GraphicsDevice;
typedef	D3D11_VIEWPORT				ViewPort;
typedef ID3D11CommandList*			CommandList;
typedef ID3D11RenderTargetView*		RenderTargetView;
typedef ID3D11ShaderResourceView*	TextureView;
typedef ID3D11Texture2D*			Texture2D;
typedef ID3D11SamplerState*			Sampler;
typedef ID3D11Resource*				APIResource;
typedef ID3D11Buffer*				BufferResource;
typedef ID3D11VertexShader*			VertexShader;
typedef ID3D11PixelShader*			PixelShader;
typedef ID3D11GeometryShader*		GeometryShader;
typedef ID3D11HullShader*			HullShader;
typedef ID3D11DomainShader*			DomainShader;
typedef ID3D11ComputeShader*		ComputeShader;
typedef ID3D11InputLayout*			VertexLayout;
typedef D3D11_INPUT_ELEMENT_DESC    VertexLayoutDesc;
typedef ID3D11BlendState*			BlendState;
typedef ID3D11DepthStencilState*	DepthStencilState;
typedef ID3D11DepthStencilView*		DepthStencilView;
typedef ID3D11RasterizerState*		RasterizerState;
typedef D3D11_SO_DECLARATION_ENTRY	StreamOutDeclaration;
typedef D3D11_TEXTURE2D_DESC		Texture2DDesc;


enum PRIMITIVETOPOLOGY {
	TRIANGLELIST		= D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	TRIANGLESTRIP		= D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	POINTLIST			= D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
	LINELIST			= D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
	PATCHLIST			= D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
};

struct VertexShaderInfo {
	VertexShader vertexShader;
	VertexLayout vertexLayout;

	VertexShaderInfo() {
		vertexShader = nullptr;
		vertexLayout = VertexLayout();
	}
};


