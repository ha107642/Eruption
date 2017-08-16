#pragma once

#include "Entity.h"
#include "Time.h"
#include "State.h"
#include "Memory_Stream.h"
#include "io.h"

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <cassert>
#include <memory>

class ISystem {
public:
	virtual void update(Time& time) = 0;
	virtual void delete_component(Entity entity) = 0;
	virtual void clone_component(Entity source, Entity destination) = 0;
	virtual System_State get_state() = 0;
	virtual void serialize_delta_state(Memory_Stream &stream, System_State &base) = 0;
	virtual void deserialize_delta_state(Memory_Stream &stream) = 0;
	virtual void set_state(System_State state) = 0;
	virtual uint8_t get_system_id() = 0;
};

template<typename T>
struct Component_Holder {
	Entity entity;
	T component;
};

template<typename T> class Component_Reference;

template<typename T>
class System : public ISystem {
private:
	friend class Component_Reference<T>;
protected:
	std::vector<Component_Holder<T>> components;
	std::unordered_map<Entity, int> map = {};

	virtual bool are_components_equal(T* a, T* b) { return memcmp(a, b, sizeof(*a)) == 0; } //TODO: FIX! This may compare garbage alignment bytes, and thus not work correctly.
	virtual void reindex_map() {
		map.clear();
		int count = components.size();
		for (int i = 0; i < count; ++i)
			map.insert({ components[i].entity, i });
	}
	
	virtual void update(T* const component, Entity entity, Time& time) {};
public:
	static uint8_t system_id;

	uint8_t get_system_id() override { return system_id; }
	T* get_component(Entity entity) { return &components[map.at(entity)].component; }
	Component_Reference<T> get_component_reference(Entity entity) {
		if (has_component(entity))
			return Component_Reference<T>(get_component(entity), components.data(), this, entity, &map.at(entity));
		else
			return Component_Reference<T>();
	}
	bool has_component(Entity entity) { return map.find(entity) != map.end(); }
	virtual T* add_component(Entity entity) {
		int new_index = components.size();
		Component_Holder<T> holder = {};
		holder.entity = entity;
		holder.component = {};
		components.push_back(holder);
		map.insert({ entity, new_index });
		add_entity_component(entity, this);
		return &components[new_index].component;
	}

	void clone_component(Entity source, Entity destination) {
		T* new_component = add_component(destination);
		*new_component = *get_component(source);
	}
	
	virtual void serialize(Memory_Stream &stream, T& component, Entity entity) {
		write(stream, component);
	}

	virtual void deserialize(Memory_Stream &stream, T* component, Entity entity) {
		read(stream, component);
	}

	virtual System_State get_state() override {
		System_State state = {};
		state.system_id = system_id;
		state.count = components.size();

		Memory_Stream stream;
		for (uint32_t i = 0; i < state.count; ++i) {
			write(stream, components[i].entity);
			serialize(stream, components[i].component, components[i].entity);
		}

		state.size = stream.write_offset;
		state.data = std::shared_ptr<void>(stream.buffer, free);

		//state.size = state.count * sizeof(Component_Holder<T>);
		//state.data = std::shared_ptr<void>(malloc(state.size), free);
		//memcpy(state.data.get(), components.data(), state.size);
		return state;
	}

	virtual void set_state(System_State state) override {
		components.resize(state.count);
		
		Memory_Stream stream(state.data.get(), state.size);
		for (uint32_t i = 0; i < state.count; ++i) {
			read(stream, &components[i].entity);
			deserialize(stream, &components[i].component, components[i].entity);
		}
		//memcpy(components.data(), state.data.get(), state.size);
		reindex_map();
	}

	virtual void set_state(System_State_Delta<T> &delta) {
		int count = delta.deleted.size();
		for (int i = count - 1; i >= 0; --i) {
			delete_component(delta.deleted[i]);
		}

		count = delta.changed.size();
		for (int i = 0; i < count; ++i) {
			T* t = get_component(delta.changed[i].entity);
			*t = delta.changed[i].component;
		}

		count = delta.added.size();
		for (int i = 0; i < count; ++i) {
			T* t = add_component(delta.added[i].entity);
			*t = delta.added[i].component;
		}
	}

	virtual void delete_component(Entity entity) {
		int index = map[entity];
		Component_Holder<T> &index_ref = components[index];
		Component_Holder<T> &last_ref = components[components.size() - 1];
		map[last_ref.entity] = index; //We're swapping the indices, so we need to swap in the map as well.
		std::swap(index_ref, last_ref);
		components.pop_back();
		map.erase(entity);
		remove_entity_component(entity, this);
	}

	virtual void update(Time& time) override {
		int count = components.size();
		for (int i = 0; i < count; ++i)
			update(&components[i].component, components[i].entity, time);
	}

