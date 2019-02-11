#include "pch.h"
#include "../DX12/DX12Renderer.h"
#include "Renderer.h"


Renderer* Renderer::makeRenderer(BACKEND option)
{
	if (option == BACKEND::DX12)
		return new DX12Renderer();

	throw("The option sent to makeRenderer was invalid.");
}

