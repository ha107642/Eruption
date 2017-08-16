#include "io.h"

#include <fstream>

bool read_file_binary(const char* file_name, std::vector<char>* buffer) {
	std::streampos size;

	std::ifstream file(file_name, std::ios::in | std::ios::binary | std::ios::ate);
	if (!file.is_open())
		return false;

	size = file.tellg();
	buffer->resize((size_t)size);
	file.seekg(0, std::ios::beg);
	file.read(buffer->data(), size);
	file.close();

	return true;
}

bool file_exists(const char* file_name) {
	std::ifstream f(file_name);
	return f.good();
}