#include "msgd.h"

Msgd::Msgd(int port, bool debug_t) 
{
    debug("Msgd::constructor");
    // setup variables
    port_ = port;
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
    debugFlag = debug_t;

    sem_init(&user_lock, 0 ,1);
    sem_init(&quu_lock, 0, 1);
    sem_init(&quu_notEmpty, 0 , 0);
}

Msgd::~Msgd() 
{
    debug("Msgd::destructor");
    delete buf_;
}

struct package
{
    sem_t* quu_notEmpty;
    sem_t* quu_lock;
    sem_t* user_lock;

    queue<int>* quu;

};


void*
thread_run(void *vptr)
{
    struct package* pack = (struct package*) vptr;
    int currentClient;
    queue<int>* quu = pack->quu;

    cout << "I AM " << pthread_self() << endl;
    while(1)
    {
        //wait for the quu not to be empty
        sem_wait(pack->quu_notEmpty);
        
        //get the front of the queue
        sem_wait(pack->quu_lock);
        currentClient = quu->front();
        quu->pop();
        sem_post(pack->quu_lock);

        //handle the request of this client
        handle(currentClient);

        //return it to the end
        //      sem_post()
        //  OR
        //remove it
        //      NOTHING

    }
}

void
Msgd::run()
{
    // create and run the Msgd
    create();

    //create the pthreads
    for(int i = 0; i < 10; i++)
    {
        debug("Msgd::run()::creating thread");

        struct package pack;
        pack.quu_notEmpty = &quu_notEmpty;
        pack.quu_lock = &quu_lock;
        pack.user_lock = &user_lock;
        pack.quu = &quu;

        pthread_t temp;
        threads.push_back(pthread_create(&temp, NULL, &thread_run, &pack));
    }

    serve();
}



