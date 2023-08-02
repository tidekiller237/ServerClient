#include "client.h"

int Client::init()
{
	//create udp socket
	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//set udp opts
	char opt = 1;
	int result = setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (result == SOCKET_ERROR)
		return OPT_FAILURE;

	//create sockaddr
	sockaddr_in udpaddr;
	udpaddr.sin_family = AF_INET;
	udpaddr.sin_addr.S_un.S_addr = INADDR_ANY;
	udpaddr.sin_port = htons(31337);

	//bind
	result = bind(udp_socket, (sockaddr*)&udpaddr, sizeof(udpaddr));
	if (result == SOCKET_ERROR)
		return BIND_FAILURE;

	//add to master set
	FD_SET(udp_socket, &masterset);

	SV_SUCCESS = new char[buffer_size];
	SV_FULL = new char[buffer_size];
	memset(SV_SUCCESS, 0, buffer_size);
	memset(SV_FULL, 0, buffer_size);
	SV_SUCCESS = (char*)"SV_SUCCESS";
	SV_FULL = (char*)"SV_FULL";

	timeout.tv_sec = 1;

	server_bcaddr.sin_family = AF_INET;
	server_bcaddr.sin_port = serv_port;

	return SUCCESS;
}

int Client::tcp_init(uint16_t port, char* address)
{
	//create the socket
	tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcp_socket == INVALID_SOCKET)
		return SOCKET_FAILURE;

	//connect
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr(address);
	server_addr.sin_port = htons(port);

	int result = connect(tcp_socket, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (result == SOCKET_ERROR)
		return CONNECTION_FAILURE;

	return SUCCESS;
}

void Client::stop()
{
	shutdown(tcp_socket, SD_BOTH);
	closesocket(tcp_socket);
}

void Client::output_override() {
	printf("\r> ");
	fflush(stdout);
}

void Client::recv_logf(int32_t size)
{
	int bytes_recvd = 0;
	char* inbuff = new char[buffer_size];
	memset(inbuff, 0, buffer_size);
	fstream stream;
	stream.open("client_log.txt", ios::out | ios::trunc);

	output_override();
	printf("%i\n", size);

	FD_ZERO(&readyset);

	while (bytes_recvd < size && stream.is_open()) {
		select(0, &readyset, NULL, NULL, &timeout);

		//recv new bytes
		int newbytes = recv(tcp_socket, inbuff, buffer_size, 0);
		bytes_recvd += newbytes;

		stream << inbuff;
	}

	stream.close();

	output_override();
	printf("Log received.\n");

	delete[] inbuff;

	recv_log = false;
}

int Client::client_main()
{
	//wait for server
	printf("\nSearching for server...\n");
	while (true) {
		memset(buff, 0, buffer_size);
		int bytes = 0;

		readyset = masterset;

		select(0, &readyset, NULL, NULL, &timeout);

		if (FD_ISSET(udp_socket, &readyset)) {
			int addrsize = sizeof(server_bcaddr);
			bytes = recvfrom(udp_socket, buff, buffer_size, 0, (sockaddr*)&server_bcaddr, &addrsize);
		}

		if (bytes == SOCKET_ERROR) {
			printf("\nSocket error\n");
		}
		else {
			//parse information
			char clientIp[256];
			ZeroMemory(clientIp, 256);

			inet_ntop(AF_INET, &server_bcaddr.sin_addr, clientIp, 256);

			//failed to get server
			if (strcmp(clientIp, (char*)"0.0.0.0") == 0) {
				continue;
			}

			cout << "Server found at " << clientIp << ":" << serv_port << endl;

			//connect to server
			int res = tcp_init(serv_port, clientIp);

			if (res == INVALID_SOCKET) {
				printf("\nInvalid socket\n");
				continue;
			}

			//close the udp socket
			shutdown(udp_socket, SD_BOTH);
			closesocket(udp_socket);

			break;
		}
	}

	delete[] buff;

	//get username
	while (username == nullptr) {
		utils.pad();

		username = utils.promptuser_str((char*)"Username: ");
	}

	//set up fd_sets
	//FD_SET(tcp_socket, &masterset);
	//timeout.tv_sec = 1;

	utils.pad();
	char clcommand[512];
	strcpy(clcommand, "$register ");
	strcat(clcommand, username);
	int clresult = send(tcp_socket, clcommand, strlen(clcommand), 0);

	//read connection result from server
	utils.pad();
	char* con_res = new char[buffer_size];
	memset(con_res, 0, buffer_size);
	int recv_res = recv(tcp_socket, con_res, buffer_size, 0);
	if (recv_res == 0) {
		printf("Failed to contact server.");
		stop();
		return 0;
	}
	
	if(strcmp(con_res, SV_SUCCESS) == 0)
		printf("Connected.");
	else if (strcmp(con_res, SV_FULL) == 0) {
		printf("Server full.\n");
		stop();
		system("pause");
		return 0;
	}
	utils.pad();

	delete[] con_res;

	//set up I/O threads
	s_done = false;
	r_done = false;
	shutdowncmd = false;
	recv_log = false;
	thread send_thread([this] {
		char* buffer = new char[buffer_size];
		char* msg = new char[buffer_size + strlen(username)];

		memset(buffer, 0, buffer_size);
		memset(msg, 0, buffer_size + strlen(username));

		while (!shutdowncmd) {
			while(recv_log){}

			output_override();

			cin.getline(buffer, buffer_size);

			if (strcmp(buffer, (char*)"$getlog") == 0) {
				recv_log = true;
			}

			char* temp = new char[buffer_size];
			strncpy(temp, buffer, buffer_size - strlen(username));
			strcat(temp, "\n\0");
			strcpy(msg, temp);
			int result = send(tcp_socket, msg, strlen(msg), 0);

			delete[] temp;
			memset(buffer, 0, buffer_size);
			memset(msg, 0, buffer_size + strlen(username));
		}

		delete[] buffer;
		delete[] msg;
		s_done = true;
		return;
	});

	thread recv_thread([this] {
		char* msg = new char[buffer_size];
		memset(msg, 0, buffer_size);

		while (!shutdowncmd) {
			//int result = read_message(msg, buffer_size);
			int result = recv(tcp_socket, msg, buffer_size, 0);

			if (recv_log) {
				//parse size
				int size = atoi(msg);

				if (size > 0)
					recv_logf(size);
				else {
					output_override();
					printf("Failed to receive log\n");
				}
			}
			else
			{
				if (strcmp(msg, (char*)"client_server_shutdown_command") == 0) {
					break;
				}

				if (result > 0) {
					printf("%s", msg);
					output_override();
				}
				else if (result == 0)
					break;
			}

			memset(msg, 0, buffer_size);
		}
		delete[] msg;
		r_done = true;
		return;
	});

	while (true) {
		if (r_done) {
			shutdowncmd = true;
			output_override();
			printf("Press enter to exit...");
			break;
		}
		else if (s_done) {
			shutdowncmd = true;
			output_override();
			printf("Something went wrong. Input thread exit.");
			break;
		}
	}
		
	stop();
	send_thread.join();
	recv_thread.join();

	return 0;
}