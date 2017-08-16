#pragma once

#include <stdlib.h>
#include <algorithm>

template<typename T>
class Aligned_Array {
private:
	uint8_t* _data;
	int alignment;
	int _size;
	int _capacity;

	void set_capacity(int new_capacity) {
		if (_capacity == new_capacity)
			return;
		_capacity = new_capacity;
		_data = (uint8_t*)realloc(_data, alignment * new_capacity);
	}
public:
	Aligned_Array(int alignment) : Aligned_Array(alignment, 2) {}

	Aligned_Array(int alignment, int capacity) : alignment(alignment), _size(0), _capacity(capacity) {
		_data = (uint8_t*)malloc(alignment * capacity);
	}
	~Aligned_Array() { free(_data); }

	int capacity() { return _capacity; }
	int size() { return _size; }
	int memory_size() { return _size * alignment; }
	int memory_capacity() { return _capacity * alignment; }
	T* data() { return (T*)_data; }
	T& operator[](int index) {
		assert(index >= 0 && index < _size);
		return (T&)_data[index * alignment]; 
	}

	void resize(int new_size) {
		set_capacity(new_size);
		_size = new_size;
	}

	void push_back(T& value) {
		if (_size == _capacity)
			set_capacity(_capacity * 2);
		
		memcpy(_data + (_size * alignment), &value, sizeof(value));
		++_size;
	}
	void pop_back() {
		_size--;
		//shrink?
	}
	
};