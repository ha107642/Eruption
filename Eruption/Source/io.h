#pragma once

#include <vector>
#include <iostream>
#include "Memory_Stream.h"

bool read_file_binary(const char* file_name, std::vector<char>* buffer);
bool file_exists(const char* file_name);

template<typename T> void write(Memory_Stream &stream, const T& data) {
	stream.write((void*)&data, sizeof(T));
}

template<typename T> void write(std::ostream& stream, const T& data) {
	stream.write((char*)&data, sizeof(T));
}

template<typename T> void write_vector(Memory_Stream &stream, const std::vector<T>& vector) {
	uint32_t size = vector.size();
	write<uint32_t>(stream, size);
	stream.write((char*)vector.data(), sizeof(T) * size);
}

template<typename T> void write_vector(std::ostream& stream, const std::vector<T>& vector) {
	uint32_t size = vector.size();
	write<uint32_t>(stream, size);
	stream.write((char*)vector.data(), sizeof(T) * size);
}

template<typename T> void read(Memory_Stream &stream, const T* data) {
	stream.read((void*)data, sizeof(T));
}

template<typename T> void read(std::istream& stream, const T* data) {
	stream.read((char*)data, sizeof(T));
}

template<typename T> void read_vector(Memory_Stream &stream, std::vector<T>& vector) {
	uint32_t size;
	read<uint32_t>(stream, &size);
	vector.resize(size);
	stream.read((char*)vector.data(), sizeof(T) * size);
}

template<typename T> void read_vector(std::istream& stream, std::vector<T>& vector) {
	uint32_t size;
	read<uint32_t>(stream, &size);
	vector.resize(size);
	stream.read((char*)vector.data(), sizeof(T) * size);
}