#include "server.h"

int Server::init(uint16_t port)
{
	char* tempchar = new char[5];
	_itoa(port, tempchar, 10);

	//empty log file
	fs.open("server_log.txt", std::fstream::out | std::fstream::trunc);
	fs << "";
	fs.close();
	log_size = 0;

	//set memory
	memset(messages, 0, buffer_size);
	memset(buffer, 0, buffer_size);

	//zero master set
	FD_ZERO(&masterset);
	timeout.tv_sec = 1;

	//create listen socket
	s_listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s_listensocket == INVALID_SOCKET)
		return SOCKET_FAILURE;

	//bind
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	//save the address and port
	broadcastmsg = inet_ntoa(server_addr.sin_addr);
	strcat(broadcastmsg, (char*)"\n");
	strcat(broadcastmsg, tempchar);
	//delete[] tempchar;

	int result = bind(s_listensocket, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (result == SOCKET_ERROR)
		return BIND_FAILURE;

	FD_SET(s_listensocket, &masterset);
	r_clients.push_back(new User(SERVER_NAME, s_listensocket));

	//listen
	result = listen(s_listensocket, 1);
	if (result == SOCKET_ERROR)
		return SETUP_FAILURE;

	//create the udp broadcast socket
	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	//set broadcast option
	char opt = 1;
	result = setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
	if (result == SOCKET_ERROR)
		return OPT_FAILURE;
	
	//create broadcast addr
	udpaddr.sin_family = AF_INET;
	udpaddr.sin_addr.S_un.S_addr = htonl(INADDR_BROADCAST);
	udpaddr.sin_port = htons(port);

	FD_SET(udp_socket, &masterset);
	r_clients.push_back(new User(SERVER_NAME, udp_socket));
	nsockets = 2;
	nclients = 0;

	SV_SUCCESS = new char[buffer_size];
	SV_FULL = new char[buffer_size];
	memset(SV_SUCCESS, 0, buffer_size);
	memset(SV_FULL, 0, buffer_size);
	SV_SUCCESS = (char*)"SV_SUCCESS";
	SV_FULL = (char*)"SV_FULL";

	return SUCCESS;
}

Connection Server::socket_accept()
{
	Connection res;

	SOCKET connected_socket = accept(s_listensocket, NULL, NULL);
	if (connected_socket == INVALID_SOCKET)
		res.result = CONNECTION_FAILURE;
	
	//add socket to master set
	FD_SET(connected_socket, &masterset);
	nsockets++;

	//recive username
	socket_recv(connected_socket, buffer, buffer_size);

	res.result = SUCCESS;
	res.c_socket = connected_socket;

	return res;
}

int Server::socket_recv(SOCKET r_socket, char* buffer, int32_t size)
{
	int result = recv(r_socket, buffer, size, 0);
	if (result == SOCKET_ERROR)
		return DISCONNECTED;
	else if (result == 0)
		return SHUTDOWN;

	if (result > buffer_size)
		return PARAMETER_ERROR;

	return SUCCESS;
}

int Server::socket_send(SOCKET s_socket, char* buffer, int32_t size)
{
	if (size < 0 || size > MAXBUFFERSIZE)
		return PARAMETER_ERROR;

	int result = send(s_socket, buffer, size, 0);
	if (result == SOCKET_ERROR)
		return DISCONNECTED;
	else if (result == 0)
		return SHUTDOWN;

	return SUCCESS;
}

void Server::tcp_broadcast(char* message, User* excl) 
{
	for (int i = 2; i < nsockets; i++) {
		if (excl != nullptr) {
			if (r_clients[i]->u_socket != excl->u_socket)
				socket_send(r_clients[i]->u_socket, message, strlen(message));
		}
		else {
			socket_send(r_clients[i]->u_socket, message, strlen(message));
		}
	}
}

