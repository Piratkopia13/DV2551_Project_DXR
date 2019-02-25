#include "pch.h"
#include "DX12Renderer.h"

#include "../Core/Technique.h"
#include "DX12Technique.h"
#include "DX12Material.h"
#include "DX12Mesh.h"
#include "DX12RenderState.h"
#include "DX12ConstantBuffer.h"
#include "DX12Texture2D.h"
#include "DX12VertexBuffer.h"
#include "DX12Skybox.h"
#include "D3DUtils.h"

#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx12.h"

#include <guiddef.h>
//#include <pix3.h> // Used for pix events

using namespace DirectX;

#define MULTITHREADED
const UINT DX12Renderer::NUM_WORKER_THREADS = 1;
const UINT DX12Renderer::NUM_SWAP_BUFFERS = 2;
const UINT DX12Renderer::MAX_NUM_SAMPLERS = 1;

DX12Renderer::DX12Renderer()
	: m_renderTargetDescriptorSize(0U)
	, m_fenceValue(0U)
	, m_eventHandle(nullptr)
	, m_globalWireframeMode(false) 
	, m_firstFrame(true)
	, m_numSamplerDescriptors(0U)
	, m_samplerDescriptorHandleIncrementSize(0U)
	, m_supportsDXR(false)
	, m_DXREnabled(false)
{
	m_renderTargets.resize(NUM_SWAP_BUFFERS);
}

DX12Renderer::~DX12Renderer() {
	// Tell workers to shut down
	m_running.store(false);
	{
		// Wake up workers for termination
		std::lock_guard<std::mutex> guard(m_mainMutex);
		for (int i = 0; i < NUM_WORKER_THREADS; i++) {
			m_runWorkers[i] = true;
		}
	}
	m_mainCondVar.notify_all();

	waitForGPU();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	delete m_skybox;

	reportLiveObjects();

	for (std::thread& thread : m_workerThreads)
		thread.join();
}

int DX12Renderer::shutdown() {
	return 0;
}

Mesh* DX12Renderer::makeMesh() {
	return new DX12Mesh(this);
}

Texture2D* DX12Renderer::makeTexture2D() {
	return new DX12Texture2D(this);
}

Sampler2D* DX12Renderer::makeSampler2D() {
	assert(m_numSamplerDescriptors < MAX_NUM_SAMPLERS);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += m_samplerDescriptorHandleIncrementSize * m_numSamplerDescriptors;

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += m_samplerDescriptorHandleIncrementSize * m_numSamplerDescriptors;

	m_numSamplerDescriptors++;

	return new DX12Sampler2D(m_device.Get(), cpuHandle, gpuHandle);
}

ConstantBuffer* DX12Renderer::makeConstantBuffer(std::string NAME, unsigned int location) {
	return new DX12ConstantBuffer(NAME, location, this);
}

std::string DX12Renderer::getShaderPath() {
	return std::string("..\\assets\\shaders\\");
}

std::string DX12Renderer::getShaderExtension() {
	return std::string(".hlsl");
}

ID3D12Device5* DX12Renderer::getDevice() const {
	return m_device.Get();
}

ID3D12CommandQueue* DX12Renderer::getCmdQueue(D3D12_COMMAND_LIST_TYPE type) const {
	switch (type) {
	case D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT:
		return m_directCommandQueue.Get();
	case D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return m_computeCommandQueue.Get();
	case D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY:
		return m_copyCommandQueue.Get();
	default:
		throw(std::exception(("Type: " + std::to_string(type) + " was not a valid type.").c_str()));
		return nullptr;
	}
}

ID3D12GraphicsCommandList4* DX12Renderer::getCmdList() const {
	//assert(m_firstFrame); // Should never be called after initialization
	// Returns the pre command list
	return m_preCommand.list.Get();
}

ID3D12RootSignature* DX12Renderer::getRootSignature() const {
	return m_globalRootSignature.Get();
}

ID3D12CommandAllocator* DX12Renderer::getCmdAllocator() const {
	// Returns the pre command allocator
	return m_preCommand.allocator.Get();
}

UINT DX12Renderer::getNumSwapBuffers() const {
	return NUM_SWAP_BUFFERS;
}

UINT DX12Renderer::getFrameIndex() const {
	return m_swapChain->GetCurrentBackBufferIndex();
}

