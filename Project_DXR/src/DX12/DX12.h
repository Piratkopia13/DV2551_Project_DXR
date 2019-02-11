#pragma once

#include <wrl.h>
#include <d3d12.h>
#include "d3dx12.h"

#define ThrowIfFailed(hr) { \
	if (FAILED(hr)) { \
		throw std::exception(); \
	} \
}

template<class Interface>
inline void SafeRelease(
	Interface **ppInterfaceToRelease) {
	if (*ppInterfaceToRelease != NULL) {
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}

template <typename T>
using wComPtr = Microsoft::WRL::ComPtr<T>;