#pragma once

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#include <string>
#include <iostream>
#include <vector>
#include <queue>

#include "user.h"
#include "message.h"

using namespace std;

class Msgd {
public:
    Msgd(int port, bool debug);
    ~Msgd();

    void run();
    
private:
    void create();
    void close_socket();
    void serve();
    void handle(int client);
    string get_request(int client);
    bool send_response(int client, string response);
    
    void debug(string message);
    string handPut(Message message);
    string handList(Message message);
    string handGet(Message message);

    Message parse(string message, int client);

    // void* thread_run(void *);

    int port_;
    int server_;
    int buflen_;
    char* buf_;

    bool debugFlag;
    bool commandFound;


    sem_t user_lock;
    sem_t quu_lock;
    sem_t quu_notEmpty;

    vector<User> users;
    vector<pthread_t> threads;
    queue<int> quu;

};