Win32Window* DX12Renderer::getWindow() const {
	return m_window.get();
}

ID3D12DescriptorHeap* DX12Renderer::getSamplerDescriptorHeap() const {
	return m_samplerDescriptorHeap.Get();
}

void DX12Renderer::enableDXR(bool enable) {
	if (m_supportsDXR) {
		m_DXREnabled = enable;
	}
	else {
		m_DXREnabled = false;
	}
}

bool DX12Renderer::isDXREnabled() const {
	return m_DXREnabled;
}

DXR& DX12Renderer::getDXR() {
	return *m_dxr;
}

VertexBuffer* DX12Renderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) {
	return new DX12VertexBuffer(size, usage, this);
};

Material* DX12Renderer::makeMaterial(const std::string& name) {
	return new DX12Material(name, this);
}

Technique* DX12Renderer::makeTechnique(Material* m, RenderState* r) {
	return new DX12Technique((DX12Material*)m, (DX12RenderState*)r, this);
}

RenderState* DX12Renderer::makeRenderState() {
	DX12RenderState* newRS = new DX12RenderState();
	newRS->setGlobalWireFrame(&m_globalWireframeMode);
	newRS->setWireFrame(false);
	return newRS;
}

void DX12Renderer::setWinTitle(const char* title) {
	size_t size = strlen(title) + 1;
	wchar_t* wtext = new wchar_t[size];

	size_t outSize;
	mbstowcs_s(&outSize, wtext, size, title, size - 1);

	m_window->setWindowTitle(wtext);
	delete[] wtext;
}

int DX12Renderer::initialize(unsigned int width, unsigned int height) {

	m_window = std::make_unique<Win32Window>(GetModuleHandle(NULL), width, height, "");

	// 1. Initalize the window
	if (!m_window->initialize()) {
		OutputDebugString(L"\nFailed to initialize Win32Window\n");
		return 1;
	}

	createDevice();
	createCmdInterfacesAndSwapChain();
	createFenceAndEventHandle();
	createRenderTargets();
	createShaderResources();
	createGlobalRootSignature();
	createDepthStencilResources();

	// Reset pre allocator and command list to prep for initialization commands
	ThrowIfFailed(m_preCommand.allocator->Reset());
	ThrowIfFailed(m_preCommand.list->Reset(m_preCommand.allocator.Get(), nullptr));

	// DXR
	m_supportsDXR = checkRayTracingSupport();
	if (m_supportsDXR) {
		m_dxr = std::make_unique<DXR>(this);
		m_dxr->init(m_preCommand.list.Get());
		m_DXREnabled = true;
	}

	// 7. Viewport and scissor rect
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = (float)width;
	m_viewport.Height = (float)height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.left = (long)0;
	m_scissorRect.right = (long)width;
	m_scissorRect.top = (long)0;
	m_scissorRect.bottom = (long)height;

	OutputDebugString(L"DX12 Initialized.\n");

	// Initialize worker threads
	m_workerThreads.reserve(NUM_WORKER_THREADS);
	m_runWorkers.resize(NUM_WORKER_THREADS);
	m_numWorkersFinished = 0;
	// Atomic boolean telling worker threads to run
	m_running.store(true);
	for (int i = 0; i < NUM_WORKER_THREADS; i++) {
		// Create command allocator and list
		m_workerCommands.push_back(Command());
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_workerCommands.back().allocator)));
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_workerCommands.back().allocator.Get(), nullptr, IID_PPV_ARGS(&m_workerCommands.back().list)));
		m_workerCommands.back().list->Close();
		m_runWorkers[i] = false;
		// Create thread
		m_workerThreads.emplace_back(&DX12Renderer::workerThread, this, i);
	}

	// Skybox
	m_skybox = new DX12Skybox(this);

	// ImGui
	initImGui();

	return 0;
}

/*
 Super simplified implementation of a work submission
 TODO.
*/

void DX12Renderer::submit(Mesh* mesh) {
	assert(drawList.empty()); // Restricted to only one mesh atm (DXR needs to update otherwise)
	drawList[(DX12Technique*)mesh->technique].push_back((DX12Mesh*)mesh);
};

