#pragma once

#include "../Core/Renderer.h"
#include "DX12.h"
#include "DXR.h"

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
class DX12Skybox;
class Camera;

namespace GlobalRootParam {
	enum Slot {
		CBV_TRANSFORM = 0,
		CBV_DIFFUSE_TINT,
		CBV_CAMERA,
		DT_SRVS,
		DT_SAMPLERS,
		SIZE
	};
}

class DX12Renderer : public Renderer {

public:
	DX12Renderer();
	~DX12Renderer();

	virtual Material* makeMaterial(const std::string& name) override;
	virtual Mesh* makeMesh() override;
	virtual VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) override;
	virtual IndexBuffer* makeIndexBuffer(size_t size, IndexBuffer::DATA_USAGE usage) override;
	virtual ConstantBuffer* makeConstantBuffer(std::string NAME, size_t size) override;
	virtual RenderState* makeRenderState() override;
	virtual Technique* makeTechnique(Material* m, RenderState* r) override;
	virtual Texture2D* makeTexture2D() override;
	virtual Sampler2D* makeSampler2D() override;

	virtual std::string getShaderPath() override;
	virtual std::string getShaderExtension() override;
	ID3D12Device5* getDevice() const;
	ID3D12CommandQueue* getCmdQueue(D3D12_COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT) const;
	ID3D12GraphicsCommandList4* getCmdList() const;
	ID3D12RootSignature* getRootSignature() const;
	ID3D12CommandAllocator* getCmdAllocator() const;
	UINT getNumSwapBuffers() const;
	inline UINT getFrameIndex() const;
	Win32Window* getWindow() const;
	ID3D12DescriptorHeap* getSamplerDescriptorHeap() const;
	
	void enableDXR(bool enable);
	bool& isDXREnabled();
	bool& isDXRSupported();
	DXR& getDXR();

	virtual int initialize(unsigned int width = 640, unsigned int height = 480) override;
	virtual void setWinTitle(const char* title) override;
	virtual int shutdown() override;

	virtual void setClearColor(float, float, float, float) override;
	virtual void clearBuffer(unsigned int) override;
	//	void setRenderTarget(RenderTarget* rt); // complete parameters
	virtual void setRenderState(RenderState* ps) override;
	virtual void submit(Mesh* mesh) override;
	void frame(std::function<void()> imguiFunc = []() {});
	virtual void present() override;

	void useCamera(Camera* camera);
	
	void executeNextOpenPreCommand(std::function<void()> func);

	void waitForGPU();
	void reportLiveObjects();
	
private:
	void createDevice();
	void createCmdInterfacesAndSwapChain();
	void createFenceAndEventHandle();
	void createRenderTargets();
	void createGlobalRootSignature();
	void createShaderResources();
	void createDepthStencilResources();
	void nextFrame();

	// DXR
	bool checkRayTracingSupport();

	// ImGui
	HRESULT initImGui();

	// Multithreading
	void workerThread(unsigned int id);
	struct Command {
		std::vector<wComPtr<ID3D12CommandAllocator>> allocators; // Allocator only grows, use multple (one for each thing)
		wComPtr<ID3D12GraphicsCommandList4> list;
	};
private:
	std::vector<std::function<void()>> m_preCommandFuncsToExecute; // Stored functions to execute on next open pre-command list

	// Only used for initialization
	IDXGIFactory6* m_factory;

	std::unique_ptr<Win32Window> m_window;
	bool m_globalWireframeMode;
	float m_clearColor[4];
	bool m_firstFrame;
	UINT m_backBufferIndex;
	
	static const UINT NUM_SWAP_BUFFERS;
	static const UINT MAX_NUM_SAMPLERS;

	bool m_supportsDXR;
	bool m_DXREnabled;

	// DX12 stuff
	std::unique_ptr<DXR> m_dxr;

	wComPtr<ID3D12Device5> m_device;
	wComPtr<ID3D12CommandQueue> m_directCommandQueue;
	wComPtr<ID3D12CommandQueue> m_computeCommandQueue;
	wComPtr<ID3D12CommandQueue> m_copyCommandQueue;
	Command m_preCommand;
	Command m_postCommand;
	wComPtr<ID3D12Fence1> m_fence;
	wComPtr<ID3D12DescriptorHeap> m_renderTargetsHeap;
	wComPtr<IDXGISwapChain4> m_swapChain;
	std::vector<wComPtr<ID3D12Resource1>> m_renderTargets;
	wComPtr<ID3D12RootSignature> m_globalRootSignature;
	// Depth/Stencil
	wComPtr<ID3D12Resource> m_depthStencilBuffer;
	wComPtr<ID3D12DescriptorHeap> m_dsDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsvDescHandle;

	// ImGui
	wComPtr<ID3D12DescriptorHeap> m_ImGuiSrvDescHeap;

	wComPtr<IDXGIFactory2> m_dxgiFactory;

	wComPtr<ID3D12DescriptorHeap> m_samplerDescriptorHeap;
	UINT m_numSamplerDescriptors;
	UINT m_samplerDescriptorHandleIncrementSize;
	
	UINT m_renderTargetDescriptorSize;
	std::vector<UINT64> m_fenceValues;
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

	DX12Skybox* m_skybox;
	Camera* m_cam;

	// Upload buffer stuff
	// Currently only one large
	//wComPtr<ID3D12Resource> m_uploadBuffer;
	//size_t m_uploadHeapSize;
	//UINT8* m_pDataBegin = nullptr; // Starting position of upload buffer
	//UINT8* m_pDataCur = nullptr; // Current position of upload buffer
	//UINT8* m_pDataEnd = nullptr; // End position of upload buffer

	std::unordered_map<DX12Technique*, std::vector<DX12Mesh*>> drawList;

};