void Server::remove_user(User* u)
{
	int index = -1;

	for (int i = 0; i < nsockets; i++) {
		if (r_clients[i] == u) {
			index = i;
		}
	}

	if (index >= 0) {
		r_clients.erase(r_clients.begin() + index);
	}

	nsockets--;
	nclients--;

	FD_CLR(u->u_socket, &masterset);

	shutdown(u->u_socket, SD_BOTH);
	closesocket(u->u_socket);

	delete u;
}

void Server::stop()
{
	//close client sockets
	for (int i = 0; i < nsockets; i++) {
		shutdown(r_clients[i]->u_socket, SD_BOTH);
		closesocket(r_clients[i]->u_socket);
	}
}

void Server::ClearBuffer(char* buffer, int32_t size)
{
	memset(buffer, 0, buffer_size);
}

void Server::output_override() {
	printf("\r> ");
	fflush(stdout);
}

int Server::server_main()
{
	//set server size
	max_clients = 0;
	while (max_clients <= 0) {
		max_clients = 0;
		max_clients = utils.promptuser_uint((char*)"Number of clients allowed: ");
	}

	utils.pad();
	printf("Server online, awaiting connections");
	utils.pad();

	while (true)
	{
		//reset ready set
		readyset = masterset;

		//select
		int ready = select(0, &readyset, NULL, NULL, &timeout);

		//broadcast
		sendto(udp_socket, (char*)"1", 1, 0, (sockaddr*)&udpaddr, sizeof(udpaddr));
		/*output_override();
		printf("Broadcasting\n");*/

		//parse
		if (FD_ISSET(s_listensocket, &readyset)) {
			Connection result = socket_accept();

			output_override();
			switch (result.result)
			{
			case SUCCESS:
				printf("%s\n", buffer);
				parse(result.c_socket, buffer);
				break;
			case CONNECTION_FAILURE:
				printf("Client connection failed\n");
				break;
			case SERVER_FULL:
				printf("Server capcity reached, Can't accept new client\n.");
				break;
			default:
				break;
			}

			FD_CLR(s_listensocket, &readyset);
		}
		
		for (int i = 0; i < nsockets; i++) {
			//read
			if (FD_ISSET(r_clients[i]->u_socket, &readyset)) {
				
				//clear buffer
				ClearBuffer(buffer, buffer_size);

				//get data
				int result = socket_recv(r_clients[i]->u_socket, buffer, buffer_size);

				//skip processing if message is empty
				if (strlen(buffer) == 1)
					continue;

				//log data
				char* str = new char[buffer_size];
				memset(str, 0, buffer_size);

				for (int read = 0; read < strlen(buffer); read++) {
					if (buffer[read] != '\n') {
						str[read] = buffer[read];
					}
				}

				strcat(str, " - ");
				strcat(str, r_clients[i]->username);
				strcat(str, "\n");
				log_str(str);

				//check result
				output_override();
				switch (result) {
				case SUCCESS:
					printf("%s", buffer);
					parse(r_clients[i], buffer);
					break;
				case DISCONNECTED:
					printf("Client connection error\n");
					client_disconnect_handle(r_clients[i]);
					break;
				case SHUTDOWN:
					printf("Client disconnected\n");
					client_shutdown_handle(r_clients[i]);
					break;
				case PARAMETER_ERROR:
					printf("Message error\n");
				default:
					break;
				}
			}
		}
	}

	stop();
	return 0;
}

