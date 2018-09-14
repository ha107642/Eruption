#pragma once

#include <cstdint>
#include <vector>

class Memory_Block {
private:
	struct Memory_Chunk {
		bool allocated : 1;
		uint32_t size : 31;
		uint8_t data[];
	};

	uint8_t* memory;
	uint32_t memory_size;
	//std::vector<Memory_Chunk*> allocations;
public:
	void* allocate(const int size);	
	template<typename T> T* allocate() { return reinterpret_cast<T*>(allocate(sizeof(T))); }
	void free(void* address);
	template<typename T> void free(T* address) { free(reionterpret_cast<void*>(address)); }
};