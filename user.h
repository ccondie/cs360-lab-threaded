#pragma once

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

class User
{
public:
	User(string name_t);
	~User();

	string list();
	string read(int index);

	string name;

	vector<string> subject;
	vector<string> message;


protected:



private:



};