
#include "includes.h"
#include "client.h"
#include "server.h"
#include <atomic>
#include <thread>

using namespace std;

WSADATA wsaData;

int main()
{
	//initialize winsock
	int iresult = WSAStartup(WINSOCK_VERSION, &wsaData);
	if (iresult != 0) {
		printf("WSAStartup failed: %d\n", iresult);
		return 1;
	}

	//objects
	Client* client = nullptr;
	Server* server = nullptr;

	//utility
	CMDUtils utils;

	//0 == server; 1 == client
	int mode = -1;

	//mode select
	while (mode == -1) {

		//select mode
		utils.pad();
		mode = utils.promptuser_options((char*)"Select mode:\n", 2, "Server", "Client");
	}

#pragma region Client

	if (mode == 1) {
		utils.pad();
		printf("Started in CLIENT mode.");

		while (mode == 1 && client == nullptr) {
			utils.pad();

			//create client
			/*char* address = utils.promptuser_str((char*)"Server IP Address: ");
			int port = utils.promptuser_uint((char*)"\nServer Port: ");*/

			client = new Client();
			int result = client->init();

			if (result != SUCCESS) {
				utils.pad();

				//asses error
				if (result == SOCKET_FAILURE)
					printf("Socket error");
				else if (result == CONNECTION_FAILURE)
					printf("Failed to connect");
				else
					printf("An unknown error occured");

				//clear failed client
				delete client;
				client = nullptr;
			}
		}

		int clientexit = client->client_main();

		if (clientexit == 0) {
			printf("Exit clean\n");
		}

		delete client;

		iresult = WSACleanup();
		return 0;
	}

#pragma endregion


#pragma region Server

	if (mode == 0) {
		utils.pad();
		printf("Started in SERVER mode.\n");

		while (server == nullptr) {
			utils.pad();

			server = new Server();
			int result = server->init(serv_port);
			if (result != SUCCESS) {
				utils.pad();

				if (result == SOCKET_FAILURE)
					printf("\nSocket error\n");
				else if (result == BIND_FAILURE) {
					printf("\nFailed to bind socket\n");
					cout << "\n" << WSAGetLastError() << endl;
				}
				else if (result == SETUP_FAILURE) {
					printf("\nFailed to setup socket for listening\n");
					cout << "\n" << WSAGetLastError() << endl;
				}
				else
					printf("\nAn unknown error occured\n");

				delete server;
				server = nullptr;
				continue;
			}
			
			break;
		}

		int serverexit = server->server_main();

		if (serverexit == 0) {
			printf("Exit clean\n");
		}

		delete server;

		iresult = WSACleanup();
		return 0;
	}

#pragma endregion

	iresult = WSACleanup();
	utils.pad();
	system("PAUSE");
	return 0;
}