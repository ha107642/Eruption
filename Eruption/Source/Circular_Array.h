#pragma once

#include <vector>
#include <cassert>

template<typename T>
class Circular_Array {
private:
	std::vector<T> container;
	int pointer;
	int _size;

	int get_internal_index(int index) { return ((pointer - index - 1) + 2 * capacity()) % capacity(); }
public:
	Circular_Array(int capacity) : container(capacity), pointer(0), _size(0) {}

	int size() { return _size; }
	int capacity() { return container.capacity(); }
	T operator[](int index) {
		assert(index >= 0);
		assert(index < _size);
		int i = get_internal_index(index);
		return container[i];
	}

	void push_front(T data) {
		container[pointer] = std::move(data);
		pointer = (pointer + 1) % capacity();
		if (_size < capacity())
			++_size;
	}

	void pop_back() {
		assert(_size > 0);
		--_size;
	}
};