#pragma once

class Memory_Stream {
public:
	void* buffer;
	int size;
	int write_offset;
	int read_offset;
	bool is_dynamic;
	
	Memory_Stream();
	Memory_Stream(int size);
	Memory_Stream(void* buffer, int size);

	void write(void *data, int data_size);
	void read(void* data, int data_size);
};