void DX12Renderer::createDevice() {

	DWORD dxgiFactoryFlags = 0;
#ifdef _DEBUG
	//Enable the D3D12 debug layer.
	wComPtr<ID3D12Debug1> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(true);
	}
	wComPtr<IDXGIInfoQueue> dxgiInfoQueue;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())))) {
		dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
	}
#endif
	ThrowIfFailed(
		CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()))
	);

	// 2. Find comlient adapter and create device

	// dxgi1_6 is only needed for the initialization process using the adapter
	m_factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;
	// Create a factory and iterate through all available adapters 
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)));
	for (UINT adapterIndex = 0;; ++adapterIndex) {
		adapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == m_factory->EnumAdapters1(adapterIndex, &adapter)) {
			break; // No more adapters to enumerate
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device5), nullptr))) {
			break;
		}

		SafeRelease(&adapter);
	}
	if (adapter) {
		// Create the actual device
		ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));
		SafeRelease(&adapter);
	} else {
		// Create warp device if no adapter was found
		m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
	}

}

void DX12Renderer::createCmdInterfacesAndSwapChain() {
	// 3. Create command queue/allocator/list

	// Create direct command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_directCommandQueue)));

	// Create compute command queue
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_computeCommandQueue)));

	// Create copy command queue
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_copyCommandQueue)));

	// Create allocators
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_preCommand.allocator)));
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_postCommand.allocator)));
	// Create command lists
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_preCommand.allocator.Get(), nullptr, IID_PPV_ARGS(&m_preCommand.list)));
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_postCommand.allocator.Get(), nullptr, IID_PPV_ARGS(&m_postCommand.list)));

	// Command lists are created in the recording state. Since there is nothing to
	// record right now and the main loop expects it to be closed, we close them
	m_preCommand.list->Close();
	m_postCommand.list->Close();


	// 5. Create swap chain
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = 0;
	scDesc.Height = 0;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.Stereo = FALSE;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = NUM_SWAP_BUFFERS;
	scDesc.Scaling = DXGI_SCALING_NONE;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags = 0;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	IDXGISwapChain1* swapChain1 = nullptr;
	if (SUCCEEDED(m_factory->CreateSwapChainForHwnd(m_directCommandQueue.Get(), *m_window->getHwnd(), &scDesc, nullptr, nullptr, &swapChain1))) {
		if (SUCCEEDED(swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain)))) {
			m_swapChain->Release();
		}
	}

	// No more m_factory using
	SafeRelease(&m_factory);
}

void DX12Renderer::createFenceAndEventHandle() {
	// 4. Create fence
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceValue = 1;
	// Create an event handle to use for GPU synchronization
	m_eventHandle = CreateEvent(0, false, false, 0);
}

void DX12Renderer::createRenderTargets() {
	// Create descriptor heap for render target views
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors = NUM_SWAP_BUFFERS;
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_renderTargetsHeap)));

	// Create resources for the render targets
	m_renderTargetDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_renderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	// One RTV for each frame
	for (UINT n = 0; n < NUM_SWAP_BUFFERS; n++) {
		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, cdh);
		cdh.ptr += m_renderTargetDescriptorSize;
	}
}

