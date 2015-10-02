#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include "msg.h"

using namespace std;

int
main(int argc, char **argv)
{
    int option;

    // setup default arguments
    int port = 3000;
    string host = "localhost";
    bool debug = false;

    // process command line options using getopt()
    // see "man 3 getopt"
    while ((option = getopt(argc,argv,"h:p:d")) != -1) {
        switch (option) 
        {
            case 'p':
                port = atoi(optarg);
                break;

            case 'h':
                host = optarg;
                break;

            case 'd':
                debug = true;
                break;

            default:
                cout << "msg [-h host] [-p port] [-d]" << endl;
                exit(EXIT_FAILURE);
        }
    }

    Msg msg = Msg(host, port, debug);
    msg.run();
}

