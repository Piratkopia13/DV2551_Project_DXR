#include "pch.h"
#include "DXILShaderCompiler.h"

DXILShaderCompiler::DXILShaderCompiler() 
	: m_linker(nullptr)
	, m_library(nullptr)
	, m_includeHandler(nullptr)
	, m_compiler(nullptr)
{

}

DXILShaderCompiler::~DXILShaderCompiler() {
	SafeRelease(m_compiler);
	SafeRelease(m_library);
	SafeRelease(m_includeHandler);
	SafeRelease(m_linker);
}

HRESULT DXILShaderCompiler::init() {
#ifdef _WIN64
	HMODULE dll = LoadLibraryA("dxcompiler_x64.dll");
#else
	HMODULE dll = LoadLibraryA("dxcompiler_x86.dll");
#endif

	DxcCreateInstanceProc pfnDxcCreateInstance = DxcCreateInstanceProc(GetProcAddress(dll, "DxcCreateInstance"));
	HRESULT hr = E_FAIL;

	if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)))) {
		if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library)))) {
			if (SUCCEEDED(m_library->CreateIncludeHandler(&m_includeHandler))) {
				if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(&m_linker)))) {

				}
			}
		}
	}

	return hr;
}

HRESULT DXILShaderCompiler::compileFromFile(DXILShaderCompiler::Desc* desc, IDxcBlob** ppResult) {
	HRESULT hr = E_FAIL;

	if (desc) {
		IDxcBlobEncoding* source = nullptr;
		if (SUCCEEDED(hr = m_library->CreateBlobFromFile(desc->FilePath, nullptr, &source))) {
			IDxcOperationResult* pResult = nullptr;
			if (SUCCEEDED(hr = m_compiler->Compile(
				source,									// program text
				desc->FilePath,							// file name, mostly for error messages
				desc->EntryPoint,						// entry point function
				desc->TargetProfile,					// target profile
				desc->CompileArguments.data(),          // compilation arguments
				(UINT)desc->CompileArguments.size(),	// number of compilation arguments
				desc->Defines.data(),					// define arguments
				(UINT)desc->Defines.size(),				// number of define arguments
				m_includeHandler,						// handler for #include directives
				&pResult))) {
				HRESULT hrCompile = E_FAIL;
				if (SUCCEEDED(hr = pResult->GetStatus(&hrCompile))) {
					if (SUCCEEDED(hrCompile)) {
						if (ppResult) {
							pResult->GetResult(ppResult);
							hr = S_OK;
						} else {
							hr = E_FAIL;
						}
					} else {
						IDxcBlobEncoding* pPrintBlob = nullptr;
						if (SUCCEEDED(pResult->GetErrorBuffer(&pPrintBlob))) {
							// We can use the library to get our preferred encoding.
							IDxcBlobEncoding* pPrintBlob16 = nullptr;
							m_library->GetBlobAsUtf16(pPrintBlob, &pPrintBlob16);

							MessageBox(0, (LPCWSTR)pPrintBlob16->GetBufferPointer(), L"", 0);

							SafeRelease(pPrintBlob);
							SafeRelease(pPrintBlob16);
						}
					}

					SafeRelease(pResult);
				}
			}
		}
	}

	if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
		std::string errMsg("Shader not found");
		MessageBoxA(0, errMsg.c_str(), "", 0);
		OutputDebugStringA(errMsg.c_str());

	}

	return hr;
}