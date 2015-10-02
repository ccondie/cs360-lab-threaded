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

using namespace std;

class Msg 
{
public:
    Msg(string host, int port, bool debug);
    ~Msg();

    void run();

private:
    virtual void create();
    virtual void close_socket();
    void echo();
    void message();

    void msg_send(string command);
    void msg_list(string command);
    void msg_read(string command);
    void msg_reset();

    bool send_request(string);
    bool get_response();

    void debug(string message);

    string host_;
    int port_;
    int server_;
    int buflen_;
    char* buf_;

    bool debugFlag;
};
