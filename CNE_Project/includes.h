#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <filesystem>
#include <stdint.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <cstdarg>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "networking.h"
#include "CMDUtils.h"
#include "errorcodes.h"

using namespace std;