void DX12Renderer::createGlobalRootSignature() {
	// 8. Create root signature

	// Define descriptor range(s)
	D3D12_DESCRIPTOR_RANGE descRangeSrv[1];
	descRangeSrv[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRangeSrv[0].NumDescriptors = 1;
	descRangeSrv[0].BaseShaderRegister = 0; // register bX
	descRangeSrv[0].RegisterSpace = 0; // register (bX,spaceY)
	descRangeSrv[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descRangeSampler[1];
	descRangeSampler[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	descRangeSampler[0].NumDescriptors = MAX_NUM_SAMPLERS;
	descRangeSampler[0].BaseShaderRegister = 0; // register bX
	descRangeSampler[0].RegisterSpace = 0; // register (bX,spaceY)
	descRangeSampler[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Create descriptor tables
	D3D12_ROOT_DESCRIPTOR_TABLE dtSrv;
	dtSrv.NumDescriptorRanges = ARRAYSIZE(descRangeSrv);
	dtSrv.pDescriptorRanges = descRangeSrv;

	D3D12_ROOT_DESCRIPTOR_TABLE dtSampler;
	dtSampler.NumDescriptorRanges = ARRAYSIZE(descRangeSampler);
	dtSampler.pDescriptorRanges = descRangeSampler;

	// Create root descriptors
	D3D12_ROOT_DESCRIPTOR rootDescCBV = {};
	rootDescCBV.ShaderRegister = CB_REG_TRANSFORM;
	rootDescCBV.RegisterSpace = 0;
	D3D12_ROOT_DESCRIPTOR rootDescCBV2 = {};
	rootDescCBV2.ShaderRegister = CB_REG_DIFFUSE_TINT;
	rootDescCBV2.RegisterSpace = 0;
	D3D12_ROOT_DESCRIPTOR rootDescCBV3 = {};
	rootDescCBV3.ShaderRegister = CB_REG_CAMERA;
	rootDescCBV3.RegisterSpace = 0;

	// Create root parameters
	D3D12_ROOT_PARAMETER rootParam[GlobalRootParam::SIZE];

	rootParam[GlobalRootParam::CBV_TRANSFORM].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[GlobalRootParam::CBV_TRANSFORM].Descriptor = rootDescCBV;
	rootParam[GlobalRootParam::CBV_TRANSFORM].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[GlobalRootParam::CBV_DIFFUSE_TINT].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[GlobalRootParam::CBV_DIFFUSE_TINT].Descriptor = rootDescCBV2;
	rootParam[GlobalRootParam::CBV_DIFFUSE_TINT].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[GlobalRootParam::CBV_CAMERA].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[GlobalRootParam::CBV_CAMERA].Descriptor = rootDescCBV3;
	rootParam[GlobalRootParam::CBV_CAMERA].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParam[GlobalRootParam::DT_SRVS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[GlobalRootParam::DT_SRVS].DescriptorTable = dtSrv;
	rootParam[GlobalRootParam::DT_SRVS].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParam[GlobalRootParam::DT_SAMPLERS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[GlobalRootParam::DT_SAMPLERS].DescriptorTable = dtSampler;
	rootParam[GlobalRootParam::DT_SAMPLERS].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};
	staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc.MipLODBias = 0.f;
	staticSamplerDesc.MaxAnisotropy = 1;
	staticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplerDesc.MinLOD = 0.f;
	staticSamplerDesc.MaxLOD = FLT_MAX;
	staticSamplerDesc.ShaderRegister = 1;
	staticSamplerDesc.RegisterSpace = 0;
	staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = ARRAYSIZE(rootParam);
	rsDesc.pParameters = rootParam;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers = &staticSamplerDesc;

	// Serialize and create the actual signature
	ID3DBlob* sBlob;
	ID3DBlob* errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sBlob, &errorBlob);
	if (FAILED(hr)) {
		MessageBoxA(0, (char*)errorBlob->GetBufferPointer(), "", 0);
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		ThrowIfFailed(hr);
	}
	ThrowIfFailed(m_device->CreateRootSignature(0, sBlob->GetBufferPointer(), sBlob->GetBufferSize(), IID_PPV_ARGS(&m_globalRootSignature)));
}

bool DX12Renderer::checkRayTracingSupport() {
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5;
	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
	if (FAILED(hr) || features5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
		//MessageBox(0, L"Raytracing is not supported on this device. Make sure your GPU supports DXR (such as Nvidia's Volta or Turing RTX) and you're on the latest drivers. The DXR fallback layer is not supported.", L"Ray Tracing not supported", 0);
		OutputDebugStringA("Raytracing is not supported on this device! Using rasterisation.");
		return false;
	}
	return true;
}

HRESULT DX12Renderer::initImGui() {
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_ImGuiSrvDescHeap)) != S_OK)
		return E_FAIL;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(*getWindow()->getHwnd());
	ImGui_ImplDX12_Init(getDevice(),
		getNumSwapBuffers(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_ImGuiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		m_ImGuiSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

	return S_OK;
}

void DX12Renderer::createShaderResources() {
	// Create descriptor heap for samplers
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.NumDescriptors = MAX_NUM_SAMPLERS;
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap)));
	m_samplerDescriptorHandleIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void DX12Renderer::createDepthStencilResources() {
	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsDescriptorHeap)));

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_window->getWindowWidth(), m_window->getWindowHeight(), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer)
	);
	m_dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");
	m_depthStencilBuffer->SetName(L"Depth/Stencil Resource Buffer");
	m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilDesc, m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	m_dsvDescHandle = m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void DX12Renderer::workerThread(unsigned int id) {

	while (true) {
		{
			// Wait for event from main thread to begin
			std::unique_lock<std::mutex> mlock(m_mainMutex);
			m_mainCondVar.wait(mlock, [&]() { return m_runWorkers[id]; });
			m_runWorkers[id] = false;
		}
		// Stop thread if asked
		if (!m_running) break;

		auto& allocator = m_workerCommands[id].allocator;
		auto& list = m_workerCommands[id].list;

		// Reset
		allocator->Reset();
		list->Reset(allocator.Get(), nullptr);
		
		list->OMSetRenderTargets(1, &m_cdh, true, &m_dsvDescHandle);
		list->SetGraphicsRootSignature(m_globalRootSignature.Get());

		// Iterate part of the drawlist and draw
		auto start = drawList.begin();
		unsigned int load = static_cast<unsigned int>(drawList.size()) / NUM_WORKER_THREADS;
		std::advance(start, id * load);
		auto end = start;
		if (id == NUM_WORKER_THREADS - 1) {
			end = drawList.end();
		} else {
			std::advance(end, load);
		}
		for (auto work = start; work != end; ++work) {
			// Set pipeline
			// TODO: only do this when neccesary
			list->SetPipelineState(work->first->getPipelineState());

			// Enable the technique
			// This binds constant buffers
			work->first->enable(this, list.Get()); 

			// Set topology
			list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//Set necessary states.
			list->RSSetViewports(1, &m_viewport);
			list->RSSetScissorRects(1, &m_scissorRect);

			for (auto mesh : work->second) {
				size_t numberElements = mesh->geometryBuffer.numElements;
				for (auto t : mesh->textures) {
					static_cast<DX12Texture2D*>(t.second)->bind(t.first, list.Get());
				}

				// Bind vertices, normals and UVs
				mesh->bindIAVertexBuffer(list.Get());

				// Bind translation constant buffer
				static_cast<DX12ConstantBuffer*>(mesh->getTransformCB())->bind(work->first->getMaterial(), list.Get());
				// Bind camera data constant buffer
				static_cast<DX12ConstantBuffer*>(mesh->getCameraCB())->bind(work->first->getMaterial(), list.Get());
				// Draw
				list->DrawInstanced(static_cast<UINT>(numberElements), 1, 0, 0);
			}
		}
		list->Close();

		{
			// Notify main thread that rendering subtask is done
			std::lock_guard<std::mutex> guard(m_workerMutex);
			m_numWorkersFinished++;
		}
		m_workerCondVar.notify_all();
	}
}

