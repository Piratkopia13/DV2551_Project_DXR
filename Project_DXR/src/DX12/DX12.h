#pragma once

#define ThrowIfFailed(hr) { \
	if (FAILED(hr)) { \
		throw std::exception(); \
	} \
}

#define ThrowIfBlobError(hr, blob) { \
	if (FAILED(hr)) { \
		MessageBoxA(0, (char*)blob->GetBufferPointer(), "", 0); \
		OutputDebugStringA((char*)blob->GetBufferPointer()); \
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

template<class T>
inline void SafeDelete(T*& toDelete) {
	if (toDelete != nullptr) {
		delete toDelete;
		toDelete = nullptr;
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
