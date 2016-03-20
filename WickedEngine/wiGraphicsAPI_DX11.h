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

		virtual HRESULT CreateBuffer(const BufferDesc *pDesc, const SubresourceData* pInitialData, BufferResource *ppBuffer);
		virtual HRESULT CreateTexture1D();
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D);
		virtual HRESULT CreateTexture3D();
		virtual HRESULT CreateTextureCube(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, TextureCube **ppTextureCube);
		virtual HRESULT CreateShaderResourceView(Texture2D* pTexture);
		virtual HRESULT CreateRenderTargetView(Texture2D* pTexture);
		virtual HRESULT CreateDepthStencilView(Texture2D* pTexture);
		virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
			const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *ppInputLayout);
		virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, VertexShader *ppVertexShader);
		virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, PixelShader *ppPixelShader);
		virtual HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, GeometryShader *ppGeometryShader);
		virtual HRESULT CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const StreamOutDeclaration *pSODeclaration,
			UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ClassLinkage pClassLinkage, GeometryShader *ppGeometryShader);
		virtual HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, HullShader *ppHullShader);
		virtual HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, DomainShader *ppDomainShader);
		virtual HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, ComputeShader *ppComputeShader);
		virtual HRESULT CreateBlendState(const BlendDesc *pBlendStateDesc, BlendState *ppBlendState);
		virtual HRESULT CreateDepthStencilState(const DepthStencilDesc *pDepthStencilDesc, DepthStencilState *ppDepthStencilState);
		virtual HRESULT CreateRasterizerState(const RasterizerDesc *pRasterizerDesc, RasterizerState *ppRasterizerState);
		virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *ppSamplerState);

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
				static thread_local RenderTargetView renderTargetViews[8];
				for (UINT i = 0; i < min(NumViews, 8); ++i)
				{
					renderTargetViews[i] = ppRenderTargetViews[i]->renderTargetView;
				}
				deviceContexts[threadID]->OMSetRenderTargets(NumViews, renderTargetViews,
					(depthStencilTexture == nullptr ? nullptr : depthStencilTexture->depthStencilView));
			}
		}
		virtual void ClearRenderTarget(Texture2D* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && pTexture != nullptr && pTexture->renderTargetView != nullptr) {
				deviceContexts[threadID]->ClearRenderTargetView(pTexture->renderTargetView, ColorRGBA);
			}
		}
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && pTexture != nullptr && pTexture->depthStencilView != nullptr) {
				UINT _flags = 0;
				if (ClearFlags & CLEAR_DEPTH)
					_flags |= D3D11_CLEAR_DEPTH;
				if (ClearFlags & CLEAR_STENCIL)
					_flags |= D3D11_CLEAR_STENCIL;
				deviceContexts[threadID]->ClearDepthStencilView(pTexture->depthStencilView, _flags, Depth, Stencil);
			}
		}
		virtual void BindTexturePS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->PSSetShaderResources(slot, 1, &texture->shaderResourceView);
			}
		}
		virtual void BindTextureVS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->VSSetShaderResources(slot, 1, &texture->shaderResourceView);
			}
		}
		virtual void BindTextureGS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->GSSetShaderResources(slot, 1, &texture->shaderResourceView);
			}
		}
		virtual void BindTextureDS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->DSSetShaderResources(slot, 1, &texture->shaderResourceView);
			}
		}
		virtual void BindTextureHS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && texture != nullptr) {
				deviceContexts[threadID]->HSSetShaderResources(slot, 1, &texture->shaderResourceView);
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
		virtual void BindSamplerPS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr) {
				deviceContexts[threadID]->PSSetSamplers(slot, 1, &sampler);
			}
		}
		virtual void BindSamplersPS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && samplers != nullptr) {
				deviceContexts[threadID]->PSSetSamplers(slot, num, samplers);
			}
		}
		virtual void BindSamplerVS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr) {
				deviceContexts[threadID]->VSSetSamplers(slot, 1, &sampler);
			}
		}
		virtual void BindSamplersVS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && samplers != nullptr) {
				deviceContexts[threadID]->VSSetSamplers(slot, num, samplers);
			}
		}
		virtual void BindSamplerGS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr) {
				deviceContexts[threadID]->GSSetSamplers(slot, 1, &sampler);
			}
		}
		virtual void BindSamplersGS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && samplers != nullptr) {
				deviceContexts[threadID]->GSSetSamplers(slot, num, samplers);
			}
		}
		virtual void BindSamplerHS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr) {
				deviceContexts[threadID]->HSSetSamplers(slot, 1, &sampler);
			}
		}
		virtual void BindSamplersHS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && samplers != nullptr) {
				deviceContexts[threadID]->HSSetSamplers(slot, num, samplers);
			}
		}
		virtual void BindSamplerDS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && sampler != nullptr) {
				deviceContexts[threadID]->DSSetSamplers(slot, 1, &sampler);
			}
		}
		virtual void BindSamplersDS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr && samplers != nullptr) {
				deviceContexts[threadID]->DSSetSamplers(slot, num, samplers);
			}
		}
		virtual void BindConstantBufferPS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->PSSetConstantBuffers(slot, 1, &buffer);
			}
		}
		virtual void BindConstantBufferVS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->VSSetConstantBuffers(slot, 1, &buffer);

			}
		}
		virtual void BindConstantBufferGS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->GSSetConstantBuffers(slot, 1, &buffer);
			}
		}
		virtual void BindConstantBufferDS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {

				deviceContexts[threadID]->DSSetConstantBuffers(slot, 1, &buffer);
			}
		}
		virtual void BindConstantBufferHS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->HSSetConstantBuffers(slot, 1, &buffer);
			}
		}
		virtual void BindVertexBuffer(BufferResource vertexBuffer, int slot, UINT stride, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				UINT offset = 0;
				deviceContexts[threadID]->IASetVertexBuffers(slot, 1, &vertexBuffer, &stride, &offset);
			}
		}
		virtual void BindIndexBuffer(BufferResource indexBuffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
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
		virtual void BindVertexLayout(VertexLayout layout, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->IASetInputLayout(layout);
			}
		}
		virtual void BindBlendState(BlendState state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				static float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				static UINT sampleMask = 0xffffffff;
				deviceContexts[threadID]->OMSetBlendState(state, blendFactor, sampleMask);
			}
		}
		virtual void BindBlendStateEx(BlendState state, const XMFLOAT4& blendFactor = XMFLOAT4(1, 1, 1, 1), UINT sampleMask = 0xffffffff,
			GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				float fblendFactor[4] = { blendFactor.x, blendFactor.y, blendFactor.z, blendFactor.w };
			}
		}
		virtual void BindDepthStencilState(DepthStencilState state, UINT stencilRef, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->OMSetDepthStencilState(state, stencilRef);
			}
		}
		virtual void BindRasterizerState(RasterizerState state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->RSSetState(state);
			}
		}
		virtual void BindStreamOutTarget(BufferResource buffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				UINT offsetSO[1] = { 0 };
				deviceContexts[threadID]->SOSetTargets(1, &buffer, offsetSO);
			}
		}
		virtual void BindPS(PixelShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->PSSetShader(shader, nullptr, 0);
			}
		}
		virtual void BindVS(VertexShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->VSSetShader(shader, nullptr, 0);
			}
		}
		virtual void BindGS(GeometryShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->GSSetShader(shader, nullptr, 0);
			}
		}
		virtual void BindHS(HullShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->HSSetShader(shader, nullptr, 0);
			}
		}
		virtual void BindDS(DomainShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->DSSetShader(shader, nullptr, 0);
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
			deviceContexts[threadID]->GenerateMips(texture->shaderResourceView);
		}
		virtual void CopyResource(APIResource pDstResource, const APIResource pSrcResource, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				deviceContexts[threadID]->CopyResource(pDstResource, pSrcResource);
			}
		}
		virtual void CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) {
			if (deviceContexts[threadID] != nullptr) {
				SAFE_RELEASE(pDst->shaderResourceView);
				deviceContexts[threadID]->CopyResource(pDst->texture2D, pSrc->texture2D);
				CreateShaderResourceView(pDst);
			}
		}
		virtual void UpdateBuffer(BufferResource& buffer, const void* data, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, int dataSize = -1)
		{
			if (buffer != nullptr && data != nullptr && deviceContexts[threadID] != nullptr) {
				static thread_local D3D11_BUFFER_DESC d3dDesc;
				buffer->GetDesc(&d3dDesc);
				HRESULT hr;
				if (dataSize > (int)d3dDesc.ByteWidth) { //recreate the buffer if new datasize exceeds buffer size
					buffer->Release();
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

					hr = CreateBuffer(&desc, nullptr, &buffer);
				}
				if (d3dDesc.Usage == D3D11_USAGE_DYNAMIC) {
					static thread_local D3D11_MAPPED_SUBRESOURCE mappedResource;
					void* dataPtr;
					deviceContexts[threadID]->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					dataPtr = (void*)mappedResource.pData;
					memcpy(dataPtr, data, (dataSize >= 0 ? dataSize : d3dDesc.ByteWidth));
					deviceContexts[threadID]->Unmap(buffer, 0);
				}
				else {
					deviceContexts[threadID]->UpdateSubresource(buffer, 0, nullptr, data, 0, 0);
				}
			}
		}

		virtual HRESULT CreateTextureFromFile(const wstring& fileName, Texture2D **ppTexture, bool mipMaps = true, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);

	};

}
