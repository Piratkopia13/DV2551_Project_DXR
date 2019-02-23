#pragma once

#include "Material.h"

class ConstantBuffer {
public:
	ConstantBuffer(std::string NAME, size_t size);
	virtual ~ConstantBuffer();
	// set data will update the buffer associated, including whatever is necessary to
	// update the GPU memory.
	virtual void setData(const void* data, unsigned int location) = 0;
	virtual void bind(Material*) = 0;

protected:
	size_t bufferSize;

};

