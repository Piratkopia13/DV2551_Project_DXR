#pragma once

#include "../Core/Renderer.h"
#include "DX12.h"

#ifdef _DEBUG
#include <initguid.h>
#include <DXGIDebug.h>
#endif
#include "Win32Window.h"
#include <memory>
#include <dxgi1_6.h> //Only used for initialization of the device and swap chain.
#include <d3dcompiler.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class DX12Mesh;
class DX12Technique;

namespace GlobalRootParam {
	enum Slot {
		CBV_TRANSLATION = 0,
		CBV_DIFFUSE_TINT,
		DT_SRVS,
		DT_SAMPLERS
	};
}
namespace DXRGlobalRootParam {
	enum Slot {
		FLOAT_RED_CHANNEL = 0,
		SRV_ACCELERATION_STRUCTURE,
	};
}
namespace DXRRayGenRootParam {
	enum Slot {
		DT_UAV_OUTPUT = 0
	};
}
namespace DXRHitGroupRootParam {
	enum Slot {
		FLOAT3_SHADER_TABLE_COLOR = 0
	};
}
namespace DXRMissRootParam {
	enum Slot {
	};
}

class DX12Renderer : public Renderer {

public:
	DX12Renderer();
	~DX12Renderer();

	virtual Material* makeMaterial(const std::string& name) override;
	virtual Mesh* makeMesh() override;
	virtual VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) override;
	virtual ConstantBuffer* makeConstantBuffer(std::string NAME, unsigned int location) override;
	virtual RenderState* makeRenderState() override;
	virtual Technique* makeTechnique(Material* m, RenderState* r) override;
	virtual Texture2D* makeTexture2D() override;
	virtual Sampler2D* makeSampler2D() override;

	virtual std::string getShaderPath() override;
	virtual std::string getShaderExtension() override;
	ID3D12Device5* getDevice() const;
	ID3D12CommandQueue* getCmdQueue() const;
	ID3D12GraphicsCommandList3* getCmdList() const;
	ID3D12RootSignature* getRootSignature() const;
	ID3D12CommandAllocator* getCmdAllocator() const;
	UINT getNumSwapBuffers() const;
	UINT getFrameIndex() const;

	ID3D12Resource1* createBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);
	void setResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);

	ID3D12DescriptorHeap* getSamplerDescriptorHeap() const;

	virtual int initialize(unsigned int width = 640, unsigned int height = 480) override;
	virtual void setWinTitle(const char* title) override;
	virtual int shutdown() override;

	virtual void setClearColor(float, float, float, float) override;
	virtual void clearBuffer(unsigned int) override;
	//	void setRenderTarget(RenderTarget* rt); // complete parameters
	virtual void setRenderState(RenderState* ps) override;
	virtual void submit(Mesh* mesh) override;
	virtual void frame() override;
	virtual void present() override;
	
	//void addCbvSrvUavDescriptor();
	//void addSamplerDescriptor();

	void waitForGPU();
	void reportLiveObjects();
	
private:
	void createDevice();
	void createCmdInterfacesAndSwapChain();
	void createFenceAndEventHandle();
	void createRenderTargets();
	void createGlobalRootSignature();

	// DXR
	void checkRayTracingSupport();
	void createAccelerationStructures();
	void createDxrGlobalRootSignature();
	void createShaderResources();
	void createShaderTables();
	void createBLAS(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource1* vb);
	void createTLAS(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource1* blas);
	void createRaytracingPSO();
	ID3D12RootSignature* createRayGenLocalRootSignature();
	ID3D12RootSignature* createHitGroupLocalRootSignature();
	ID3D12RootSignature* createMissLocalRootSignature();

	// Multithreading
	void workerThread(unsigned int id);
	struct Command {
		wComPtr<ID3D12CommandAllocator> allocator; // Allocator only grows, use multple (one for each thing)
		wComPtr<ID3D12GraphicsCommandList4> list;
	};
