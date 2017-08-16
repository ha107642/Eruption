#include "Client.h"

#include "Engine.h"
#include "Memory_Stream.h"
#include "io.h"

void Client::accept(ENetEvent ev) {
	_is_connected = true;
}

void Client::receive_packet(ENetEvent ev) {
	Memory_Stream stream(ev.packet->data, ev.packet->dataLength);
	Packet_Type packet_type;
	read(stream, &packet_type);
	switch (packet_type) {
		case PACKET_STATE: {
			State state;
			
			read(stream, &state.id);
			read(stream, &state.last_entity);
			uint32_t state_count;
			read(stream, &state_count);
			state.states.resize(state_count);
			for (uint32_t i = 0; i < state_count; ++i) {
				read(stream, &state.states[i].system_id);
				read(stream, &state.states[i].count);
				read(stream, &state.states[i].size);
				state.states[i].data = std::shared_ptr<void>(malloc(state.states[i].size), free);
				stream.read(state.states[i].data.get(), state.states[i].size);
			}

			uint32_t ec_count;
			read(stream, &ec_count);
			state.entity_components.clear();
			Entity entity;
			uint32_t system_count;
			for (uint32_t i = 0; i < ec_count; ++i) {
				read(stream, &entity);
				read(stream, &system_count);
				state.entity_components[entity] = std::vector<ISystem*>(system_count);
				uint8_t system_id;
				for (uint32_t j = 0; j < system_count; ++j) {
					read(stream, &system_id);
					state.entity_components[entity].push_back(engine->get_system(system_id));
				}
			}

			engine->set_state(state);
			current_state_id = state.id;

			ENetPacket* packet = make_baseline_packet(state.id);
			enet_host_broadcast(connection, NETWORK_CHANNEL_UNRELIABLE, packet);
			
			break;
		}
		case PACKET_STATE_DELTA: {
			uint32_t state_id;
			uint8_t delta_offset;
			read(stream, &state_id);
			read(stream, &delta_offset);

			if (state_id == current_state_id) {
				engine->set_delta_state(stream);
				current_state_id = state_id + delta_offset;
				ENetPacket* packet = make_baseline_packet(current_state_id);
				enet_host_broadcast(connection, NETWORK_CHANNEL_UNRELIABLE, packet);
			}
			else {
				printf("Got a packet with state_id = %d, current_state_id = %d. Discarding.\r\n", state_id, current_state_id);
			}			

			break;
		}
		default:
			break;
		}
}

void Client::initialize() {
	Network::initialize();

	connection = enet_host_create(nullptr, 1, NETWORK_CHANNEL_COUNT, 0, 0);

	if (!connection) {
		Engine::fail("Unable to create network server");
	}

	is_initialized = true;
}

void Client::connect(const char* host, int port) {
	_is_connected = false; //?

	ENetAddress address;
	enet_address_set_host(&address, host);
	address.port = port;

	server = enet_host_connect(connection, &address, 2, 0);

	if (!server) {
		Engine::fail("Unable to initialize a connection to %s:%d", host, port);
	}
}

ENetPacket * Client::make_baseline_packet(uint32_t id) {
	const int size = sizeof(Packet_Type) + sizeof(id);
	ENetPacket *packet = enet_packet_create(nullptr, size, 0);

	Memory_Stream stream(packet->data, size);
	write(stream, PACKET_UPDATE_BASELINE);
	write(stream, id);

	return packet;
}