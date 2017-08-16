#include "System.h"

#include "Engine.h"

void add_entity_component(Entity entity, ISystem *system) {
	engine->add_entity_component(entity, system);
}

void remove_entity_component(Entity entity, ISystem *system) {
	engine->remove_entity_component(entity, system);
}