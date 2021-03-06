#include "pipe.h"

#define BUFSIZE 512

HANDLE requestPipe = NULL;
HANDLE broadcastPipe = NULL;

struct read_params {
	HANDLE pipe;
	void(*requestHandler)(HANDLE, rapidxml::xml_node<>* node);
} params;

struct ServerParams {
	string pipeName;
	PipeTypes pipeType;
	void(*requestHandler)(HANDLE, rapidxml::xml_node<>* node);
};

int send_body(HANDLE pipe, rapidxml::xml_node<>* node, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*)) {

	TCHAR  chBuf[BUFSIZE];
	BOOL   fSuccess = FALSE;
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;

	string message;
	rapidxml::xml_document<> doc;
	doc.append_node(node);
	rapidxml::print(back_inserter(message), doc);

	cbToWrite = (strlen(message.c_str()) + 1) * sizeof(TCHAR);
	//cout << "Sending " << cbToWrite << " byte message[" << pipe << "]: \"" << message << "\"" << endl;

	fSuccess = WriteFile(
		pipe,                  // pipe handle 
		message.c_str(),           // message 
		cbToWrite,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		cerr << "WriteFile to pipe failed (GLE=" << GetLastError() << ")" << endl;
		return -1;
	}

	//cout << "Message sent to server" << endl;

	// if we're expecting a response
	// TODO: make a timeout in case we never get anything
	if (requestHandler != NULL) {
		do
		{
			// Read from the pipe. 
			fSuccess = ReadFile(
				pipe,    // pipe handle 
				chBuf,    // buffer to receive reply 
				BUFSIZE * sizeof(TCHAR),  // size of buffer 
				&cbRead,  // number of bytes read 
				NULL);    // not overlapped 

			if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
				break;

			// should just read in from the buffer and spit it back to the handler

			// parse chBuf into xml doc
			rapidxml::xml_document<> respDoc;
			respDoc.parse<0>(respDoc.allocate_string(chBuf));
			rapidxml::xml_node<>* clonedResponse = doc.clone_node(respDoc.first_node());

			requestHandler(pipe, clonedResponse);

		} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

		if (!fSuccess)
		{
			cerr << "ReadFile from pipe failed (GLE=" << GetLastError() << ")" << endl;
			return -1;
		}

	}

	return 1;
}

/*
int send_response(HANDLE pipe, rapidxml::xml_node<>* node, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*)) {

	TCHAR  chBuf[BUFSIZE];
	BOOL   fSuccess = FALSE;
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;

	string message;
	rapidxml::xml_document<> doc;
	doc.append_node(node);
	rapidxml::print(back_inserter(message), doc);

	cbToWrite = (strlen(message.c_str()) + 1) * sizeof(TCHAR);
	//cout << "Sending " << cbToWrite << " byte message[" << pipe << "]: \"" << message << "\"" << endl;

	fSuccess = WriteFile(
		pipe,                  // pipe handle 
		message.c_str(),           // message 
		cbToWrite,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		cerr << "WriteFile to pipe failed (GLE=" << GetLastError() << ")" << endl;
		return -1;
	}

	//cout << "Message sent to server" << endl;

	// if we're expecting a response
	// TODO: make a timeout in case we never get anything
	if (requestHandler != NULL) {
		do
		{
			// Read from the pipe. 
			fSuccess = ReadFile(
				pipe,    // pipe handle 
				chBuf,    // buffer to receive reply 
				BUFSIZE * sizeof(TCHAR),  // size of buffer 
				&cbRead,  // number of bytes read 
				NULL);    // not overlapped 

			if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
				break;

			rapidxml::xml_document<> doc;
			rapidxml::xml_node<>* parent = doc.allocate_node(rapidxml::node_element, "body");

			// parse chBuf into xml doc
			rapidxml::xml_document<> respDoc;
			respDoc.parse<0>(respDoc.allocate_string(chBuf));
			rapidxml::xml_node<>* clonedResponse = doc.clone_node(respDoc.first_node());

			rapidxml::xml_node<>* clonedRequest = doc.clone_node(node);

			parent->append_node(clonedResponse);
			parent->append_node(clonedRequest);

			requestHandler(pipe, parent);

		} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

		if (!fSuccess)
		{
			cerr << "ReadFile from pipe failed (GLE=" << GetLastError() << ")" << endl;
			return -1;
		}

	}

	return 1;
}
*/

