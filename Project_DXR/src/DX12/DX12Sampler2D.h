#pragma once
#include "../Core/Sampler2D.h"

#include "DX12.h"

class DX12Sampler2D : public Sampler2D
{
public:
	DX12Sampler2D(ID3D12Device4* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);
	~DX12Sampler2D();

	void setMagFilter(FILTER filter);
	void setMinFilter(FILTER filter);
	void setWrap(WRAPPING u, WRAPPING v);

	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle();


private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
	D3D12_SAMPLER_DESC m_samplerDesc;
	ID3D12Device4* m_device;

};

