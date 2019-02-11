#pragma once
class RenderState
{
public:
	RenderState();
	~RenderState();

	virtual void setWireFrame(bool) = 0;

	// activate all options in this render state.
	virtual void set() = 0;
};

