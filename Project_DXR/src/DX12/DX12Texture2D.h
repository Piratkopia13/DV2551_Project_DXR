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

private:
	DX12Renderer* m_renderer;

	unsigned char* m_rgba;

	wComPtr<ID3D12Resource> m_textureBuffer;
	wComPtr<ID3D12DescriptorHeap> m_mainDescriptorHeap;
	wComPtr<ID3D12Resource> m_textureBufferUploadHeap;
};

