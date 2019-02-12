#pragma once

#include "DX12.h"
#include <Windows.h>
#include <vector>
#include <dxcapi.h>

class DXILShaderCompiler {
public:

	struct Desc {
		LPCWSTR FilePath;
		LPCWSTR EntryPoint;
		LPCWSTR TargetProfile;
		std::vector<LPCWSTR> CompileArguments;
		std::vector<DxcDefine> Defines;
	};

	DXILShaderCompiler();
	~DXILShaderCompiler();

	HRESULT init();
	HRESULT compileFromFile(Desc* desc, IDxcBlob** ppResult);

private:
	IDxcLibrary* m_library;
	IDxcCompiler* m_compiler;
	IDxcLinker* m_linker;
	IDxcIncludeHandler* m_includeHandler;

};