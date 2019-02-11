#include "pch.h"
#include "DX12Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "DX12Sampler2D.h"

DX12Texture2D::DX12Texture2D(DX12Renderer* renderer) {
	m_renderer = renderer;
}


DX12Texture2D::~DX12Texture2D() {

}


// return 0 if image was loaded and texture created.
// else return -1
int DX12Texture2D::loadFromFile(std::string filename) {
	
	int w, h, bpp;
	m_rgba = stbi_load(filename.c_str(), &w, &h, &bpp, STBI_rgb_alpha); // TODO: remove m_rgba memory after copy to gpu is complete
	if (m_rgba == nullptr) {
		return -1;
	}

	D3D12_RESOURCE_DESC textureDesc = {};

	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//textureDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Width = w;
	textureDesc.Height = h;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.MipLevels = 1;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;

	// create a default heap where the upload heap will copy its contents into (contents being the texture)
	HRESULT hr = m_renderer->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&textureDesc, // the description of our texture
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
	//textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
	m_renderer->getDevice()->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

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
	textureData.RowPitch = w * bpp; /// size of all our triangle vertex data
	textureData.SlicePitch = w * bpp * h; /// also the size of our triangle vertex data

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
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_renderer->getDevice()->CreateShaderResourceView(m_textureBuffer.Get(), &srvDesc, m_mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	return 0;
}

void DX12Texture2D::bind(unsigned int slot) {

	throw std::exception("The texture must be bound using the other bind method taking two parameters");

}

void DX12Texture2D::bind(unsigned int slot, ID3D12GraphicsCommandList3* cmdList) {
	if (m_rgba) {
		delete m_rgba;
		m_rgba = nullptr;
	}

	// Set the descriptor heaps
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_mainDescriptorHeap.Get(), m_renderer->getSamplerDescriptorHeap() };
	cmdList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootDescriptorTable(DX12Renderer::RootParameterIndex::DT_SAMPLERS, reinterpret_cast<DX12Sampler2D*>(sampler)->getGPUHandle());
	cmdList->SetGraphicsRootDescriptorTable(DX12Renderer::RootParameterIndex::DT_SRVS, m_mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}
