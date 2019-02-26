#pragma once

#include <d3d12.h>

#include "DX12.h"
#include <vector>

class DX12Renderer;
class DX12Sampler2D;

class DX12Texture2DArray {
public:
	DX12Texture2DArray(DX12Renderer* renderer);
	~DX12Texture2DArray();

	HRESULT loadFromFiles(std::vector<std::string> filenames);

	void bind(ID3D12GraphicsCommandList3* cmdList);
	
	DXGI_FORMAT getFormat();
	UINT getMips();
	ID3D12Resource* getResource();
	D3D12_CPU_DESCRIPTOR_HANDLE getCpuDescHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE getGpuDescHandle();
	D3D12_SHADER_RESOURCE_VIEW_DESC getSRVDesc();

	void setSampler(DX12Sampler2D* sampler);

private:
	DX12Renderer* m_renderer;

	unsigned char* m_rgba;
	std::vector<unsigned char> m_rgbaVec;
	D3D12_RESOURCE_DESC m_textureDesc;
	wComPtr<ID3D12Resource> m_textureBuffer;
	wComPtr<ID3D12DescriptorHeap> m_mainDescriptorHeap;
	wComPtr<ID3D12Resource> m_textureBufferUploadHeap;

	DX12Sampler2D* m_sampler;

	D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc;
};