DWORD WINAPI ReadHandler(LPVOID lpvParam)
{
	struct read_params *p = (struct read_params*) lpvParam;
	TCHAR  chBuf[BUFSIZE];
	BOOL   fSuccess = FALSE;
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;

	//cout << "Pipe: " << p->pipe << endl;

	while (1) {

		fSuccess = ReadFile(
			p->pipe,    // pipe handle 
			chBuf,    // buffer to receive reply 
			BUFSIZE * sizeof(TCHAR),  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 

		if (!fSuccess || cbRead == 0) {
			if (GetLastError() == ERROR_BROKEN_PIPE) {
				cerr << "Client disconnected" << endl;
			}
			else {
				cerr << "ReadFile Failed: " << GetLastError() << endl;
			}
			break;
		}

		//cout << "received: " << chBuf << endl;

		// parse chBuf into xml doc
		rapidxml::xml_document<> readDoc;
		readDoc.parse<0>(readDoc.allocate_string(chBuf));

//		string rdMessage;
//		rapidxml::print(back_inserter(rdMessage), readDoc);
//		cout << "Read message:" << endl;
//		cout << rdMessage << endl;

		rapidxml::xml_node<>* cloned = readDoc.clone_node(readDoc.first_node());

		p->requestHandler(p->pipe, cloned);
	}

	//cout << "Exiting the read handler" << endl;

	return 0;
}

void startRead(HANDLE pipeFile, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*)) {

	TCHAR  chBuf[BUFSIZE];
	DWORD  cbRead, cbToWrite, cbWritten;

	//cout << "Making a read thread" << endl;

	struct read_params *p = (struct read_params *) malloc(sizeof(struct read_params));

	p->pipe = pipeFile;
	p->requestHandler = requestHandler;

	// spin off a 'read' thread
	DWORD  threadId = 0;
	HANDLE thread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		ReadHandler,    // thread proc
		(LPVOID)p,      // thread parameter 
		0,                 // not suspended 
		&threadId);        // returns thread ID 

	if (thread == NULL) {
		cerr << "CreateThread failed (GLE=" << GetLastError() << ")" << endl;
		return;
	}
	else {
		CloseHandle(thread);
	}

	//cout << "Start read done" << endl;
}


DWORD WINAPI WriteHandler(LPVOID lpvParam) {

	//cout << "Created write handler" << endl;

	HANDLE pipe = (HANDLE)lpvParam;
	TCHAR  chBuf[BUFSIZE];
	BOOL   fSuccess = FALSE;
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;

	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		pipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if (!fSuccess)
	{
		cerr << "SetNamedPipeHandleState to pipe failed (GLE=" << GetLastError() << ")" << endl;
		return -1;
	}

	//cout << "Opened pipe: " << pipe << endl;

	return 0;
}

void startWrite(HANDLE pipe) {

	// spin off a 'write' thread
	DWORD  threadId = 0;
	HANDLE thread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		WriteHandler,    // thread proc
		(LPVOID)pipe,      // thread parameter 
		0,                 // not suspended 
		&threadId);        // returns thread ID 

	if (thread == NULL) {
		cerr << "CreateThread failed (GLE=" << GetLastError() << ")" << endl;
		return;
	}
	else {
		CloseHandle(thread);
	}

}


int openPipe(string pipeName, HANDLE &pipeFile) {

	while (1) {
		pipeFile = CreateFile(
			pipeName.c_str(),   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

							// Break if the pipe handle is valid. 

		if (pipeFile != INVALID_HANDLE_VALUE) {
			//cout << "Connected to pipe file" << endl;
			startWrite(pipeFile);

			return 1;
		}

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			cerr << "Could not open pipe (GLE=" << GetLastError() << ")" << endl;
			return -1;
		}

		// All pipe instances are busy, so wait for 20 seconds. 

		if (!WaitNamedPipe(pipeName.c_str(), 20000))
		{
			cerr << "Could not open pipe: 20 second wait timed out." << endl;
			return -1;
		}
	}

	return 0;
}


