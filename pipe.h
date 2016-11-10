#pragma once

#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#include <iostream>
#include <string>
using namespace std;

enum PipeTypes {
	PIPE_REQUEST,
	PIPE_BROADCAST
};

int create_server(string pipeName, void(*requestHandler)(HANDLE, string), PipeTypes pipeType);

int send_request(string message, void(*requestHandler)(HANDLE, string, string));

int send_response(HANDLE pipe, string message, void(*requestHandler)(HANDLE, string, string));
int emit_broadcast(string message);
