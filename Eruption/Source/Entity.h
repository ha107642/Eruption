#pragma once

#include <cinttypes>

typedef uint32_t Entity;

struct Entity_Hash {
	size_t operator()(const Entity entity) const { return entity; }
};

constexpr Entity ENTITY_NULL = ((Entity)0);