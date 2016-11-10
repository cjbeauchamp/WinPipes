#include "pipe.h"

void RequestHandler(HANDLE pipe, string message) {
	cout << "Received request: " << message << endl;

	string response = "echoing " + message;
	if (message == "temp") { response = "39"; }
	send_response(pipe, response, NULL);
}

int main(int argc, char** argv) {

	create_server("\\\\.\\pipe\\request_pipe", RequestHandler, PIPE_REQUEST);

	cout << "Send your own broadcast:" << endl;

	for (string line; getline(cin, line);) {
		emit_broadcast(line);
	}


	while (1) {
		Sleep(10000);
	}

	return 0;
}