/*
 Naive implementation, no re-ordering, checking for state changes, etc.
 TODO.
//*/
#ifdef MULTITHREADED
void DX12Renderer::frame() {

	// TODO: check if drawList differs from last frame, if so rebuild DXR acceleration structures


	if(m_firstFrame) {
		//Execute the initialization command list
		ThrowIfFailed(m_preCommand.list->Close());
		ID3D12CommandList* listsToExecute[] = { m_preCommand.list.Get() };
		m_directCommandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

		waitForGPU(); //Wait for GPU to finish.
		m_firstFrame = false;
	}

	auto frameIndex = getFrameIndex();
	// Get the handle for the current render target used as back buffer
	m_cdh = m_renderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	m_cdh.ptr += m_renderTargetDescriptorSize * frameIndex;

	if (!m_DXREnabled) {
		{
			// Notify workers to begin creating command lists
			std::lock_guard<std::mutex> guard(m_mainMutex);
			for (int i = 0; i < NUM_WORKER_THREADS; i++) {
				m_runWorkers[i] = true;
			}
		}
		m_mainCondVar.notify_all();
	}

	// Command list allocators can only be reset when the associated command lists have
	// finished execution on the GPU; fences are used to ensure this (See WaitForGpu method)
	m_preCommand.allocator->Reset();
	m_preCommand.list->Reset(m_preCommand.allocator.Get(), nullptr);
	
	// Indicate that the back buffer will be used as render target.
	D3D12_RESOURCE_BARRIER barrierDesc = {};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = m_renderTargets[frameIndex].Get();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_preCommand.list->ResourceBarrier(1, &barrierDesc);

	m_preCommand.list->OMSetRenderTargets(1, &m_cdh, true, &m_dsvDescHandle);
	m_preCommand.list->ClearRenderTargetView(m_cdh, m_clearColor, 0, nullptr);
	m_preCommand.list->ClearDepthStencilView(m_dsvDescHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	if (!m_DXREnabled) {
		m_preCommand.list->SetGraphicsRootSignature(m_globalRootSignature.Get());
		// Set topology
		m_preCommand.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//Set necessary states.
		m_preCommand.list->RSSetViewports(1, &m_viewport);
		m_preCommand.list->RSSetScissorRects(1, &m_scissorRect);

		m_cam->updateConstantBuffer();
		m_skybox->setCamCB(m_cam->getConstantBuffer());
		m_skybox->render(m_preCommand.list.Get());
	}

	{
		//Execute the pre command list
		m_preCommand.list->Close();
		ID3D12CommandList* listsToExecute[] = { m_preCommand.list.Get() };
		m_directCommandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);
	}

	if (!m_DXREnabled) {
		// Wait for worker threads to finish
		{
			std::unique_lock<std::mutex> mlock(m_workerMutex);
			//OutputDebugStringA("wait\n");
			m_workerCondVar.wait(mlock, [&]() { return m_numWorkersFinished >= NUM_WORKER_THREADS; });
			//OutputDebugStringA("do\n");
			//OutputDebugStringA("All workers finished! Executing worker command lists\n");
			m_numWorkersFinished.store(0);
			// Execute the worker command lists
			ID3D12CommandList* listsToExecute[NUM_WORKER_THREADS];
			//listsToExecute[0] = m_preCommand.list.Get();
			for (int i = 0; i < NUM_WORKER_THREADS; i++) {
				listsToExecute[i] = m_workerCommands[i].list.Get();
			}
			m_directCommandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);
		}
	}
	// Clear submitted draw list
	drawList.clear();

	// Reset post command
	m_postCommand.allocator->Reset();
	m_postCommand.list->Reset(m_postCommand.allocator.Get(), nullptr);


	// DXR
	if (m_DXREnabled) {
		m_dxr->updateAS(m_postCommand.list.Get());
		m_dxr->doTheRays(m_postCommand.list.Get());
		m_dxr->copyOutputTo(m_postCommand.list.Get(), m_renderTargets[frameIndex].Get());
	}

	// ImGui
	{
		m_postCommand.list->OMSetRenderTargets(1, &m_cdh, true, nullptr);
		m_postCommand.list->SetGraphicsRootSignature(m_globalRootSignature.Get());

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
			
		RECT rect;
		GetClientRect(*m_window->getHwnd(), &rect);

		//bool show_demo_window = true;
		//bool show_another_window = true;
		//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		//if (show_demo_window)
			//ImGui::ShowDemoWindow(&show_demo_window);

		// Only display options if the window isn't collapsed
		if (ImGui::Begin("Options")) {
			if (ImGui::TreeNode("Backend Flags")) {
				if (m_supportsDXR) {
					ImGui::Checkbox("DXR Enabled", &m_DXREnabled);
				}
				ImGui::TreePop();
				ImGui::Separator();
			}
		}

		ImGui::End();

		// Set the descriptor heaps
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_ImGuiSrvDescHeap.Get() };
		m_postCommand.list->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_postCommand.list.Get());
	}

	// Indicate that the back buffer will now be used to present
	D3DUtils::setResourceTransitionBarrier(m_postCommand.list.Get(), m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);



	{
		// Close the list to prepare it for execution.
		m_postCommand.list->Close();
		ID3D12CommandList* listsToExecute[] = { m_postCommand.list.Get() };
		m_directCommandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);
	}

};
#else
void DX12Renderer::frame() {

	if (m_firstFrame) {
		//Execute the initialization command list
		ThrowIfFailed(m_preCommand.list->Close());
		ID3D12CommandList* listsToExecute[] = { m_preCommand.list.Get() };
		m_commandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

		waitForGPU(); //Wait for GPU to finish.
		m_firstFrame = false;
	}

	// Command list allocators can only be reset when the associated command lists have
	// finished execution on the GPU; fences are used to ensure this (See WaitForGpu method)
	m_preCommand.allocator->Reset();
	
	m_preCommand.list->Reset(m_preCommand.allocator.Get(), nullptr);
	
	// Indicate that the back buffer will be used as render target.
	D3D12_RESOURCE_BARRIER barrierDesc = {};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = m_renderTargets[getFrameIndex()].Get();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_preCommand.list->ResourceBarrier(1, &barrierDesc);

	// Record commands
	// Get the handle for the current render target used as back buffer
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_renderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += m_renderTargetDescriptorSize * getFrameIndex();

	m_preCommand.list->OMSetRenderTargets(1, &cdh, true, nullptr);
	m_preCommand.list->ClearRenderTargetView(cdh, m_clearColor, 0, nullptr);

	// Set root signature
	m_preCommand.list->SetGraphicsRootSignature(m_globalRootSignature.Get());

	for (auto work : drawList) {
		// Set pipeline
		// TODO: only do this when neccesary
		m_preCommand.list->SetPipelineState(work.first->getPipelineState());

		// Enable the technique
		// This binds constant buffers
		work.first->enable(this, m_preCommand.list.Get());

		// Set topology
		m_preCommand.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//Set necessary states.
		m_preCommand.list->RSSetViewports(1, &m_viewport);
		m_preCommand.list->RSSetScissorRects(1, &m_scissorRect);

		for (auto mesh : work.second) {
			size_t numberElements = mesh->geometryBuffers[0].numElements;
			for (auto t : mesh->textures) {
				static_cast<DX12Texture2D*>(t.second)->bind(t.first, m_preCommand.list.Get());
			}

			// Bind vertices, normals and UVs
			for (auto element : mesh->geometryBuffers) {
				mesh->bindIAVertexBuffer(element.first, m_preCommand.list.Get());
			}
			// Bind translation constant buffer
			static_cast<DX12ConstantBuffer*>(mesh->txBuffer)->bind(work.first->getMaterial(), m_preCommand.list.Get());
			// Draw
			m_preCommand.list->DrawInstanced(static_cast<UINT>(numberElements), 1, 0, 0);
		}

	}
	drawList.clear();

	// Indicate that the back buffer will now be used to present
	barrierDesc = {};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = m_renderTargets[getFrameIndex()].Get();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_preCommand.list->ResourceBarrier(1, &barrierDesc);

	//Close the list to prepare it for execution.
	m_preCommand.list->Close();

	//Execute the command list.
	ID3D12CommandList* listsToExecute[] = { m_preCommand.list.Get() };
	m_commandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

};
#endif
void DX12Renderer::present() {
	waitForGPU(); //Wait for GPU to finish.
				  //NOT BEST PRACTICE, only used as such for simplicity.

	//Present the frame.
	DXGI_PRESENT_PARAMETERS pp = {};
	m_swapChain->Present1(0, 0, &pp);


}

