#pragma once
#include "../Core/Material.h"
#include "DX12.h"
#include <vector>
#include "DX12ConstantBuffer.h"

class DX12Renderer;

class DX12Material : public Material {
	friend DX12Renderer;

public:
	DX12Material(const std::string& name, DX12Renderer* renderer);
	~DX12Material();


	virtual void setShader(const std::string& shaderFileName, ShaderType type) override;
	virtual void removeShader(ShaderType type) override;
	virtual int compileMaterial(std::string& errString) override;
	virtual int enable() override;
	int enable(ID3D12GraphicsCommandList3* cmdList);
	void disable();
	virtual void setDiffuse(Color c) override;

	// location identifies the constant buffer in a unique way
	virtual void updateConstantBuffer(const void* data, unsigned int location) override;
	// slower version using a string
	virtual void addConstantBuffer(std::string name, unsigned int location, size_t size) override;

	std::vector<DX12ConstantBuffer*> getConstantBuffers() const;

	// DX12 specifics
	ID3DBlob* getShaderBlob(Material::ShaderType type);
	D3D12_INPUT_LAYOUT_DESC& getInputLayoutDesc();

private:
	int compileShader(ShaderType type);
	std::string expandShaderText(std::string& shaderText, ShaderType type);

private:
	std::string m_materialName;
	DX12Renderer* m_renderer;
	std::map<unsigned int, DX12ConstantBuffer*> m_constantBuffers;

	Material::ShaderType m_shaderTypes[4];
	std::string m_shaderNames[4];
	// Compiled shader blobs
	wComPtr<ID3DBlob> m_shaderBlobs[4];

	D3D12_INPUT_ELEMENT_DESC* m_inputElementDesc;
	D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;

};

