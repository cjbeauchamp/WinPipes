#pragma once

#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#include <iostream>
#include <string>
#include <iterator>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

using namespace std;

enum PipeTypes {
	PIPE_REQUEST,
	PIPE_BROADCAST
};

int create_server(string pipeName, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*), PipeTypes pipeType);

int send_request(rapidxml::xml_node<>* node, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*));

int send_response(HANDLE pipe, rapidxml::xml_node<>* node, void(*requestHandler)(HANDLE, rapidxml::xml_node<>*));
int emit_broadcast(rapidxml::xml_node<>* xmlDoc);
