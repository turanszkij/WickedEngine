#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

namespace wiGraphicsTypes
{

	class GraphicsDevice_DX11 : public GraphicsDevice
	{
	private:
		ID3D11Device*				device;
		D3D_DRIVER_TYPE				driverType;
		D3D_FEATURE_LEVEL			featureLevel;
		IDXGISwapChain*				swapChain;
		ID3D11RenderTargetView*		renderTargetView;
		ViewPort					viewPort;
		ID3D11DeviceContext*		deviceContexts[GRAPHICSTHREAD_COUNT];
		ID3D11CommandList*			commandLists[GRAPHICSTHREAD_COUNT];
		bool						DX11, DEFERREDCONTEXT_SUPPORT;

	public:
#ifndef WINSTORE_SUPPORT
		GraphicsDevice_DX11(HWND window, int screenW, int screenH, bool windowed);
#else
		GraphicsDevice_DX11(Windows::UI::Core::CoreWindow^ window);
#endif

		~GraphicsDevice_DX11();

		virtual HRESULT CreateBuffer(const BufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer);
		virtual HRESULT CreateTexture1D();
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D);
		virtual HRESULT CreateTexture3D();
		virtual HRESULT CreateTextureCube(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, TextureCube **ppTextureCube);
		virtual HRESULT CreateShaderResourceView(Texture2D* pTexture);
		virtual HRESULT CreateRenderTargetView(Texture2D* pTexture);
		virtual HRESULT CreateDepthStencilView(Texture2D* pTexture);
		virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
			const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout);
		virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, VertexShader *pVertexShader);
		virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, PixelShader *pPixelShader);
		virtual HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, GeometryShader *pGeometryShader);
		virtual HRESULT CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const StreamOutDeclaration *pSODeclaration,
			UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ClassLinkage* pClassLinkage, GeometryShader *pGeometryShader);
		virtual HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, HullShader *pHullShader);
		virtual HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, DomainShader *pDomainShader);
		virtual HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, ComputeShader *pComputeShader);
		virtual HRESULT CreateBlendState(const BlendDesc *pBlendStateDesc, BlendState *pBlendState);
		virtual HRESULT CreateDepthStencilState(const DepthStencilDesc *pDepthStencilDesc, DepthStencilState *pDepthStencilState);
		virtual HRESULT CreateRasterizerState(const RasterizerDesc *pRasterizerDesc, RasterizerState *pRasterizerState);
		virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState);

		virtual void PresentBegin();
		virtual void PresentEnd();

		virtual void ExecuteDeferredContexts();
		virtual void FinishCommandList(GRAPHICSTHREAD thread);

		virtual bool GetMultithreadingSupport() { return DEFERREDCONTEXT_SUPPORT; }

		virtual void SetScreenWidth(int value);
		virtual void SetScreenHeight(int value);

		///////////////Thread-sensitive////////////////////////

		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				D3D11_VIEWPORT* pd3dViewPorts = new D3D11_VIEWPORT[NumViewports];
				for (UINT i = 0; i < NumViewports; ++i)
				{
					pd3dViewPorts[i].TopLeftX = pViewports[i].TopLeftX;
					pd3dViewPorts[i].TopLeftY = pViewports[i].TopLeftY;
					pd3dViewPorts[i].Width = pViewports[i].Width;
					pd3dViewPorts[i].Height = pViewports[i].Height;
					pd3dViewPorts[i].MinDepth = pViewports[i].MinDepth;
					pd3dViewPorts[i].MaxDepth = pViewports[i].MaxDepth;
				}
				deviceContexts[threadID]->RSSetViewports(NumViewports, pd3dViewPorts);
				SAFE_DELETE_ARRAY(pd3dViewPorts);
			}
		}
		virtual void BindRenderTargets(UINT NumViews, Texture2D* const *ppRenderTargetViews, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				static thread_local ID3D11RenderTargetView* renderTargetViews[8];
				for (UINT i = 0; i < min(NumViews, 8); ++i)
				{
					renderTargetViews[i] = ppRenderTargetViews[i]->renderTargetView_DX11;
				}
				deviceContexts[threadID]->OMSetRenderTargets(NumViews, renderTargetViews,
					(depthStencilTexture == nullptr ? nullptr : depthStencilTexture->depthStencilView_DX11));
			}
		}
		virtual void ClearRenderTarget(Texture2D* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && pTexture != nullptr && pTexture->renderTargetView_DX11 != nullptr) {
				deviceContexts[threadID]->ClearRenderTargetView(pTexture->renderTargetView_DX11, ColorRGBA);
			}
		}
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && pTexture != nullptr && pTexture->depthStencilView_DX11 != nullptr) {
				UINT _flags = 0;
				if (ClearFlags & CLEAR_DEPTH)
					_flags |= D3D11_CLEAR_DEPTH;
				if (ClearFlags & CLEAR_STENCIL)
					_flags |= D3D11_CLEAR_STENCIL;
				deviceContexts[threadID]->ClearDepthStencilView(pTexture->depthStencilView_DX11, _flags, Depth, Stencil);
			}
		}
		virtual void BindTexturePS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->PSSetShaderResources(slot, 1, &texture->shaderResourceView_DX11);
			}
		}
		virtual void BindTextureVS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->VSSetShaderResources(slot, 1, &texture->shaderResourceView_DX11);
			}
		}
		virtual void BindTextureGS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->GSSetShaderResources(slot, 1, &texture->shaderResourceView_DX11);
			}
		}
		virtual void BindTextureDS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->DSSetShaderResources(slot, 1, &texture->shaderResourceView_DX11);
			}
		}
		virtual void BindTextureHS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->HSSetShaderResources(slot, 1, &texture->shaderResourceView_DX11);
			}
		}
		virtual void UnbindTextures(int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE)
		{
			assert(num <= 32 && "UnbindTextures limit of 32 reached!");
			if (deviceContexts[threadID] != nullptr)
			{
				static ID3D11ShaderResourceView* empties[32] = {
					nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
					nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
					nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
					nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
				};
				deviceContexts[threadID]->PSSetShaderResources(slot, num, empties);
				deviceContexts[threadID]->VSSetShaderResources(slot, num, empties);
				deviceContexts[threadID]->GSSetShaderResources(slot, num, empties);
				deviceContexts[threadID]->HSSetShaderResources(slot, num, empties);
				deviceContexts[threadID]->DSSetShaderResources(slot, num, empties);
			}
		}
		virtual void BindSamplerPS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr && sampler->resource_DX11 != nullptr) {
				deviceContexts[threadID]->PSSetSamplers(slot, 1, &sampler->resource_DX11);
			}
		}
		virtual void BindSamplerVS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr && sampler->resource_DX11 != nullptr) {
				deviceContexts[threadID]->VSSetSamplers(slot, 1, &sampler->resource_DX11);
			}
		}
		virtual void BindSamplerGS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr && sampler->resource_DX11 != nullptr) {
				deviceContexts[threadID]->GSSetSamplers(slot, 1, &sampler->resource_DX11);
			}
		}
		virtual void BindSamplerHS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr && sampler->resource_DX11 != nullptr) {
				deviceContexts[threadID]->HSSetSamplers(slot, 1, &sampler->resource_DX11);
			}
		}
		virtual void BindSamplerDS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr && sampler->resource_DX11 != nullptr) {
				deviceContexts[threadID]->DSSetSamplers(slot, 1, &sampler->resource_DX11);
			}
		}
		virtual void BindConstantBufferPS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11Buffer* res = buffer ? buffer->resource_DX11 : nullptr;
				deviceContexts[threadID]->PSSetConstantBuffers(slot, 1, &res);
			}
		}
		virtual void BindConstantBufferVS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11Buffer* res = buffer ? buffer->resource_DX11 : nullptr;
				deviceContexts[threadID]->VSSetConstantBuffers(slot, 1, &res);

			}
		}
		virtual void BindConstantBufferGS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11Buffer* res = buffer ? buffer->resource_DX11 : nullptr;
				deviceContexts[threadID]->GSSetConstantBuffers(slot, 1, &res);
			}
		}
		virtual void BindConstantBufferDS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11Buffer* res = buffer ? buffer->resource_DX11 : nullptr;
				deviceContexts[threadID]->DSSetConstantBuffers(slot, 1, &res);
			}
		}
		virtual void BindConstantBufferHS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11Buffer* res = buffer ? buffer->resource_DX11 : nullptr;
				deviceContexts[threadID]->HSSetConstantBuffers(slot, 1, &res);
			}
		}
		virtual void BindVertexBuffer(const GPUBuffer* vertexBuffer, int slot, UINT stride, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				UINT offset = 0;
				ID3D11Buffer* res = vertexBuffer ? vertexBuffer->resource_DX11 : nullptr;
				deviceContexts[threadID]->IASetVertexBuffers(slot, 1, &res, &stride, &offset);
			}
		}
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11Buffer* res = indexBuffer ? indexBuffer->resource_DX11 : nullptr;
				deviceContexts[threadID]->IASetIndexBuffer(res, DXGI_FORMAT_R32_UINT, 0);
			}
		}
		virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				D3D11_PRIMITIVE_TOPOLOGY d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				switch (type)
				{
				case TRIANGLELIST:
					d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					break;
				case TRIANGLESTRIP:
					d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
					break;
				case POINTLIST:
					d3dType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
					break;
				case LINELIST:
					d3dType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
					break;
				case PATCHLIST:
					d3dType = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
					break;
				default:
					break;
				};
				deviceContexts[threadID]->IASetPrimitiveTopology(d3dType);
			}
		}
		virtual void BindVertexLayout(const VertexLayout* layout, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11InputLayout* res = layout ? layout->resource_DX11 : nullptr;
				deviceContexts[threadID]->IASetInputLayout(res);
			}
		}
		virtual void BindBlendState(const BlendState* state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && state != nullptr) {
				static float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				static UINT sampleMask = 0xffffffff;
				deviceContexts[threadID]->OMSetBlendState(state->resource_DX11, blendFactor, sampleMask);
			}
		}
		virtual void BindBlendStateEx(const BlendState* state, const XMFLOAT4& blendFactor = XMFLOAT4(1, 1, 1, 1), UINT sampleMask = 0xffffffff,
			GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && state != nullptr) {
				float fblendFactor[4] = { blendFactor.x, blendFactor.y, blendFactor.z, blendFactor.w };
				deviceContexts[threadID]->OMSetBlendState(state->resource_DX11, fblendFactor, sampleMask);
			}
		}
		virtual void BindDepthStencilState(const DepthStencilState* state, UINT stencilRef, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && state != nullptr) {
				deviceContexts[threadID]->OMSetDepthStencilState(state->resource_DX11, stencilRef);
			}
		}
		virtual void BindRasterizerState(const RasterizerState* state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && state != nullptr) {
				deviceContexts[threadID]->RSSetState(state->resource_DX11);
			}
		}
		virtual void BindStreamOutTarget(const GPUBuffer* buffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				UINT offsetSO[1] = { 0 };
				ID3D11Buffer* res = buffer ? buffer->resource_DX11 : nullptr;
				deviceContexts[threadID]->SOSetTargets(1, &res, offsetSO);
			}
		}
		virtual void BindPS(const PixelShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11PixelShader* res = shader ? shader->resource_DX11 : nullptr;
				deviceContexts[threadID]->PSSetShader(res, nullptr, 0);
			}
		}
		virtual void BindVS(const VertexShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11VertexShader* res = shader ? shader->resource_DX11 : nullptr;
				deviceContexts[threadID]->VSSetShader(res, nullptr, 0);
			}
		}
		virtual void BindGS(const GeometryShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11GeometryShader* res = shader ? shader->resource_DX11 : nullptr;
				deviceContexts[threadID]->GSSetShader(res, nullptr, 0);
			}
		}
		virtual void BindHS(const HullShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11HullShader* res = shader ? shader->resource_DX11 : nullptr;
				deviceContexts[threadID]->HSSetShader(res, nullptr, 0);
			}
		}
		virtual void BindDS(const DomainShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				ID3D11DomainShader* res = shader ? shader->resource_DX11 : nullptr;
				deviceContexts[threadID]->DSSetShader(res, nullptr, 0);
			}
		}
		virtual void Draw(int vertexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->Draw(vertexCount, 0);
			}
		}
		virtual void DrawIndexed(int indexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->DrawIndexed(indexCount, 0, 0);
			}
		}
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
			}
		}
		virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE)
		{
			deviceContexts[threadID]->GenerateMips(texture->shaderResourceView_DX11);
		}
		virtual void CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				SAFE_RELEASE(pDst->shaderResourceView_DX11);
				deviceContexts[threadID]->CopyResource(pDst->texture2D_DX11, pSrc->texture2D_DX11);
				CreateShaderResourceView(pDst);
			}
		}
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, int dataSize = -1)
		{
			if (buffer != nullptr && buffer->resource_DX11 != nullptr && data != nullptr && deviceContexts[threadID] != nullptr) {
				static thread_local D3D11_BUFFER_DESC d3dDesc;
				buffer->resource_DX11->GetDesc(&d3dDesc);
				HRESULT hr;
				if (dataSize > (int)d3dDesc.ByteWidth) { //recreate the buffer if new datasize exceeds buffer size
					buffer->resource_DX11->Release();
					d3dDesc.ByteWidth = dataSize * 2;

					BufferDesc desc;
					desc.ByteWidth = d3dDesc.ByteWidth;
					switch (d3dDesc.Usage)
					{
					case D3D11_USAGE_DEFAULT:
						desc.Usage = USAGE_DEFAULT;
						break;
					case D3D11_USAGE_IMMUTABLE:
						desc.Usage = USAGE_IMMUTABLE;
						break;
					case D3D11_USAGE_DYNAMIC:
						desc.Usage = USAGE_DYNAMIC;
						break;
					case D3D11_USAGE_STAGING:
						desc.Usage = USAGE_STAGING;
						break;
					};
					desc.BindFlags = 0;
					{
						if (d3dDesc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
							desc.BindFlags |= BIND_CONSTANT_BUFFER;
						if (d3dDesc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
							desc.BindFlags |= BIND_VERTEX_BUFFER;
						if (d3dDesc.BindFlags & D3D11_BIND_INDEX_BUFFER)
							desc.BindFlags |= BIND_INDEX_BUFFER;
					}
					desc.CPUAccessFlags = 0;
					{
						if (d3dDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE)
							desc.CPUAccessFlags |= CPU_ACCESS_WRITE;
						if (d3dDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ)
							desc.CPUAccessFlags |= CPU_ACCESS_READ;
					}
					desc.MiscFlags = 0;
					desc.StructureByteStride = d3dDesc.StructureByteStride;

					hr = CreateBuffer(&desc, nullptr, buffer);
				}
				if (d3dDesc.Usage == D3D11_USAGE_DYNAMIC) {
					static thread_local D3D11_MAPPED_SUBRESOURCE mappedResource;
					void* dataPtr;
					deviceContexts[threadID]->Map(buffer->resource_DX11, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					dataPtr = (void*)mappedResource.pData;
					memcpy(dataPtr, data, (dataSize >= 0 ? dataSize : d3dDesc.ByteWidth));
					deviceContexts[threadID]->Unmap(buffer->resource_DX11, 0);
				}
				else {
					deviceContexts[threadID]->UpdateSubresource(buffer->resource_DX11, 0, nullptr, data, 0, 0);
				}
			}
		}

		virtual HRESULT CreateTextureFromFile(const wstring& fileName, Texture2D **ppTexture, bool mipMaps = true, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);

	};

}