	virtual void deserialize_delta_state(Memory_Stream &stream) override {
		System_State_Delta<T> delta;
		
		read_vector(stream, delta.deleted);

		size_t size;
		read(stream, &size);
		delta.changed.resize(size);
		for (size_t i = 0; i < size; ++i) {
			read(stream, &delta.changed[i].entity);
			deserialize(stream, &delta.changed[i].component, delta.changed[i].entity);
		}

		read(stream, &size);
		delta.added.resize(size);
		for (size_t i = 0; i < size; ++i) {
			read(stream, &delta.added[i].entity);
			deserialize(stream, &delta.added[i].component, delta.added[i].entity);
		}

		set_state(delta);
	}

	virtual void serialize_delta_state(Memory_Stream &stream, System_State &base) override {
		System_State_Delta<T> delta = get_delta_state(base);

		//const int STATIC_SIZE = sizeof(size_t) * 3;
		//int size = STATIC_SIZE + sizeof(Entity) * delta.deleted.size() + sizeof(Component_Holder<T>) * (delta.changed.size() + delta.added.size());

		//Memory_Stream s(size);
		
		write_vector(stream, delta.deleted);
		
		write(stream, delta.changed.size());
		for (size_t i = 0; i < delta.changed.size(); ++i) {
			write(stream, delta.changed[i].entity);
			serialize(stream, delta.changed[i].component, delta.changed[i].entity);
		}
		
		write(stream, delta.added.size());
		for (size_t i = 0; i < delta.added.size(); ++i) {
			write(stream, delta.added[i].entity);
			serialize(stream, delta.added[i].component, delta.added[i].entity);
		}
	}

	virtual System_State_Delta<T> get_delta_state(System_State &base) {
		assert(base.system_id == system_id);
		std::vector<Component_Holder<T>> old_components;
		old_components.resize(base.count);
		Memory_Stream stream(base.data.get(), base.size);
		for (uint32_t i = 0; i < base.count; ++i) {
			read(stream, &old_components[i].entity);
			deserialize(stream, &old_components[i].component, old_components[i].entity);
		}

		System_State_Delta<T> delta = {};
		std::vector<bool> new_components(components.size());

		//crude.

		//First pass.. Try to find every direct match.
		int min_size = components.size() < old_components.size() ? components.size() : old_components.size(); //min.
		for (int i = min_size - 1; i >= 0; --i) {
			if (components[i].entity == old_components[i].entity) { //Same entity 
				if (!are_components_equal(&components[i].component, &old_components[i].component))
					delta.changed.push_back(components[i]); //ADD AS CHANGED
				
				new_components[i] = true;
				int last_index = old_components.size() - 1;
				if (i != last_index) std::swap(old_components[i], old_components[last_index]); //Put the element last.
				old_components.pop_back();
			}
		}
		
		//Second pass.. Find using map.
		for (int i = old_components.size() - 1; i >= 0; --i) {
			if (!has_component(old_components[i].entity)) continue;
			int index = map[old_components[i].entity];
			if (!are_components_equal(&components[index].component, &old_components[i].component))
				delta.changed.push_back(components[index]);//ADD AS CHANGED
			
			new_components[index] = true;
			int last_index = old_components.size() - 1;
			if (i != last_index) std::swap(old_components[i], old_components[last_index]); //Put the element last.
			old_components.pop_back();
		}

		size_t count = old_components.size();
		for (size_t i = 0; i < count; ++i) {
			delta.deleted.push_back(old_components[i].entity);
		}

		count = new_components.size();
		for (size_t i = 0; i < count; ++i) {
			if (new_components[i] == false)
				delta.added.push_back(components[i]);
		}

		return delta;
	}
};

template<typename T> uint8_t System<T>::system_id = -1;

template<typename T>
class Component_Reference {
private:
	Component_Holder<T>* data;
	System<T>* system;
	Entity entity;
	int index;
	int *index_pointer; //this will hopefully remain valid throughout the entity's life time.
	T* component;

	void update() {
		component = system->get_component(entity);
		data = system->components.data();
		index_pointer = &system->map[entity];
		index = *index_pointer;
		component = &system->components[index].component;
		std::cout << "update" << std::endl;
	}

	inline bool is_valid() {
		assert(system != nullptr);
		return system->components.data() == data && index == *index_pointer;
	}
public:

	Component_Reference() : system(nullptr) {}
	Component_Reference(T* component, Component_Holder<T>* data, System<T>* system, Entity entity, int* index) : 
		component(component), data(data), system(system), entity(entity), index(*index), index_pointer(index) { }

	T* get() {
		if (!is_valid())
			update();
		return component;
	}
	inline T* operator->() { return get(); }
	inline bool is_initialized() { return system != nullptr; }
};

extern void add_entity_component(Entity entity, ISystem *system);
extern void remove_entity_component(Entity entity, ISystem *system);