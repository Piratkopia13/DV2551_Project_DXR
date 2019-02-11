#pragma once

// just some values to play with, not complete!
enum WRAPPING { REPEAT = 0, CLAMP = 1 };

// just some values to play with, not complete!
enum FILTER { POINT_SAMPLER = 0, LINEAR = 0 };

class Sampler2D
{
public:
	Sampler2D();
	virtual ~Sampler2D();
	virtual void setMagFilter(FILTER filter) = 0;
	virtual void setMinFilter(FILTER filter) = 0;
	virtual void setWrap(WRAPPING s, WRAPPING t) = 0;
};

