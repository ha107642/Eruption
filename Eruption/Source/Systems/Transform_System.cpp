#include "Transform_System.h"

#include "Engine.h"
#include "Interpolate.h"

#include <iostream>
#include <sstream>

void Transform_System::set_state(System_State_Delta<Transform> &delta) {
	int count = delta.deleted.size();
	for (int i = count - 1; i >= 0; --i) {
		delete_component(delta.deleted[i]);
	}

	if (engine->is_authoritative()) {
		count = delta.changed.size();
		for (int i = 0; i < count; ++i) {
			Transform* t = &(components.at(map.at(delta.changed[i].entity)).component);
			*t = delta.changed[i].component;
		}
	} else { 
		//As clients, we want to interpolate between components and target_components.
		//Thus, we only change the target for now (and let update interpolate).
		Interpolate* interp_system = (Interpolate*)engine->get_system<Interpolation>();
		interp_system->set_targets(delta);
		//interp_system->snap();
		//for (int i = 0; i < count; ++i) {
		//	Interpolation* interpolation = interp_system->add_component(delta.changed[i].entity);
		//	interpolation->base = *get_component(delta.changed[i].entity);
		//	interpolation->target = delta.changed[i].component;

			/*Transform* t = &(components_to_change->at(map.at(delta.changed[i].entity)).component);
			*t = delta.changed[i].component;*/
		//}
	}

	//for (int i = 0; i < count; ++i) {
	//	Interpolation* interpolation = interp_system->add_component(delta.changed[i].entity);
	//	interpolation->current = get_component_reference(delta.changed[i].entity);
	//	interpolation->base = *interpolation->current.get();
	//	interpolation->target = delta.changed[i].component;

	//	/*Transform* t = &(components_to_change->at(map.at(delta.changed[i].entity)).component);
	//	*t = delta.changed[i].component;*/
	//}

	count = delta.added.size();
	for (int i = 0; i < count; ++i) {
		Transform* t = add_component(delta.added[i].entity);
		*t = delta.added[i].component;
	}
}