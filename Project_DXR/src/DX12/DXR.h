#pragma once

#include "DX12.h"
#include <d3d12.h>
#include "D3DUtils.h"
#include <random>
#include <DirectXMath.h>

class DX12Renderer;
class DX12VertexBuffer;
class DX12IndexBuffer;
class DX12Material;
class DX12Technique;
class DX12RenderState;
class DX12ConstantBuffer;
class DX12Mesh;
class DX12Texture2D;
class Camera;
class CameraController;
struct SceneConstantBuffer;

namespace DXRGlobalRootParam {
	enum Slot {
		FLOAT_RED_CHANNEL = 0,
		SRV_ACCELERATION_STRUCTURE,
		CBV_SCENE_BUFFER,
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
		SRV_VERTEX_BUFFER = 0,
		SRV_INDEX_BUFFER,
		DT_TEXTURES,
		CBV_MATERIAL,
		CBV_SETTINGS,
		SIZE
	};
}
namespace DXRMissRootParam {
	enum Slot {
		SRV_SKYBOX = 0,
		SIZE
	};
}

class DXR {
public:
	DXR(DX12Renderer* renderer);
	virtual ~DXR();

	void init(ID3D12GraphicsCommandList4* cmdList);
	void updateAS(ID3D12GraphicsCommandList4* cmdList);
	void doTheRays(ID3D12GraphicsCommandList4* cmdList);
	void doTemporalAccumulation(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* renderTarget);
	void copyOutputTo(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* target);

	void setMeshes(const std::vector<std::unique_ptr<DX12Mesh>>& meshes);
	void setSkyboxTexture(DX12Texture2D* texture);
	void updateBLASnextFrame(bool inPlace = true);
	void updateTLASnextFrame(std::function<DirectX::XMFLOAT3X4(int)> instanceTransform = {});
	void useCamera(Camera* camera);
	void reloadShaders();

	int& getRTFlags();
	float& getAORadius();
	UINT& getNumAORays();
	UINT& getNumGISamples();
	UINT& getNumGIBounces();
	UINT& getTemporalAccumulationCount();

private:
	void createAccelerationStructures(ID3D12GraphicsCommandList4* cmdList);
	void createShaderResources();
	void createShaderTables();
	void createBLAS(ID3D12GraphicsCommandList4* cmdList, bool onlyUpdate = false);
	void createTLAS(ID3D12GraphicsCommandList4* cmdList, std::function<DirectX::XMFLOAT3X4(int)> instanceTransform = [](int) {DirectX::XMFLOAT3X4 m; DirectX::XMStoreFloat3x4(&m, DirectX::XMMatrixIdentity()); return m;});
	void createRaytracingPSO();
	void createDxrGlobalRootSignature();
	ID3D12RootSignature* createRayGenLocalRootSignature();
	ID3D12RootSignature* createHitGroupLocalRootSignature();
	ID3D12RootSignature* createMissLocalRootSignature();
	void createTemporalAccumulationResources(ID3D12GraphicsCommandList4* cmdList);

private:
	bool m_updateBLAS;
	bool m_newInPlace;

	bool m_updateTLAS;
	bool m_numMeshesChanged;
	std::function<DirectX::XMFLOAT3X4(int)> m_newInstanceTransform;

private:
	DX12Renderer* m_renderer;

	const std::vector<std::unique_ptr<DX12Mesh>>* m_meshes;
	//DX12Mesh* m_mesh; // Not owned by DXR. TODO: support multiple meshes
	std::unique_ptr<DX12ConstantBuffer> m_sceneCB; // Temporary constant buffer
	std::unique_ptr<DX12ConstantBuffer> m_rayGenSettingsCB; // Temporary constant buffer

	Camera* m_camera;
	
	union AlignedSceneConstantBuffer { 	// TODO: Use this instead of SceneConstantBuffer
		SceneConstantBuffer* constants;
		uint8_t alignmentPadding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
	};
	AlignedSceneConstantBuffer* m_mappedSceneCBData; // TODO: Fix memory leak
	SceneConstantBuffer* m_sceneCBData; // TODO: Fix memory leak
	RayGenSettings m_rayGenCBData;

	struct AccelerationStructureBuffers {
		wComPtr<ID3D12Resource1> scratch = nullptr;
		wComPtr<ID3D12Resource1> result = nullptr;
		wComPtr<ID3D12Resource1> instanceDesc = nullptr;    // Used only for top-level AS
	};
	std::vector<AccelerationStructureBuffers> m_DXR_BottomBuffers;
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
	struct MeshHandles {
		D3D12_GPU_VIRTUAL_ADDRESS vertexBufferHandle;
		D3D12_GPU_VIRTUAL_ADDRESS indexBufferHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandle;
		D3D12_GPU_VIRTUAL_ADDRESS materialHandle;
	};

	wComPtr<ID3D12DescriptorHeap> m_rtDescriptorHeap = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtHeapCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtHeapGPUHandle;
	UINT m_heapIncr;
	ResourceWithDescriptor m_rtOutputUAV;

	std::vector<MeshHandles> m_rtMeshHandles;

	//D3D12_GPU_VIRTUAL_ADDRESS m_vb_GPU = {};

	const WCHAR* m_rayGenName = L"rayGen";
	const WCHAR* m_closestHitName = L"closestHit";
	const WCHAR* m_missName = L"miss";
	const WCHAR* m_hitGroupName = L"HitGroup";

	wComPtr<ID3D12RootSignature> m_dxrGlobalRootSignature;
	wComPtr<ID3D12RootSignature> m_localSignatureRayGen;
	wComPtr<ID3D12RootSignature> m_localSignatureHitGroup;
	wComPtr<ID3D12RootSignature> m_localSignatureMiss;

	DX12Texture2D* m_skyboxTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_skyboxGPUDescHandle;

	// Temporal Accumulation
	std::unique_ptr<DX12VertexBuffer> m_taVb;
	std::unique_ptr<DX12IndexBuffer> m_taIb;
	std::unique_ptr<DX12Mesh> m_taMesh;
	std::unique_ptr<DX12Material> m_taMaterial;
	std::unique_ptr<DX12Technique> m_taTechnique;
	//std::vector<wComPtr<ID3D12Resource1>> m_taLastFrames;
	wComPtr<ID3D12Resource1> m_taLastFrameBuffer;
	wComPtr<ID3D12DescriptorHeap> m_taSrvDescriptorHeap = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_taSrvGPUHandle;
	struct TAConstantBufferData {
		UINT accumCount;
	};
	TAConstantBufferData m_taCBData;
	DirectX::XMMATRIX m_camViewMat; // Copy of the camera view matrix
	std::random_device m_rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 m_gen; //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<> m_dis;
};