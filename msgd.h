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
#include <map>

#include "user.h"
#include "message.h"

using namespace std;

class Msgd {
public:
    Msgd(int port, bool debug);
    ~Msgd();

    void run();
    void handle(int client);
    
private:
    void create();
    void close_socket();
    void serve();

    

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

    bool debugFlag;

    sem_t user_lock;
    sem_t quu_lock;
    sem_t quu_notEmpty;
    sem_t cache_lock;

    vector<User> users;
    vector<pthread_t> threads;
    queue<int> quu;

    map<int,string> caches;

};
