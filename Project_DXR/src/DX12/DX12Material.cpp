#include "pch.h"
#include "DX12Material.h"

typedef unsigned int uint;

/*
	vtx_shader is the basic shader text coming from the .vs file.
	strings will be added to the shader in this order:
	// - version of HLSL
	// - defines from map
	// - existing shader code
*/
std::string DX12Material::expandShaderText(std::string& shaderSource, ShaderType type) {

	std::stringstream ss;
	ss << "\n\n // hai \n\0";
	for (auto define : shaderDefines[type])
		ss << define;
	ss << shaderSource;

	return ss.str();
};

DX12Material::DX12Material(const std::string& name, DX12Renderer* renderer)
	: m_materialName(name)
	, m_renderer(renderer)
{
	isValid = false;
};

DX12Material::~DX12Material() {

	delete[] m_inputElementDesc;
	for (auto cb : m_constantBuffers) {
		delete cb.second;
	}
};

void DX12Material::setDiffuse(Color c) {

}

void DX12Material::setShader(const std::string& shaderFileName, ShaderType type) {
	if (shaderFileNames.find(type) != shaderFileNames.end()) {
		removeShader(type);
	}
	shaderFileNames[type] = shaderFileName;
};

// this constant buffer will be bound every time we bind the material
void DX12Material::addConstantBuffer(std::string name, unsigned int location) {
	m_constantBuffers[location] = new DX12ConstantBuffer(name, location, m_renderer);
}

std::vector<DX12ConstantBuffer*> DX12Material::getConstantBuffers() const {
	std::vector<DX12ConstantBuffer*> cbuffers;

	for (auto buffer : m_constantBuffers)
		cbuffers.push_back(buffer.second);

	return cbuffers;
}

// location identifies the constant buffer in a unique way
void DX12Material::updateConstantBuffer(const void* data, size_t size, unsigned int location) {
	m_constantBuffers[location]->setData(data, size, this, location);
}

void DX12Material::removeShader(ShaderType type) {
	auto shader = m_shaderBlobs[(int)type];
	// check if name exists (if it doesn't there should not be a shader here.
	if (shaderFileNames.find(type) == shaderFileNames.end()) {
		assert(shader == 0);
		return;
	} else if (shader != 0) {
		shader->Release();
	};
};

ID3DBlob* DX12Material::getShaderBlob(Material::ShaderType type) {
	assert(isValid);
	return m_shaderBlobs[(int)type].Get();
}

D3D12_INPUT_LAYOUT_DESC& DX12Material::getInputLayoutDesc() {
	return m_inputLayoutDesc;
}

int DX12Material::compileShader(ShaderType type) {

	// open the file and read it to a string "shaderText"
	std::ifstream shaderFile(shaderFileNames[type]);
	std::string shaderText;
	if (shaderFile.is_open()) {
		shaderText = std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
		shaderFile.close();
	} else {
		std::string err = "Cannot find file: " + shaderFileNames[type] + "\n";
		OutputDebugStringA(err.c_str());
		return -1;
	}

	// make final vector<string> with shader source + defines + HLSL version
	// in theory this uses move semantics (compiler does it automagically)
	std::string shaderSource = expandShaderText(shaderText, type);

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	UINT flags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG;
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	std::wstring filename = std::wstring(shaderFileNames[type].begin(), shaderFileNames[type].end());

	HRESULT hr;
	switch (type) {
	case Material::ShaderType::VS:
		hr = D3DCompile(shaderSource.c_str(), shaderSource.length(), nullptr, nullptr, nullptr, "VSMain", "vs_5_1", flags, 0, &shaderBlob, &errorBlob);
		break;
	case Material::ShaderType::PS:
		hr = D3DCompile(shaderSource.c_str(), shaderSource.length(), nullptr, nullptr, nullptr, "PSMain", "ps_5_1", flags, 0, &shaderBlob, &errorBlob);
		break;
	case Material::ShaderType::GS:
		hr = D3DCompile(shaderSource.c_str(), shaderSource.length(), nullptr, nullptr, nullptr, "GSMain", "gs_5_1", flags, 0, &shaderBlob, &errorBlob);
		break;
	case Material::ShaderType::CS:
		hr = D3DCompile(shaderSource.c_str(), shaderSource.length(), nullptr, nullptr, nullptr, "CSMain", "cs_5_1", flags, 0, &shaderBlob, &errorBlob);
		break;
	}

	if (FAILED(hr)) {
		// Print shader compilation error
		if (errorBlob) {
			char* msg = (char*)(errorBlob->GetBufferPointer());
			std::stringstream ss;
			ss << "Failed to compile shader (" << shaderFileNames[type] << ")\n";
			for (size_t i = 0; i < errorBlob->GetBufferSize(); i++) {
				ss << msg[i];
			}
			OutputDebugStringA(ss.str().c_str());

			OutputDebugStringA("\n");
			errorBlob->Release();
		}
		if (shaderBlob)
			shaderBlob->Release();
		ThrowIfFailed(hr);
	}

	m_shaderBlobs[(int)type] = shaderBlob;

	return hr;
}

int DX12Material::compileMaterial(std::string& errString) {
	// remove all shaders.
	removeShader(ShaderType::VS);
	removeShader(ShaderType::PS);

	// compile shaders
	compileShader(ShaderType::VS);
	compileShader(ShaderType::PS);

	////// Input Layout //////
	const unsigned int NUM_ELEMS = 3;
	m_inputElementDesc = new D3D12_INPUT_ELEMENT_DESC[NUM_ELEMS];
	m_inputElementDesc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	m_inputElementDesc[1] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	m_inputElementDesc[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	m_inputLayoutDesc.pInputElementDescs = m_inputElementDesc;
	m_inputLayoutDesc.NumElements = NUM_ELEMS;

	isValid = true;
	return 0;
};

int DX12Material::enable() {

	throw std::exception("The material must be enabled using the other enable method taking one parameter");

	return -1;
}
int DX12Material::enable(ID3D12GraphicsCommandList3* cmdList) {
	if (!isValid)
		return -1;

	for (auto cb : m_constantBuffers) {
		cb.second->bind(this, cmdList);
	}

	return 0;
}
;

void DX12Material::disable() {
};
