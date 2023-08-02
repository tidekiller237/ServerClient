#pragma once

#include "includes.h"



class Client
{
	char* SV_SUCCESS;
	char* SV_FULL;

	SOCKET tcp_socket;
	SOCKET udp_socket;
	sockaddr_in server_bcaddr;

	const int buffer_size = 1024;
	char* buff = new char[buffer_size];
	bool readytosend;
	fd_set masterset, readyset;
	timeval timeout;
	atomic<bool> s_done;
	atomic<bool> r_done;
	atomic<bool> shutdowncmd;
	atomic<bool> recv_log;

	//utility
	CMDUtils utils;

public:
	char* username = nullptr;
	bool registered = false;

	int init();
	int tcp_init(uint16_t port, char* address);
	int client_main();
	void recv_logf(int32_t size);
	void stop();
	void output_override();
};