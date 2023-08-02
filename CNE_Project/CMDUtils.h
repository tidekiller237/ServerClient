#pragma once

#include "includes.h"

using namespace std;

const int MAXLINES = 50;
const int MAXCHARS = 50;
const int serv_port = 31337;

class CMDUtils
{

public:

	HWND console;
	RECT consolerect;
	COORD incoming;
	COORD outgoing;
	HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	char* outputarr;

	void setup_window() {
		outgoing.X = 0;
		outgoing.Y = 0;
		incoming.X = 0;
		incoming.Y = 1;
	}

	int promptuser_options(char* prompt, int count, ...) {
		va_list options;
		int result = -1;

		while (true) {
			printf("%s", prompt);

			va_start(options, count);
			for (int i = 0; i < count; i++) {
				printf("%d: %s\n", i, va_arg(options, char*));
			}

			scanf_s("%d", &result);
			cin.ignore();

			if (result < 0 || result >= count) {
				pad();
				printf("%d is not a valid input.\n", result);
			}
			else
				break;
		}

		va_end(options);
		return result;
	}

	int promptuser_int(char* prompt, int invalid) {
		int result = invalid;
		
		while (true) {
			printf("%s", prompt);
			scanf_s("%i", &result);
			cin.ignore();

			if (result == invalid) {
				pad();
				printf("%i is not a valid input.\n", result);
			}
			else
				break;
		}

		return result;
	}

	unsigned int promptuser_uint(char* prompt) {
		int result = -1;

		while (true) {
			printf("%s", prompt);
			scanf_s("%i", &result);
			cin.ignore();

			if (result < 0) {
				pad();
				printf("%i is not a valid input.\n", result);
			}
			else
				break;
		}

		return result;
	}

	char* promptuser_str(char* prompt) {
		char* result = new char();

		while (true) {
			printf("%s", prompt);
			scanf("%s", result);
			cin.ignore();

			if (result == "") {
				pad();
				printf("That is not a valid input.\n");
			}
			else
				break;
		}

		return result;
	}

	void pad() {
		printf("\n\n");
	}

	char* appendchar(char* original, char* to_append) {
		size_t olen = strlen(original);
		size_t nlen = strlen(to_append);
		size_t tlen = olen + nlen + 1;

		char* result = new char[tlen];
		for (int i = 0; i < olen; i++) {
			result[i] = original[i];
		}

		for (int i = olen; i < tlen; i++) {
			result[i] = to_append[i - (olen)];
		}

		result[tlen - 1] = '\0';

		return result;
	}

	void print(size_t nargs, ...) {
		//write the message
		char* out = new char[MAXCHARS];
		va_list args;
		va_start(args, nargs);
		strcpy(out, va_arg(args, char*));

		for (int i = 1; i < nargs; i++) {
			strcat(out, va_arg(args, char*));
		}

		//set the cursor for writing
		SetConsoleCursorPosition(output, incoming);

		cout << out << endl;

		//move cursor back for reading
		SetConsoleCursorPosition(output, outgoing);
	}
};