DWORD WINAPI CreateServerHandler(LPVOID lpvParam) {

	struct ServerParams *p = (struct ServerParams*) lpvParam;

	for (;;) {
		//cout << "Aaeon server: Main thread awaiting Kii connection on " << p->pipeName << endl;
		HANDLE pipe = CreateNamedPipe(
			p->pipeName.c_str(),             // pipe name 
			PIPE_ACCESS_DUPLEX,       // read/write access 
			PIPE_TYPE_MESSAGE |       // message type pipe 
			PIPE_READMODE_MESSAGE |   // message-read mode 
			PIPE_WAIT,                // blocking mode 
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			BUFSIZE,                  // output buffer size 
			BUFSIZE,                  // input buffer size 
			0,                        // client time-out 
			NULL);                    // default security attribute 

		if (pipe == INVALID_HANDLE_VALUE)
		{
			cerr << "CreateNamedPipe failed (GLE=" << GetLastError() << ")" << endl;
			return -1;
		}

		// Wait for the client to connect; if it succeeds, 
		// the function returns a nonzero value. If the function
		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

		BOOL connected = ConnectNamedPipe(pipe, NULL) ?
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (connected) {
			//cout << "Connected!" << endl;

			// put it in the right spot
			if (p->pipeType == PIPE_REQUEST) {
				requestPipe = pipe;
			}
			else if (p->pipeType == PIPE_BROADCAST) {
				broadcastPipe = pipe;
			}

			startRead(pipe, p->requestHandler);
		}
		else {
			// The client could not connect, so close the pipe. 
			CloseHandle(pipe);
			return -1;
		}
	}

}

int create_server(string pipeName, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*), PipeTypes pipeType) {

	struct ServerParams *p = (struct ServerParams *) malloc(sizeof(struct ServerParams));

	p->pipeName = pipeName;
	p->requestHandler = requestHandler;
	p->pipeType = pipeType;

	//cout << "Spinning off write thread" << endl;

	// spin off a 'write' thread
	DWORD  threadId = 0;
	HANDLE thread = CreateThread(
		NULL,              // no security attribute 
		0,                 // default stack size 
		CreateServerHandler,    // thread proc
		(LPVOID)p,      // thread parameter 
		0,                 // not suspended 
		&threadId);        // returns thread ID 

	if (thread == NULL) {
		cerr << "CreateThread failed (GLE=" << GetLastError() << ")" << endl;
		return -1;
	}
	else {
		CloseHandle(thread);
	}

	return 0;
}

int emit_broadcast(rapidxml::xml_node<>* node) {
	// make sure we open the pipe first
	// TODO: make a timeout or something in case the pipe isn't available
	while (broadcastPipe == NULL) {
		openPipe("\\\\.\\pipe\\broadcast_pipe", broadcastPipe);
	}

	rapidxml::xml_document<> doc;
	rapidxml::xml_node<>* body = doc.allocate_node(rapidxml::node_element, "body");
	rapidxml::xml_node<>* bcast = doc.allocate_node(rapidxml::node_element, "broadcast");
	bcast->append_node(node);
	body->append_node(bcast);

	send_body(broadcastPipe, body, NULL);
	return 1;
}

int send_request(rapidxml::xml_node<>* node, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*)) {
	// make sure we open the pipe first
	// TODO: make a timeout or something in case the pipe isn't available
	while (requestPipe == NULL) {
		openPipe("\\\\.\\pipe\\request_pipe", requestPipe);
	}

//	rapidxml::xml_node<>* clonedResponse = doc.clone_node(respDoc.first_node());

	rapidxml::xml_document<> doc;
	rapidxml::xml_node<>* body = doc.allocate_node(rapidxml::node_element, "body");
	rapidxml::xml_node<>* request = doc.allocate_node(rapidxml::node_element, "request");
	request->append_node(node);
	body->append_node(request);

	send_body(requestPipe, body, requestHandler);
	
	return 1;
}


int send_response(rapidxml::xml_node<>* request, rapidxml::xml_node<>* response, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*)) {

	/* this should already be open
	// make sure we open the pipe first
	// TODO: make a timeout or something in case the pipe isn't available
	while (requestPipe == NULL) {
		openPipe("\\\\.\\pipe\\request_pipe", requestPipe);
	}
	*/

	rapidxml::xml_document<> doc;
	rapidxml::xml_node<>* body = doc.allocate_node(rapidxml::node_element, "body");
	rapidxml::xml_node<>* docRequest = doc.allocate_node(rapidxml::node_element, "request");
	rapidxml::xml_node<>* docResponse = doc.allocate_node(rapidxml::node_element, "response");

	docRequest->append_node(doc.clone_node(request));
	docResponse->append_node(doc.clone_node(response));

	body->append_node(docRequest);
	body->append_node(docResponse);

	send_body(requestPipe, body, requestHandler);

	return 1;
}
