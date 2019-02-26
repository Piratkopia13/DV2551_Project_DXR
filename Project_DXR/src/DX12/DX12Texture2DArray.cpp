#include "pch.h"
#include "DX12Texture2DArray.h"

#include "DX12Renderer.h"

#include "stb_image.h"
#include "DX12Sampler2D.h"

DX12Texture2DArray::DX12Texture2DArray(DX12Renderer* renderer)
	: m_renderer(renderer)
	, m_rgba(nullptr) 
{}

DX12Texture2DArray::~DX12Texture2DArray() {
	if (m_rgba) {
		delete m_rgba;
		m_rgba = nullptr;
	}
}

HRESULT DX12Texture2DArray::loadFromFiles(std::vector<std::string> filenames) {
	int width, height, bytesPerPixel;
	stbi_info(filenames[0].c_str(), &width, &height, &bytesPerPixel); // assume all files are the same width, height and bpp

	UINT imageByteSize = width * height * bytesPerPixel;
	m_rgbaVec.resize(width * height * bytesPerPixel * filenames.size());
	// Pre-allocate the required memory
	UINT index = 0;
	for (auto file : filenames) {
		int w, h, bpp;
		m_rgba = stbi_load(filenames[0].c_str(), &w, &h, &bpp, STBI_rgb_alpha); // TODO: remove m_rgba memory after copy to gpu is complete
		if (m_rgba == nullptr) {
			return -1;
		}
		assert(w == width && h == height && bpp == bytesPerPixel && "All images has to have the same dimensions and BPP");

		for (UINT i = 0; i < imageByteSize; i++) {
			m_rgbaVec[i + imageByteSize * index] = m_rgba[i];
		}
		index++;

		delete m_rgba;
	}

	m_textureDesc = {};

	m_textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//m_textureDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
	m_textureDesc.DepthOrArraySize = 1;
	m_textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	m_textureDesc.Width = width;
	m_textureDesc.Height = height;
	m_textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	m_textureDesc.MipLevels = 1;
	m_textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	m_textureDesc.SampleDesc.Count = 1;
	m_textureDesc.SampleDesc.Quality = 0;

	// create a default heap where the upload heap will copy its contents into (contents being the texture)
	HRESULT hr = m_renderer->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&m_textureDesc, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&m_textureBuffer));
	if (FAILED(hr))
		return false;
	m_textureBuffer->SetName(L"Texture Buffer Resource Heap");

	UINT64 textureUploadBufferSize;
	// this function gets the size an upload buffer needs to be to upload a texture to the gpu.
	// each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
	// eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
	//textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (m_textureDesc.Height - 1)) + imageBytesPerRow;
	m_renderer->getDevice()->GetCopyableFootprints(&m_textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	// now we create an upload heap to upload our texture to the GPU
	hr = m_renderer->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
		D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
		nullptr,
		IID_PPV_ARGS(&m_textureBufferUploadHeap));
	if (FAILED(hr))
		return false;

	m_textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = m_rgba; // pointer to our image data
	textureData.RowPitch = width * bytesPerPixel; /// size of all our triangle vertex data
	textureData.SlicePitch = textureData.RowPitch * height; /// also the size of our triangle vertex data

	// Now we copy the upload buffer contents to the default heap


	UpdateSubresources(m_renderer->getCmdList(), m_textureBuffer.Get(), m_textureBufferUploadHeap.Get(), 0, 0, 1, &textureData);

	// transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
	m_renderer->getCmdList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// create the descriptor heap that will store our srv
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = m_renderer->getDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_mainDescriptorHeap));
	if (FAILED(hr))
		return -1;

	// now we create a shader resource view (descriptor that points to the texture and describes it)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2D.MipLevels = 1;
	m_renderer->getDevice()->CreateShaderResourceView(m_textureBuffer.Get(), &srvDesc, m_mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	return E_NOTIMPL;
}

void DX12Texture2DArray::bind(ID3D12GraphicsCommandList3* cmdList) {
	if (m_rgbaVec.size() > 0) {
		m_rgbaVec.clear();
	}

	// Set the descriptor heaps
	if (m_sampler != nullptr) {
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_mainDescriptorHeap.Get(), m_renderer->getSamplerDescriptorHeap() };
		cmdList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

		cmdList->SetGraphicsRootDescriptorTable(GlobalRootParam::DT_SAMPLERS, m_sampler->getGPUHandle());
		cmdList->SetGraphicsRootDescriptorTable(GlobalRootParam::DT_SRVS, m_mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
	// Assume there's a static sampler in the 
	else {
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_mainDescriptorHeap.Get() };
		cmdList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

		cmdList->SetGraphicsRootDescriptorTable(GlobalRootParam::DT_SRVS, m_mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		m_mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	}
}

DXGI_FORMAT DX12Texture2DArray::getFormat() {
	return DXGI_FORMAT();
}

UINT DX12Texture2DArray::getMips() {
	return 0;
}

ID3D12Resource * DX12Texture2DArray::getResource() {
	return nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Texture2DArray::getCpuDescHandle() {
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12Texture2DArray::getGpuDescHandle() {
	return D3D12_GPU_DESCRIPTOR_HANDLE();
}

void DX12Texture2DArray::setSampler(DX12Sampler2D * sampler) {
	m_sampler = sampler;
}
