#pragma once
#include <vector>
#include "Material.h"
#include "RenderState.h"

class Renderer;

class Technique
{
public:
	Technique(Material* m, RenderState* r) : material(m), renderState(r) {};
	virtual ~Technique();
	Material* getMaterial() { return material; };
	RenderState* getRenderState() { return renderState; };
	virtual void enable(Renderer* renderer);
protected:
	Material* material = nullptr;
	RenderState* renderState = nullptr;
};

