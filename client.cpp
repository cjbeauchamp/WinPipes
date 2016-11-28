#include "pipe.h"

void BroadcastListener(HANDLE pipe, rapidxml::xml_node<>* node) {
	string message;

	rapidxml::xml_document<> doc;
	doc.append_node(node);

	rapidxml::print(back_inserter(message), doc);

	cout << "Received broadcast: " << message << endl;
}

void ResponseHandler(HANDLE pipe, rapidxml::xml_node<>* node) {

	string message;

	rapidxml::xml_document<> doc;
	doc.append_node(node);

	rapidxml::print(back_inserter(message), doc);

	cout << "Received response: " << message << endl;
}


int main(int argc, char** argv) {

	create_server("\\\\.\\pipe\\broadcast_pipe", BroadcastListener, PIPE_BROADCAST);

	cout << "Send your own request:" << endl;

	for (string line; getline(cin, line);) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* message = doc.allocate_node(rapidxml::node_element, line.c_str());

		send_request(message, ResponseHandler);
	}

	while (1) {
		Sleep(10000);
	}

	return 0;
}
