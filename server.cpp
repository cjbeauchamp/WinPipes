#include "pipe.h"

void RequestHandler(HANDLE pipe, rapidxml::xml_node<>* node) {
	rapidxml::xml_document<> doc;

	// pull out the actual request
	rapidxml::xml_node<>* request = doc.clone_node(node->first_node("request")->first_node());

	string message;
	doc.append_node(request);
	rapidxml::print(back_inserter(message), doc);
	cout << "Received request: " << message << endl;

	// based on what's in it, generate our response

	// default response
	rapidxml::xml_node<>* response = doc.allocate_node(rapidxml::node_element, "basic", "1234");

	// assuming the request has a single node. Should be the only use case after spec
	string requestName = request->name();
	if (requestName == "temp") {
		response = doc.allocate_node(rapidxml::node_element, "temp", "48");
	}

	send_response(request, response, NULL);
}

int main(int argc, char** argv) {

	create_server("\\\\.\\pipe\\request_pipe", RequestHandler, PIPE_REQUEST);

	cout << "Send your own broadcast:" << endl;

	for (string line; getline(cin, line);) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* message = doc.allocate_node(rapidxml::node_element, "maintenance", line.c_str());

		emit_broadcast(message);
	}


	while (1) {
		Sleep(10000);
	}

	return 0;
}

