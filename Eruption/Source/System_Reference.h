#pragma once

#include <vector>
#include <assert.h>
#include "Entity.h"

template<typename T> class System;
template<typename T> inline System<T>* get_system();


template<typename C>
class Reference_Subscriber {
protected:
	std::vector<C*> components;
	System<C>& sibling_system;

	Reference_Subscriber() : sibling_system(*get_system<C>()) {
		sibling_system.subscribe(this);
	}

public:
	virtual void component_added(const Entity entity, C* component) = 0;
	virtual void component_moved(const Entity entity, C* component) = 0;
	virtual void component_deleted(const Entity entity) = 0;
	virtual void components_reallocated(const int64_t offset) = 0;
};

class ISystem_Reference {
public:
	virtual void base_component_added(const Entity entity) = 0;
	virtual void base_component_deleted(const int index) = 0;
	virtual void state_updated(const std::vector<Entity>& entities) = 0;
};

template<typename T, typename C>
class System_Reference : Reference_Subscriber<C>, ISystem_Reference {
private:
	System<T>& system;
protected:
	void base_component_added(const Entity entity) override {
		C* component = sibling_system.has_component(entity) ? sibling_system.get_component(entity) : nullptr;
		components.push_back(component);
	}

	void base_component_deleted(const int index) override {
		std::swap(components[index], components[components.size() - 1]);
		components.pop_back();
	}

	virtual void state_updated(const std::vector<Entity>& entities) override {
		components.clear();
		for (size_t i = 0; i < entities.size(); ++i)
			base_component_added(entities[i]);
	}
	
	void component_added(const Entity entity, C* component) override {
		int index = system.get_index(entity);
		if (index == -1)
			return;
		components[index] = component;
	}

	virtual void component_moved(const Entity entity, C* component) override {
		int index = system.get_index(entity);
		if (index == -1)
			return;

		components[index] = component;
	}

	virtual void component_deleted(const Entity entity) override {
		int index = system.get_index(entity);
		if (index == -1)
			return;

		components[index] = nullptr;
	}

	virtual void components_reallocated(const int64_t offset) override {
		size_t count = components.size();
		for (size_t i = 0; i < count; ++i) {
			char* address = reinterpret_cast<char*>(components[i]);
			address += offset;
			components[i] = reinterpret_cast<C*>(address);
		}
	}
public:
	System_Reference(System<T>& base_system) : system(base_system) {
		system.add_system_reference(this);
	}

	C* operator[](const int i) {
		assert(i >= 0 && i < components.size());
		return components[i];
	}

	C* operator[](const size_t i) {
		assert(i < components.size());
		return components[i];
	}

};