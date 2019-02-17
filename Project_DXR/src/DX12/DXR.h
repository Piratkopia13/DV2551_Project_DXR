#pragma once

#include "DX12.h"
#include <d3d12.h>
#include "D3DUtils.h"

class DX12Renderer;
class DX12VertexBuffer;

namespace DXRGlobalRootParam {
	enum Slot {
		FLOAT_RED_CHANNEL = 0,
		SRV_ACCELERATION_STRUCTURE,
		SRV_VERTEX_BUFFER,
		SIZE
	};
}
namespace DXRRayGenRootParam {
	enum Slot {
		DT_UAV_OUTPUT = 0,
		SIZE
	};
}
namespace DXRHitGroupRootParam {
	enum Slot {
		FLOAT3_SHADER_TABLE_COLOR = 0,
		SIZE
	};
}
namespace DXRMissRootParam {
	enum Slot {
	};
}

class DXR {
public:
	DXR(DX12Renderer* renderer);
	virtual ~DXR();

	void init(ID3D12GraphicsCommandList4* cmdList);
	void doTheRays(ID3D12GraphicsCommandList4* cmdList);
	void copyOutputTo(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* target);

	void updateTLAS(ID3D12GraphicsCommandList4* cmdList, std::function<DirectX::XMFLOAT3X4(int)> instanceTransform = {}, UINT instanceCount = 3);

protected:


private:
	void createAccelerationStructures(ID3D12GraphicsCommandList4* cmdList);
	void createShaderResources();
	void createShaderTables();
	void createBLAS(ID3D12GraphicsCommandList4* cmdList, DX12VertexBuffer* vb);
	void createTLAS(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource1* blas);
	void createRaytracingPSO();
	void createDxrGlobalRootSignature();
	ID3D12RootSignature* createRayGenLocalRootSignature();
	ID3D12RootSignature* createHitGroupLocalRootSignature();
	ID3D12RootSignature* createMissLocalRootSignature();

private:
	DX12Renderer* m_renderer;

	DX12VertexBuffer* m_vb; // TODO: support multiple vertex buffers

	struct AccelerationStructureBuffers {
		wComPtr<ID3D12Resource1> scratch = nullptr;
		wComPtr<ID3D12Resource1> result = nullptr;
		wComPtr<ID3D12Resource1> instanceDesc = nullptr;    // Used only for top-level AS
	};
	AccelerationStructureBuffers m_DXR_BottomBuffers{};
	AccelerationStructureBuffers m_DXR_TopBuffers{};

	wComPtr<ID3D12StateObject> m_rtPipelineState = nullptr;

	D3DUtils::ShaderTableData m_rayGenShaderTable{};
	D3DUtils::ShaderTableData m_missShaderTable{};
	D3DUtils::ShaderTableData m_hitGroupShaderTable{};

	struct ResourceWithDescriptor {
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		wComPtr<ID3D12Resource> resource;
	};

	wComPtr<ID3D12DescriptorHeap> m_rtDescriptorHeap = {};

	ResourceWithDescriptor m_rtOutputUAV;

	//D3D12_GPU_VIRTUAL_ADDRESS m_vb_GPU = {};

	const WCHAR* m_rayGenName = L"rayGen";
	const WCHAR* m_closestHitName = L"closestHit";
	const WCHAR* m_missName = L"miss";
	const WCHAR* m_hitGroupName = L"HitGroup";

	wComPtr<ID3D12RootSignature> m_dxrGlobalRootSignature;
	wComPtr<ID3D12RootSignature> m_localSignatureRayGen;
	wComPtr<ID3D12RootSignature> m_localSignatureHitGroup;
	wComPtr<ID3D12RootSignature> m_localSignatureMiss;

};