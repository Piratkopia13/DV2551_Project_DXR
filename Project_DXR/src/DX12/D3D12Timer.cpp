#include "pch.h"

#include "D3D12Timer.h"

namespace D3D12
{
	D3D12Timer::D3D12Timer()
	{

	}

	// Destructor.
	D3D12Timer::~D3D12Timer()
	{
		if(queryHeap_)
			queryHeap_->Release();

		if(queryResourceCPU_)
			queryResourceCPU_->Release();

		if (queryResourceGPU_)
			queryResourceGPU_->Release();
	}

	HRESULT D3D12Timer::init(ID3D12Device* pDevice, UINT numTimers)
	{
		HRESULT hr = S_OK;
		device_ = pDevice;

		timerCount_ = numTimers;

		D3D12_QUERY_HEAP_DESC queryHeapDesc;
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		queryHeapDesc.NodeMask = 0;
		queryHeapDesc.Count = timerCount_ * 2;

		if(SUCCEEDED(hr = device_->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&queryHeap_))))
		{
			D3D12_RESOURCE_DESC resouceDesc;
			ZeroMemory(&resouceDesc, sizeof(resouceDesc));
			resouceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resouceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			resouceDesc.Width = sizeof(GPUTimestampPair) * timerCount_;
			resouceDesc.Height = 1;
			resouceDesc.DepthOrArraySize = 1;
			resouceDesc.MipLevels = 1;
			resouceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resouceDesc.SampleDesc.Count = 1;
			resouceDesc.SampleDesc.Quality = 0;
			resouceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resouceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			D3D12_HEAP_PROPERTIES heapProp = {};
			heapProp.Type = D3D12_HEAP_TYPE_READBACK;
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProp.CreationNodeMask = 1;
			heapProp.VisibleNodeMask = 1;

			if (SUCCEEDED(hr = device_->CreateCommittedResource(
				&heapProp,
				D3D12_HEAP_FLAG_NONE,
				&resouceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&queryResourceCPU_))
			))
			{
				queryResourceCPU_->SetName(L"queryResourceCPU_");
			}



			//create gpu dest
			//resouceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
			if (SUCCEEDED(hr = device_->CreateCommittedResource(
				&heapProp,
				D3D12_HEAP_FLAG_NONE,
				&resouceDesc,
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				nullptr,
				IID_PPV_ARGS(&queryResourceGPU_))
			))
			{
				queryResourceGPU_->SetName(L"queryResourceGPU_");
			}
		}

		return hr;
	}

	// Start timestamp.
	void D3D12Timer::start(ID3D12GraphicsCommandList* pCommandList, UINT timestampPairIndex)
	{
		active_ = true;

		pCommandList->EndQuery(queryHeap_, D3D12_QUERY_TYPE_TIMESTAMP, timestampPairIndex * 2);
	}

	// Stop timestamp.
	void D3D12Timer::stop(ID3D12GraphicsCommandList* pCommandList, UINT timestampPairIndex)
	{
		active_ = false;

		pCommandList->EndQuery(queryHeap_, D3D12_QUERY_TYPE_TIMESTAMP, timestampPairIndex * 2 + 1);
	}

	// Resolve query data. Write query to device memory. Make sure to wait for query to finsih before resolving data.
	void D3D12Timer::resolveQueryToCPU(ID3D12GraphicsCommandList* pCommandList, UINT timestampPairIndex)
	{
		pCommandList->ResolveQueryData(
			queryHeap_,
			D3D12_QUERY_TYPE_TIMESTAMP,
			timestampPairIndex * 2,
			2,
			queryResourceCPU_,
			sizeof(GPUTimestampPair) * timestampPairIndex
		);
	}

	void D3D12Timer::resolveQueryToCPU(ID3D12GraphicsCommandList* pCommandList, UINT timestampPairIndexFirst, UINT timestampPairIndexLast)
	{
		UINT numToResolve = timestampPairIndexLast - timestampPairIndexFirst;
		pCommandList->ResolveQueryData(
			queryHeap_,
			D3D12_QUERY_TYPE_TIMESTAMP,
			timestampPairIndexFirst * 2,
			numToResolve * 2,
			queryResourceCPU_,
			sizeof(GPUTimestampPair) * timestampPairIndexFirst
		);
	}

	void D3D12Timer::resolveQueryToGPU(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource** ppQueryResourceGPUOut)
	{
		setGPUResourceState(pCommandList, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		
		pCommandList->ResolveQueryData(queryHeap_, D3D12_QUERY_TYPE_TIMESTAMP, 0, timerCount_ * 2, queryResourceGPU_, 0);
		
		setGPUResourceState(pCommandList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);

		if (ppQueryResourceGPUOut)
		{
			*ppQueryResourceGPUOut = queryResourceGPU_;
		}
	}

	void D3D12Timer::setGPUResourceState(ID3D12GraphicsCommandList* pCommandList, D3D12_RESOURCE_STATES prevState, D3D12_RESOURCE_STATES newState)
	{
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = queryResourceGPU_;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = prevState;
		barrierDesc.Transition.StateAfter = newState;

		pCommandList->ResourceBarrier(1, &barrierDesc);
	}

	GPUTimestampPair D3D12Timer::getTimestampPair(UINT timestampPairIndex)
	{
		GPUTimestampPair p{};

		{
			GPUTimestampPair* mapMem = nullptr;
			D3D12_RANGE readRange{ sizeof(p) * timestampPairIndex, sizeof(p) * (timestampPairIndex+1) };
			D3D12_RANGE writeRange{ 0, 0 };
			if (SUCCEEDED(queryResourceCPU_->Map(0, &readRange, (void**)&mapMem)))
			{
				mapMem += timestampPairIndex;
				p = *mapMem;
				queryResourceCPU_->Unmap(0, &writeRange);
			}
		}

		return p;
	}

	// Calcluate time and map memory to CPU.
	void D3D12Timer::calculateTime()
	{
		// Copy to CPU.
		UINT64 timeStamps[2];
		{
			void* mappedResource;
			D3D12_RANGE readRange{ 0, sizeof(UINT64) * timerCount_ * 2 };
			D3D12_RANGE writeRange{ 0, 0 };
			if (SUCCEEDED(queryResourceCPU_->Map(0, &readRange, &mappedResource)))
			{
				memcpy(&timeStamps, mappedResource, sizeof(UINT64) * timerCount_ * 2);
				queryResourceCPU_->Unmap(0, &writeRange);
			}
		}

		beginTime_ = timeStamps[0];
		endTime_ = timeStamps[1];

		//			if (mBeginTime != 0) MessageBoxA(0, "ddd", "", 0);

		deltaTime_ = endTime_ - beginTime_;
	}

	// Get time from start to stop in nano seconds.
	UINT64 D3D12Timer::getDeltaTime()
	{
		return deltaTime_;
	}

	UINT64 D3D12Timer::getEndTime()
	{
		return endTime_;
	}

	UINT64 D3D12Timer::getBeginTime()
	{
		return beginTime_;
	}

	// Whether timer is active.
	bool D3D12Timer::isActive()
	{
		return active_;
	}
}