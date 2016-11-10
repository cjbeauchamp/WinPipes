# WinPipes
This is a set of two programs and a library to highlight how separate programs can communicate with one another.

## Platform
Written in C++ and specifically designed for the Windows platform, but built on top of pipes - so the architecture can be replicated on other platforms.

## Architecture
There is a library (*pipe*) that provides the shared functions for each of the agent-based programs (*client/server*). The shared library should be included when compiling either agent. 

There must be synchronization between agents regarding the address of each pipe when they're initialized in each agent and the library. This is already done in the sample, but any changes must be propagated to all files to make sure each agent is connected to the right pipe. There are two pipes: broadcast and request. 

*Broadcast Pipe:* The server sends 1-way transmissions to any agents listening.
*Request Pipe:* The client sends a request, and expects a response in return from the server.

### pipe.h + pipe.cpp
These files will act as the library and hold the main communication. Some methods are exposed in `pipe.h` for use in the server/client programs.

### server.cpp
This will be the agent that broadcasts messages to any listeners and handles requests from other agents. 

A server is created to handle agent requests, with the callback handler `RequestHandler`. Any requests received will be processed by the handler and should respond appropriately.

Once the server is ready, you can enter broadcast messages using stdin - these are 1-way emits to any other agents that are listening on the broadcast pipe. 

### client.cpp
This will be the agent that listens for broadcasted messages and sends one-time requests to the main server agent. 

A server is created to listen for broadcasts, with the callback handler `BroadcastListener`. Any broadcasts received will be handled here.

Once the server is ready, you can enter requests using stdin. These are sent over the broadcast pipe and handled by the server's `RequestHandler`. 


## Run
To run a test, compile the client and server separately. I used VisualStudio command prompt as follows:

	cl /EHsc client.cpp pipe.cpp

and 

	cl /EHsc server.cpp pipe.cpp

You're now left with two new executables (client and server). Run the server in one window, then open another and run the client. You can now send messages back and forth between running programs. 