#pragma once
#include <vector>

template<typename T>
class CircularBuffer
{
public:
	CircularBuffer(int size) : container(size) {};
	~CircularBuffer() {};
	
	int size = 0;
	int index = 0;
	std::vector<T> container;

	T Get()
	{
		if (++index >= size)
			index = 0;
		return container[index];
	}
};

