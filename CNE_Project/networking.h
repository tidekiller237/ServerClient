#pragma once

#include "includes.h"

class Networking 
{
private:
	WSADATA wsaData;

public:
	int startup()
	{
		int result = WSAStartup(WINSOCK_VERSION, &wsaData);

		if (result != 0) {
			printf("WSAStartUp failed: %d\n", result);
		}

		return result;
	}
};