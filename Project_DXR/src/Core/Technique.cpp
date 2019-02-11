#include "pch.h"
#include "Technique.h"
#include "Renderer.h"

Technique::~Technique()
{
	delete renderState;
	std::cout << "destroyed technique" << std::endl;
}

void Technique::enable(Renderer* renderer)
{
	// better to delegate the render state to the renderer, it can be
	// more clever about changes with current render state set.
	renderer->setRenderState(renderState);
	material->enable();
}

