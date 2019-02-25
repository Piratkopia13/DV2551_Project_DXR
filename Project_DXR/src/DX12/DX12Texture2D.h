#pragma once
#include "../Core/Texture2D.h"
#include "DX12.h"

#include "DX12Renderer.h"
#include "DX12Sampler2D.h"


class DX12Texture2D : public Texture2D {
public:
	DX12Texture2D(DX12Renderer* renderer);
	~DX12Texture2D();

	virtual int loadFromFile(std::string filename) override;
	virtual void bind(unsigned int slot) override;
	void bind(unsigned int slot, ID3D12GraphicsCommandList3* cmdList);

	DXGI_FORMAT getFormat();
	UINT getMips();
	ID3D12Resource* getResource();
	D3D12_CPU_DESCRIPTOR_HANDLE getCpuDescHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE getGpuDescHandle();

private:
	DX12Renderer* m_renderer;

	unsigned char* m_rgba;
	D3D12_RESOURCE_DESC m_textureDesc;
	wComPtr<ID3D12Resource> m_textureBuffer;
	wComPtr<ID3D12DescriptorHeap> m_mainDescriptorHeap;
	wComPtr<ID3D12Resource> m_textureBufferUploadHeap;
};

