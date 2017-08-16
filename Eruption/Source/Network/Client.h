#pragma once

#include "Network.h"

class Client : public Network {
protected:
	ENetPeer *server;
	bool _is_connected;
	uint32_t current_state_id;

	ENetPacket* make_baseline_packet(uint32_t id);
	void accept(ENetEvent ev);
	void receive_packet(ENetEvent ev);
public:
	bool is_connected() { return _is_connected; };
	void initialize();
	void connect(const char* host, int port);
};