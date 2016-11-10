#include "pipe.h"

void BroadcastListener(HANDLE pipe, string message) {
	cout << "Received broadcast: " << message << endl;
}

void ResponseHandler(HANDLE pipe, string command, string message) {
	cout << "Received response [" << command << "]: " << message << endl;
}


int main(int argc, char** argv) {

	create_server("\\\\.\\pipe\\broadcast_pipe", BroadcastListener, PIPE_BROADCAST);

	cout << "Send your own request:" << endl;

	for (string line; getline(cin, line);) {
		send_request(line, ResponseHandler);
	}


	while (1) {
		Sleep(10000);
	}

	return 0;
}
