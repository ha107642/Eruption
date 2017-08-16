#include "Network.h"

#include "io.h"
#include "Memory_Stream.h"

Network::Network() : connection(nullptr), is_initialized(false) {
}

Network::~Network() {
	if (connection != nullptr) enet_host_destroy(connection);
}

void Network::initialize() {
	enet_initialize();
}

void Network::update() {
	assert(is_initialized);

	ENetEvent event;

	while (enet_host_service(connection, &event, 0) > 0) {
		switch (event.type) {
			case ENET_EVENT_TYPE_CONNECT:
				accept(event);
				break;
			case ENET_EVENT_TYPE_RECEIVE: {
				//printf("A packet of length %u containing %s was received from %s on channel %u.\n",
				//	event.packet->dataLength, event.packet->data, event.peer->data, event.channelID);
				receive_packet(event);
				enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT: {
				Client_Info *info = (Client_Info*)event.peer->data;
				if (info == nullptr)
					break;
				printf("%d disconnected.\n", info->id);
				delete info;
				event.peer->data = NULL;
				break;
			}
		}
	}
}

void Network::flush() {
	enet_host_flush(connection);
}

ENetPacket* Network::make_packet_without_malloc(const void* data, size_t dataLength) {
	ENetPacket* packet = enet_packet_create(data, dataLength, ENET_PACKET_FLAG_NO_ALLOCATE);
	packet->flags = 0;
	return packet;
}
