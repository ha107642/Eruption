#pragma once

#include "Entity.h"

#include <memory>
#include <vector>
#include <unordered_map>

template<typename T> struct Component_Holder;
class ISystem;

struct System_State {
	uint8_t system_id;
	uint32_t count;
	uint32_t size;
	std::shared_ptr<void> data; //reference counter.
};

template<typename T>
struct System_State_Delta {
	std::vector<Entity> deleted;
	std::vector<Component_Holder<T>> changed;
	std::vector<Component_Holder<T>> added;

	/*int get_memory_size() { 
		const int STATIC_SIZE = sizeof(size_t) * 3;
		return STATIC_SIZE + sizeof(Entity) * deleted.size() + sizeof(Component_Holder<T>) * (changed.size() + added.size());
	}*/
};

struct State {
	uint32_t id;
	Entity last_entity;
	std::vector<System_State> states;
	std::unordered_map<Entity, std::vector<ISystem*> > entity_components;
};