void
Msgd::create() 
{
    debug("Msgd::create()");

    struct sockaddr_in server_addr;

    // setup socket address structure
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // create socket
    server_ = socket(PF_INET,SOCK_STREAM,0);
    if (!server_) {
        perror("socket");
        exit(-1);
    }

    // set socket to immediately reuse port when the application closes
    int reuse = 1;
    if (setsockopt(server_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(-1);
    }

    // call bind to associate the socket with our local address and
    // port
    if (bind(server_,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("bind");
        exit(-1);
    }

    // convert the socket to listen for incoming connections
    if (listen(server_,SOMAXCONN) < 0) {
        perror("listen");
        exit(-1);
    }
}

void
Msgd::close_socket() 
{
    debug("Msgd::close_socket()");
    close(server_);
}

void
Msgd::serve() 
{
    debug("Msgd::serve()");

    // setup client
    int client;
    struct sockaddr_in client_addr;
    socklen_t clientlen = sizeof(client_addr);

    // accept clients
    while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0)
    {
        sem_wait(&quu_lock);
        quu.push(client);
        sem_post(&quu_lock);

        sem_post(&quu_notEmpty);

        // cout << "A" << endl;
        // handle(client);
        // cout << "B" << endl;
    }
    close_socket();
}

void
Msgd::handle(int client) 
{
    Message message;
    bool success;

    debug("Msgd::handle()");

    // get a request
    string request = get_request(client);

    // break if client is done or an error occurred
    if (request.empty())
    {
        close(client);
        return;
    }

    //parse request
    message = parse(request, client);

    //handle message
    commandFound = false;

    if(message.command == "put")
    {
        commandFound = true;
        success = send_response(client, handPut(message));
    }

    if(message.command == "list")
    {
        commandFound = true;
        success = send_response(client, handList(message));
    }

    if(message.command == "get")
    {
        commandFound = true;
        success = send_response(client, handGet(message));
    }

    if(message.command == "reset")
    {
        commandFound = true;
        for(int i = 0; i < users.size(); i++)
        {
            users.at(i).subject.clear();
            users.at(i).message.clear();
        }
        users.clear();

        success = send_response(client, "OK\n");
    }

    if(!commandFound)
    {
        //throw error
        bool success = send_response(client,"error: unexpected command\n");
    }

    close(client);
}


Message
Msgd::parse(string request, int client)
{
    Message message;
    istringstream iss(request);
    string args;

    getline(iss,args,'\n');
    istringstream arg_line(args);

    getline(arg_line, message.command, ' ');


    bool commandFound = false;
    if(message.command == "put")
    {
        debug("Msgd::parse(): ifcase: put");

        commandFound = true;
        getline(arg_line, message.param[0], ' ');
        getline(arg_line, message.param[1], ' ');
        getline(arg_line, message.value, ' ');

        stringstream ss;
        bool start = true;
        //process the data let inside the in stream
        while(!iss.eof())
        {
            if(start)
                start = false; 
            else
                ss << '\n';

            string content_line;
            getline(iss, content_line);
            debug("Msgd::parse():: parsingLine:" + content_line);
            ss << content_line;
        }

        int streamSize = ss.str().size();
        int messageSize = atoi(message.value.c_str());

        if(streamSize > 0)
            message.cache = ss.str();

        if(streamSize < messageSize)
            message.needed = true;
        else
            message.needed = false;

        //handle the event where we need more data
        if(message.needed == true)
        {
            string request = message.cache;
            while(atoi(message.value.c_str()) > request.size())
            {
                string newCache = get_request(client);
                request.append(newCache);
            }
            message.cache = request;
        }

        debug("RANDOM TEST\n" + message.toString());

        //at this point we shoud have a compelte name, subject, value, and content
    }

    if(message.command == "list")
    {
        debug("Msgd::parse(): ifcase: list");

        commandFound = true;
        getline(arg_line, message.param[0], ' ');
        
        message.value = "0";
        message.needed = false;
        message.cache = "";
    }

    if(message.command == "get")
    {
        debug("Msgd::parse(): ifcase: get");

        commandFound = true;
        getline(arg_line, message.param[0], ' ');
        getline(arg_line, message.param[1], ' ');
        message.value = "0";
        message.needed = false;
        message.cache = "";
    }

    if(message.command == "reset")
    {
        commandFound = true;
    }

    return message;

}


string
Msgd::handPut(Message message)
{
    debug("Msgd::handPut()");
    debug(message.toString());

    bool userFound = false;

    for(int i = 0; i < users.size(); i++)
    {
        if(users.at(i).name == message.param[0])
        {
            userFound = true;
            users.at(i).subject.push_back(message.param[1]);
            users.at(i).message.push_back(message.cache);
        }
    }

    if(!userFound)
    {
        User newUser(message.param[0]);
        newUser.subject.push_back(message.param[1]);
        newUser.message.push_back(message.cache);
        users.push_back(newUser);
    }

    return "OK\n";
}



string
Msgd::handList(Message message)
{
    debug("Msgd::handList()");

    bool userFound = false;

    stringstream resp;

    debug("Msgd::handList()::primaryFOR");
    for(int i = 0; i < users.size(); i++)
    {
        if(users.at(i).name == message.param[0])
        {
            debug("Msgd::handList()::User Found");
            userFound = true;
            resp << users.at(i).list();
        }
    }

    if(!userFound)
    {
        debug("Msgd::handList()::userNotFound");
        //send an error
        return "error user not found\n";
    }

    return resp.str();
}

string
Msgd::handGet(Message message)
{
    debug("Msgd::handGet()");
    debug(message.toString());

    int index = atoi(message.param[1].c_str());

    bool userFound = false;

    stringstream resp;

    debug("Msgd::handGet()::primaryFOR");
    for(int i = 0; i < users.size(); i++)
    {
        if(users.at(i).name == message.param[0])
        {
            debug("Msgd::handGet()::User Found");
            
            userFound = true;
            if(index >= users.at(i).subject.size())
                return "error: index out of bounds";
            else
            resp << users.at(i).read(index);
        }
    }

    if(!userFound)
    {
        debug("Msgd::handGet()::userNotFound");
        //send an error
        return "error user not found\n";
    }

    return resp.str();
}



string
Msgd::get_request(int client) 
{
    debug("Msgd::get_request()");

    string request = "";
    bool newlineFound = false;

    // read until we get a newline
    // while (request.find("\n") == string::npos) 
    while (!newlineFound) 
    {   

        debug("\tMsgd::get_request()::while");

        //read from the connection
        int nread = recv(client,buf_,1024,0);

        //check for an recv error
        if (nread < 0) 
        {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return "";
        } 
        else if (nread == 0) 
        {
            // the socket is closed
            return "";
        }

        string grab_recv = "";
        grab_recv.append(buf_, nread);
        debug("Msgd::get_request()::appended from buf_:" + grab_recv);

        if(grab_recv.find("\n") != string::npos)
        {
            debug("Msgd::get_request()::found newline");

            string preline;
            string postline;

            //isolate what came before the newline
            istringstream iss(grab_recv);
            getline(iss, preline, '\n');

            debug("Msgd::get_request()::preline:" + preline);

            while(!iss.eof())
            {
                string dummy;
                getline(iss, dummy, '\n');

                postline.append(dummy + '\n');
            }

            debug("Msgd::get_request()::postline:BEGIN|" + postline + "|END");

            //newline found in the last grab from the recv function
            newlineFound = true;
        }
        else
        {
            //a newline was NOT found

            // be sure to use append in case we have binary data
            request.append(grab_recv);
        }
    }
    // a better server would cut off anything after the newline and
    // save it in a cache
    return request;
}

bool
Msgd::send_response(int client, string response) 
{
    debug("Msgd::send_response()");
    // prepare to send response
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(client, ptr, nleft, 0)) < 0) {
            if (errno == EINTR) {
                // the socket call was interrupted -- try again
                continue;
            } else {
                // an error occurred, so break out
                perror("write");
                return false;
            }
        } else if (nwritten == 0) {
            // the socket is closed
            return false;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return true;
}

void
Msgd::debug(string message)
{
    if(debugFlag)
        cout << "\t" << message << endl;
}