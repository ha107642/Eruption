#include "Server.h"

#include "Engine.h"
#include "Memory_Stream.h"
#include "io.h"

void Server::accept(ENetEvent ev) {
	printf("A new client connected from %x:%u.\n",
		ev.peer->address.host,
		ev.peer->address.port);
	/* Store any relevant client information here. */
	Client_Info *info = new Client_Info();
	info->base_state = -1;
	info->id = ev.peer->connectID;
	ev.peer->data = info;
	clients.push_back(ev.peer);
	
	//Disable throttling while testing...
	//enet_peer_throttle_configure(ev.peer, 0, 0, 0);
}

void Server::receive_packet(ENetEvent ev) {
	Client_Info *info = (Client_Info*)ev.peer->data;

	Memory_Stream stream(ev.packet->data, ev.packet->dataLength);
	Packet_Type packet_type;
	read(stream, &packet_type);

	switch (packet_type) {
		case PACKET_UPDATE_BASELINE:
			int32_t base_state;
			read(stream, &base_state);
			info->base_state = glm::max(info->base_state, base_state);
			break;
		default:
			break;
	}
}

Server::Server() : states(100) { }

void Server::initialize() {
	Network::initialize();

	ENetAddress address;
	//enet_address_set_host(&address, "localhost");
	address.host = ENET_HOST_ANY;
	address.port = 1234;

	const int CLIENT_COUNT = 16;

	connection = enet_host_create(&address, CLIENT_COUNT, NETWORK_CHANNEL_COUNT, 0, 0);

	if (!connection) {
		Engine::fail("Unable to create network server");
	}
	
	is_initialized = true;
}

ENetPacket* Server::make_state_packet(State &state) {
	//int size = sizeof(Packet_Type);
	//size += sizeof(state.id) + sizeof(state.last_entity) + sizeof(state.states.size());
	//for (System_State s : state.states)
	//	size += sizeof(s.system_id) + sizeof(s.count) + sizeof(s.size) + s.size;

	////This will make a double memcpy...
	//ENetPacket *packet = enet_packet_create(nullptr, size, 0);

	//Memory_Stream stream(packet->data, size);
	Memory_Stream stream;
	write(stream, PACKET_STATE);

	write(stream, engine->time.frame_count);
	write(stream, state.last_entity);
	write(stream, (uint32_t)state.states.size());
	for (System_State s : state.states) {
		write(stream, s.system_id);
		write(stream, s.count);
		write(stream, s.size);
		stream.write(s.data.get(), s.size);
	}

	write(stream, (uint32_t)state.entity_components.size());
	for (auto it = state.entity_components.begin(); it != state.entity_components.end(); ++it) {
		write(stream, it->first); //Write the entity.
		uint32_t system_count = it->second.size();
		write(stream, system_count);
		for (uint32_t i = 0; i < system_count; ++i) {
			write(stream, it->second.at(i)->get_system_id());
		}
	}

	//return packet;
	return make_packet_without_malloc(stream.buffer, stream.write_offset);
}

ENetPacket * Server::make_delta_state_packet(State &base_state) {
	Memory_Stream delta;
	write(delta, PACKET_STATE_DELTA);
	write(delta, base_state.id);
	uint8_t delta_offset = engine->time.frame_count - base_state.id;
	write(delta, delta_offset);
	engine->serialize_delta_state(delta, base_state);
	return make_packet_without_malloc(delta.buffer, delta.write_offset);
}

void Server::broadcast_state(State &state) {
	//ENetPacket* full_packet = make_state_packet(state);
	//ENetPacket* packet = make_delta_state_packet(state);
	//packet->flags = ENET_PACKET_FLAG_RELIABLE;

	states.push_front(state);
	
	for (size_t i = 0; i < clients.size(); ++i) {
		Client_Info *info = (Client_Info*)clients[i]->data;
		int state_diff = state.id - info->base_state;
		clients[i]->packetThrottleCounter = 0;

		if (info->base_state > 0 && state_diff < states.size()) {
			ENetPacket* packet = make_delta_state_packet(states[state_diff]);
			enet_peer_send(clients[i], NETWORK_CHANNEL_UNRELIABLE, packet);
			//printf("Server: Sending state! client = %d, packetsSent = %d, packetsLost = %d, packetLoss = %d, packetThrottleCounter = %d, nextTimeout = %d, state = %d\n", 
			//	clients[i], clients[i]->packetsSent, clients[i]->packetsLost, clients[i]->packetLoss, clients[i]->packetThrottleCounter, clients[i]->nextTimeout, clients[i]->state);
			//HACK! What does this even do?
		} else {
			ENetPacket* full_packet = make_state_packet(state);
			enet_peer_send(clients[i], NETWORK_CHANNEL_UNRELIABLE, full_packet);
		}
		//printf("full = %d bytes. delta = %d bytes.\n", full_packet->dataLength, packet->dataLength);
	}
	//enet_host_broadcast(connection, NETWORK_CHANNEL_UNRELIABLE, packet);
}
