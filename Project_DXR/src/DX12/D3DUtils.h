#pragma once

#include "DX12.h"

namespace D3DUtils {

	void UpdateDefaultBufferData(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* data,
		UINT64 byteSize,
		UINT64 offset,
		ID3D12Resource1* defaultBuffer,
		ID3D12Resource1** uploadBuffer
	);

};

