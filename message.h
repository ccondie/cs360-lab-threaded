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

	string cache;

	string toString();

private:

};