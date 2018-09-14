#include "Memory_Block.h"

#include <assert.h>

void* Memory_Block::allocate(const int size) {
	int total_size = 0;
	Memory_Chunk* chunk;

	do {
		chunk = reinterpret_cast<Memory_Chunk*>(memory[total_size]);
		total_size += chunk->size + sizeof(Memory_Chunk);

		if (total_size > memory_size) {
			assert("Out of memory in the memory block.");
			return nullptr;
		}

	} while(chunk->allocated || chunk->size < size);

	chunk->allocated = true;
	chunk->size = size;
	
	int chunk_total_size = chunk->size + sizeof(Memory_Chunk);
	uint8_t* address = &reinterpret_cast<uint8_t*>(&chunk)[chunk_total_size];
	Memory_Chunk* new_chunk = reinterpret_cast<Memory_Chunk*>(address);
	new_chunk->allocated = false;
	new_chunk->size = total_size - chunk_total_size;

	return chunk->data;
}

void Memory_Block::free(void* address) {
	uint8_t* chunk_address = reinterpret_cast<uint8_t*>(&address) - sizeof(Memory_Chunk);
	Memory_Chunk* chunk = reinterpret_cast<Memory_Chunk*>(chunk_address);
	chunk->allocated = false;

	int total_chunk_size = chunk->size;
	while (chunk_address < memory + memory_size) {
		chunk_address += chunk->size + sizeof(Memory_Chunk);
		Memory_Chunk* chunk = reinterpret_cast<Memory_Chunk*>(chunk_address);
		if (chunk->allocated)
			break;
		else
			total_chunk_size += chunk->size + sizeof(Memory_Chunk);
	}

	chunk->size = total_chunk_size;
}
