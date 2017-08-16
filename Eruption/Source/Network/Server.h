#pragma once

#include "Network.h"
#include "Circular_Array.h"

class Server : public Network {
private:
	std::vector<ENetPeer*> clients;
	Circular_Array<State> states;

	ENetPacket* make_state_packet(State &state);
	ENetPacket* make_delta_state_packet(State &base_state);
	void accept(ENetEvent ev);
	void receive_packet(ENetEvent ev);
public:
	Server();

	void initialize();
	void broadcast_state(State &state);
};