int Server::parse(User* user, char* buffer)
{
	//ehco
	if (buffer[0] != '$') {
		char* result = new char[buffer_size];
		strcpy(result, user->username);
		strcat(result, (char*)": ");
		strcat(result, buffer);
		tcp_broadcast(result, user);
	}
	else {
		char* cmd = new char[buffer_size];
		memset(cmd, 0, buffer_size);
		int delim = 0;

		//find the first space
		for (int i = 0; i < buffer_size; i++) {
			if (buffer[i] == ' ' || buffer[i] == '\n') {
				delim = i;
			}
		}

		for (int i = 0; i < delim; i++) {
			cmd[i] = buffer[0];

			for (int j = 0; j < buffer_size - 1; j++) {
				buffer[j] = buffer[j + 1];
			}
		}

		if (strcmp(cmd, (char*)"$getlist") == 0) {
			//return the list of connected clients
			char* str = new char[buffer_size];
			memset(str, 0, buffer_size);

			strncpy(str, (char*)"Client List :\n", strlen("Client List :\n\t"));

			for (int i = 2; i < nsockets; i++) {
				strcat(str, (char*)"\t");
				strcat(str, r_clients[i]->username);
				strcat(str, (char*)"\n");
			}

			socket_send(user->u_socket, str, strlen(str));
		}
		else if (strcmp(cmd, (char*)"$getlog") == 0) {
			//return the log file
			send_log(user);
		}
		else if (strcmp(cmd, (char*)"$exit") == 0) {
			//send shutdown command
			char* str = new char[buffer_size];
			memset(str, 0, buffer_size);

			strncpy(str, (char*)"client_server_shutdown_command", strlen("client_server_shutdown_command"));

			socket_send(user->u_socket, str, strlen(str));

			client_shutdown_handle(user);
		}
		else {
			strcat(cmd, (char*)" is an invalid command.\n");
			socket_send(user->u_socket, cmd, strlen(cmd));
		}
	}

	return SUCCESS;
}

int Server::parse(SOCKET sock, char* buffer)
{
	char* cmd = new char[buffer_size];
	memset(cmd, 0, buffer_size);
	int delim = 0;

	//find the first space
	for (int i = 0; i < buffer_size; i++) {
		if (buffer[i] == ' ' || buffer[i] == '\n') {
			delim = i;
		}
	}

	for (int i = 0; i < delim; i++) {
		cmd[i] = buffer[0];

		for (int j = 0; j < buffer_size - 1; j++) {
			buffer[j] = buffer[j + 1];
		}
	}

	//register
	User* temp = new User(buffer, sock);
	r_clients.push_back(temp);

	if (strcmp(cmd, (char*)"$register") == 0 && nclients < max_clients) {
		nclients++;
		ClearBuffer(buffer, buffer_size);
		strcpy(buffer, SV_SUCCESS);
		socket_send(sock, buffer, strlen(buffer));

		char* str = new char[buffer_size];
		memset(str, 0, buffer_size);
		strcpy(str, temp->username);
		strcat(str, (char*)" has connected\n");
		tcp_broadcast(str, temp);
	}
	else if (nclients >= max_clients) {
		nclients++;
		output_override();
		printf("%s failed to register. Server full.\n", temp->username);
		ClearBuffer(buffer, buffer_size);
		strcpy(buffer, SV_FULL);
		socket_send(sock, buffer, strlen(buffer));
		remove_user(temp);
	}

	return SUCCESS;
}

void Server::client_disconnect_handle(User* client) 
{
	char* str = new char[buffer_size];
	memset(str, 0, buffer_size);
	strcpy(str, client->username);
	strcat(str, (char*)"'s connection has timed out.\n");
	tcp_broadcast(str, client);
	remove_user(client);
}

void Server::client_shutdown_handle(User* client) 
{
	char* str = new char[buffer_size];
	memset(str, 0, buffer_size);
	strcpy(str, client->username);
	strcat(str, (char*)" has disconnected.\n");
	tcp_broadcast(str, client);
	remove_user(client);
}

void Server::log_str(char* log)
{
	log_size += strlen(log);

	fs.open("server_log.txt", std::fstream::out | std::fstream::app);
	fs << log;
	fs.close();
}

void Server::send_log(User* user)
{
	fs.open("server_log.txt", std::fstream::in);
	char* str = new char[buffer_size];
	memset(str, 0, buffer_size);

	//package size
	char* data = new char[5];
	memset(data, 0, 5);
	_itoa(log_size, data, 10);

	//send size
	socket_send(user->u_socket, data, strlen(data));

	while (!fs.eof()) {
		//read data
		memset(str, 0, buffer_size);
		fs.getline(str, buffer_size);
		strcat(str, (char*)"\n");

		//send data
		socket_send(user->u_socket, str, strlen(str));
	}

	fs.close();
}