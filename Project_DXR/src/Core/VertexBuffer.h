#pragma once
class VertexBuffer
{
public:
	enum DATA_USAGE { STATIC=0, DYNAMIC=1, DONTCARE=2 };

	VertexBuffer() {};
	virtual ~VertexBuffer() {}
	virtual void setData(const void* data, size_t size, size_t offset) = 0;
	virtual void bind(size_t offset, size_t size, unsigned int location) = 0;
	virtual void unbind() = 0;
	virtual size_t getSize() = 0;
	void incRef() { refs++; };
	void decRef() { if (refs>0) refs--; };
	inline unsigned int refCount() { return refs; };
protected:
private:
	//cheap ref counting
	unsigned int refs = 0;
};

