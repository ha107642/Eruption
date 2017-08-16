#pragma once

#include <enet/enet.h>
#include "System.h"

enum Network_Channel {
	NETWORK_CHANNEL_RELIABLE,
	NETWORK_CHANNEL_UNRELIABLE,
	NETWORK_CHANNEL_COUNT
};

enum Packet_Type : uint8_t {
	PACKET_STATE_DELTA = 0,
	PACKET_STATE = 1,
	PACKET_UPDATE_BASELINE = 2, //ACK...
	PACKET_RPC = 3
};

struct Client_Info {
	int16_t id;
	//some entity?
	int32_t base_state;
};

class Network {
protected:
	ENetHost *connection;
	bool is_initialized;

	virtual void accept(ENetEvent ev) {};
	virtual void receive_packet(ENetEvent ev) = 0;
public:
	Network();
	~Network();

	virtual void initialize();
	void update();
	void flush();
	ENetPacket * make_packet_without_malloc(const void * data, size_t dataLength);
};