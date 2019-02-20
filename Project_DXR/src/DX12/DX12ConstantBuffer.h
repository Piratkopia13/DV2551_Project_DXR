#pragma once
#include "DX12.h"
#include "../Core/ConstantBuffer.h"

class DX12Renderer;

// TODO: Make useable by both DXR and rasterization
class DX12ConstantBuffer : public ConstantBuffer {
public:
	DX12ConstantBuffer(std::string name, unsigned int location, DX12Renderer* renderer);
	~DX12ConstantBuffer();
	virtual void setData(const void* data, size_t size, unsigned int location) override;
	virtual void bind(Material*) override;
	void bind(Material* material, ID3D12GraphicsCommandList3* cmdList);

	ID3D12Resource1* getBuffer(unsigned int frameIndex) const;

private:
	DX12Renderer* m_renderer;

	unsigned int m_location;
	wComPtr<ID3D12Resource1>* m_constantBufferUploadHeap;
	UINT8** m_cbGPUAddress;
	
	std::string m_name;
};

