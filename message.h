#pragma once

#include <string>
#include <sstream>

using namespace std;

class Message
{
public:
	Message();
	~Message();

	string command;
	string param[2];
	string value;
	bool needed;
	bool parse_error;

	string message;

	string toString();

private:

};