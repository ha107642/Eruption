//#include "System.h"
//
//#include "Engine.h"
//
//
//Entity new_entity() {
//	return ++last_entity;
//}
//
//Entity new_entity(Engine &engine, Entity copy_from) {
//	Entity entity = ++last_entity;
//	std::vector<ISystem*> *entity_systems = &(engine.entity_components[copy_from]);
//	int size = entity_systems->size();
//	for (int i = 0; i < size; ++i) {
//		(*entity_systems)[i]->clone_component(copy_from, entity);
//	}
//
//	return entity;
//}
//
//void destroy_entity(Engine &engine, Entity * entity) {
//	//Destroy all of its components.
//	std::vector<ISystem*> *entity_systems = &(entity_components[*entity]);
//	int size = entity_systems->size();
//	for (int i = size - 1; i >= 0; --i) {
//		(*entity_systems)[i]->delete_component(*entity);
//		entity_systems->pop_back();
//	}
//	//Destroy its entry in the component list, and null the entity.
//	entity_components.erase(*entity);
//	*entity = ENTITY_NULL;
//}