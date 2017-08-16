#include "Memory_Stream.h"

#include <assert.h>
#include <cstdlib>
#include <cstring>

Memory_Stream::Memory_Stream() : Memory_Stream(512) {
	is_dynamic = true;
}

Memory_Stream::Memory_Stream(int size) : size(size), write_offset(0), read_offset(0), is_dynamic(false) {
	buffer = malloc(size);
}
Memory_Stream::Memory_Stream(void* buffer, int size) : buffer(buffer), size(size), write_offset(0), read_offset(0), is_dynamic(false) {}
//~Memory_Stream() { if (data_owner && buffer) free(buffer); }

void Memory_Stream::write(void *data, int data_size) {
	if (is_dynamic) {
		if (write_offset + data_size > size) {
			size *= 2;
			buffer = realloc(buffer, size);
		}
	} else {
		assert(write_offset + data_size <= size);
	}
	memcpy((char*)buffer + write_offset, data, data_size);
	write_offset += data_size;
}

void Memory_Stream::read(void * data, int data_size) {
	assert(read_offset + data_size <= size);
	memcpy(data, (char*)buffer + read_offset, data_size);
	read_offset += data_size;
}