private:
	// Only used for initialization
	IDXGIFactory6* m_factory;


	std::unique_ptr<Win32Window> m_window;
	bool m_globalWireframeMode;
	float m_clearColor[4];
	bool m_firstFrame;
	
	static const UINT NUM_SWAP_BUFFERS;
	static const UINT MAX_NUM_SAMPLERS;

	bool m_supportsDXR;

	// DXR stuff
	struct AccelerationStructureBuffers {
		ID3D12Resource1* scratch = nullptr;
		ID3D12Resource1* result = nullptr;
		ID3D12Resource1* instanceDesc = nullptr;    // Used only for top-level AS
	};
	AccelerationStructureBuffers m_DXR_BottomBuffers{};
	uint64_t m_topLevelConservativeSize = 0;
	AccelerationStructureBuffers m_DXR_TopBuffers{};

	ID3D12StateObject* m_rtPipelineState = nullptr;

	struct ShaderTableData {
		UINT64 SizeInBytes;
		UINT32 StrideInBytes;
		ID3D12Resource1* Resource = nullptr;
	};

	ShaderTableData m_rayGenShaderTable{};
	ShaderTableData m_missShaderTable{};
	ShaderTableData m_hitGroupShaderTable{};

	ID3D12DescriptorHeap* m_rtDescriptorHeap = {};

	//D3D12_CPU_DESCRIPTOR_HANDLE m_outputUAV_CPU = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_outputUAV_GPU = {};
	ID3D12Resource* m_mpOutputResource = nullptr;

	//D3D12_CPU_DESCRIPTOR_HANDLE m_rtAcceleration_CPU = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_rtAcceleration_GPU = {};

	const WCHAR* m_rayGenName = L"rayGen";
	const WCHAR* m_closestHitName = L"closestHit";
	const WCHAR* m_missName = L"miss";
	const WCHAR* m_hitGroupName = L"HitGroup";

	static const D3D12_HEAP_PROPERTIES sUploadHeapProperties;
	static const D3D12_HEAP_PROPERTIES sDefaultHeapProps;

	wComPtr<ID3D12RootSignature> m_dxrGlobalRootSignature;


	// DX12 stuff
	wComPtr<ID3D12Device5> m_device;
	wComPtr<ID3D12CommandQueue> m_commandQueue;
	Command m_preCommand;
	Command m_postCommand;
	wComPtr<ID3D12Fence1> m_fence;
	wComPtr<ID3D12DescriptorHeap> m_renderTargetsHeap;
	wComPtr<IDXGISwapChain4> m_swapChain;
	std::vector<wComPtr<ID3D12Resource1>> m_renderTargets;
	wComPtr<ID3D12RootSignature> m_globalRootSignature;

	wComPtr<IDXGIFactory2> m_dxgiFactory;

	wComPtr<ID3D12DescriptorHeap> m_samplerDescriptorHeap;
	UINT m_numSamplerDescriptors;
	UINT m_samplerDescriptorHandleIncrementSize;
	
	UINT m_renderTargetDescriptorSize;
	UINT64 m_fenceValue;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	HANDLE m_eventHandle;

	// Multi threading stuff
	static const UINT NUM_WORKER_THREADS;
	std::vector<std::thread> m_workerThreads;
	std::vector<Command> m_workerCommands;
	std::condition_variable m_workerCondVar;
	std::mutex m_workerMutex;
	std::atomic_uint m_numWorkersFinished;
	std::vector<bool> m_runWorkers;
	std::mutex m_mainMutex;
	std::condition_variable m_mainCondVar;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cdh;
	std::atomic_bool m_running;

	// Upload buffer stuff
	// Currently only one large
	//wComPtr<ID3D12Resource> m_uploadBuffer;
	//size_t m_uploadHeapSize;
	//UINT8* m_pDataBegin = nullptr; // Starting position of upload buffer
	//UINT8* m_pDataCur = nullptr; // Current position of upload buffer
	//UINT8* m_pDataEnd = nullptr; // End position of upload buffer

	std::unordered_map<DX12Technique*, std::vector<DX12Mesh*>> drawList;

};

