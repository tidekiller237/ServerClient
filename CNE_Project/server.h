#pragma once

#include "includes.h"

static char* SERVER_NAME = (char*)"SERVER15348679213653154966213";

struct User
{
	char* username;
	SOCKET u_socket;

public:

	User(char* uname, SOCKET s) {
		username = new char[25];
		memset(username, 0, 25);
		strcpy(username, uname);
		u_socket = s;
	}
};

struct Connection {
	int result;
	SOCKET c_socket;
};

class Server
{
	SOCKET s_listensocket;
	SOCKET udp_socket;
	sockaddr_in udpaddr;

	char* broadcastmsg;

	char* SV_SUCCESS;
	char* SV_FULL;

	const int buffer_size = 1024;
	char* buffer = new char[buffer_size];
	char* tcp_brodcast = new char[buffer_size];
	char* messages = new char[buffer_size];

	std::vector<User*> r_clients;
	fd_set masterset, readyset;
	int nsockets;
	timeval timeout;
	int nclients;
	int max_clients;

	fstream fs;
	int log_size;

	//utility
	CMDUtils utils;

public:	

	int init(uint16_t port);
	int server_main();
	Connection socket_accept();
	int socket_recv(SOCKET r_socket, char* buffer, int32_t size);
	int socket_send(SOCKET s_socket, char* buffer, int32_t size);
	int parse(User* user, char* buffer);
	int parse(SOCKET sock, char* buffer);
	void log_str(char* log);
	void send_log(User* user);
	void remove_user(User* u);
	void tcp_broadcast(char* message, User* excl);
	void client_disconnect_handle(User* client);
	void client_shutdown_handle(User* client);
	void stop();
	void ClearBuffer(char* buffer, int32_t size);
	void output_override();

	//commands
	void reg(SOCKET sock, char* uname);
};