void DX12Renderer::useCamera(Camera * camera) {
	m_cam = camera;

	if(m_cam->getConstantBuffer() == nullptr)
		m_cam->init(this);
}

void DX12Renderer::waitForGPU() {
	//WAITING FOR EACH FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	//This is code implemented as such for simplicity. The cpu could for example be used
	//for other tasks to prepare the next frame while the current one is being rendered.

	//Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	m_directCommandQueue->Signal(m_fence.Get(), fence);
	m_fenceValue++;

	//Wait until command queue is done.
	if (m_fence->GetCompletedValue() < fence) {
		m_fence->SetEventOnCompletion(fence, m_eventHandle);
		WaitForSingleObject(m_eventHandle, INFINITE);
	}
}

void DX12Renderer::reportLiveObjects() {
#ifdef _DEBUG
	OutputDebugStringA("== REPORT LIVE OBJECTS ==\n");
	wComPtr<IDXGIDebug1> dxgiDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
		ThrowIfFailed(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL)));
	}
	OutputDebugStringA("=========================\n");
#endif
}

void DX12Renderer::setClearColor(float r, float g, float b, float a) {
	m_clearColor[0] = r;
	m_clearColor[1] = g;
	m_clearColor[2] = b;
	m_clearColor[3] = a;
};

void DX12Renderer::clearBuffer(unsigned int flag) {
	// TODO?
};

void DX12Renderer::setRenderState(RenderState* ps) {
	// doesnt do anything
	ps->set();
};
