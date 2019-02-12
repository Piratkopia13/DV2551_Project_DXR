#pragma once

#define ThrowIfFailed(hr) { \
	if (FAILED(hr)) { \
		throw std::exception(); \
	} \
}

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease) {
	if (*ppInterfaceToRelease != NULL) {
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}

// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease) {
	if (pInterfaceToRelease != nullptr) {
		pInterfaceToRelease->Release();

		pInterfaceToRelease = nullptr;
	}
}

template <typename T>
using wComPtr = Microsoft::WRL::ComPtr<T>;