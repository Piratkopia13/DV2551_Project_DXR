#include "pch.h"
#include "DX12Sampler2D.h"


DX12Sampler2D::DX12Sampler2D(ID3D12Device4* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
	m_samplerDesc = {};
	m_samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	m_samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc.MinLOD = 0;
	m_samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	m_samplerDesc.MipLODBias = 0.0f;
	m_samplerDesc.MaxAnisotropy = 1;
	m_samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	m_CPUHandle = cpuHandle;
	m_GPUHandle = gpuHandle;
	m_device = device;
	m_device->CreateSampler(&m_samplerDesc, m_CPUHandle);
}


DX12Sampler2D::~DX12Sampler2D() {

}


/*
// just some values to play with, not complete!
enum FILTER { POINT_SAMPLER = 0, LINEAR = 0 };
*/

void DX12Sampler2D::setMagFilter(FILTER filter) {
	// No difference
}

void DX12Sampler2D::setMinFilter(FILTER filter) {
	// No difference
}


/*
// just some values to play with, not complete!
enum WRAPPING { REPEAT = 0, CLAMP = 1 };
*/

D3D12_TEXTURE_ADDRESS_MODE D3D12Wrapping[2] = { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP };
void DX12Sampler2D::setWrap(WRAPPING u, WRAPPING v) {
	m_samplerDesc.AddressU = D3D12Wrapping[u];
	m_samplerDesc.AddressV = D3D12Wrapping[v];

	// Make better implementation
	m_device->CreateSampler(&m_samplerDesc, m_CPUHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Sampler2D::getCPUHandle()
{
	return m_CPUHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12Sampler2D::getGPUHandle()
{
	return m_GPUHandle;
}
