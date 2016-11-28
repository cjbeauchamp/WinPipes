#include "pipe.h"

void RequestHandler(HANDLE pipe, rapidxml::xml_node<>* node) {
	string message;

	rapidxml::xml_document<> doc;
	doc.append_node(node);
	rapidxml::print(back_inserter(message), doc);

	cout << "Received request: " << message << endl;

	string line = "Sup dude";
	rapidxml::xml_node<>* response = doc.allocate_node(rapidxml::node_element, "response", line.c_str());

//	string response = "echoing " + message;
//	if (message == "temp") { response = "39"; }
	send_response(pipe, response, NULL);
}

int main(int argc, char** argv) {

	create_server("\\\\.\\pipe\\request_pipe", RequestHandler, PIPE_REQUEST);

	cout << "Send your own broadcast:" << endl;

	for (string line; getline(cin, line);) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* message = doc.allocate_node(rapidxml::node_element, "broadcast", line.c_str());

		emit_broadcast(message);
	}


	while (1) {
		Sleep(10000);
	}

	return 0;
}

