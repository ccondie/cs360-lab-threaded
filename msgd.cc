#include "msgd.h"

Msgd::Msgd(int port, bool debug_t) 
{
    // setup variables
    port_ = port;
    buflen_ = 1024;
    debugFlag = debug_t;
    debug("Msgd::constructor");

    sem_init(&user_lock, 0 ,1);
    sem_init(&quu_lock, 0, 1);
    sem_init(&cache_lock, 0, 1);
    sem_init(&quu_notEmpty, 0 , 0);
}

Msgd::~Msgd() 
{
    debug("Msgd::destructor");
}

struct package
{
    sem_t* quu_notEmpty;
    sem_t* quu_lock;

    queue<int>* quu;
    Msgd* self_pointer;

};


void*
thread_run(void *vptr)
{
    struct package* pack = (struct package*) vptr;
    int currentClient;
    queue<int>* quu = pack->quu;
    Msgd* self = pack->self_pointer;

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

        // cout << "thread " << pthread_self() << " running with client " << currentClient << endl;
        self->handle(currentClient);
    }
}

void
Msgd::run()
{
    // create and run the Msgd
    create();

    struct package pack;
    pack.quu_notEmpty = &quu_notEmpty;
    pack.quu_lock = &quu_lock;
    pack.quu = &quu;
    pack.self_pointer = this;

    //create the pthreads
    for(int i = 0; i < 10; i++)
    {
        
        debug("Msgd::run()::creating thread");
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
        debug("Msgd::handle()::request_error");
        close(client);
        {
            stringstream ss;
            ss << client;
            debug("Msgd::handle()::clientClosed:" + ss.str());
        }
        return;
    }

    //parse request
    message = parse(request, client);

    if(message.parse_error == true)
    {
        debug("Msgd::handle::parse_error");
        success = send_response(client, "error parse_error");
        // close(client);
        sem_wait(&quu_lock);
        quu.push(client);
        sem_post(&quu_lock);
        sem_post(&quu_notEmpty);
        return;
    }

    //handle message
    bool commandFound = false;

    debug("Msgd::handle::postParse:" + '\n' + message.toString());

    if(message.command == "put")
    {
        debug("Msgd::handle::put");
        commandFound = true;
        success = send_response(client, handPut(message));

        {
            stringstream ss;
            ss << success;
            debug("Msgd::send_response()::success:" + ss.str());
        }
    }

    if(message.command == "list")
    {
        debug("Msgd::handle::list");
        commandFound = true;
        success = send_response(client, handList(message));
        {
            stringstream ss;
            ss << success;
            debug("Msgd::send_response()::success:" + ss.str());
        }
    }

    if(message.command == "get")
    {
        debug("Msgd::handle::get");
        commandFound = true;
        success = send_response(client, handGet(message));
        {
            stringstream ss;
            ss << success;
            debug("Msgd::send_response()::success:" + ss.str());
        }
    }

    if(message.command == "reset")
    {
        debug("Msgd::handle::reset");

        sem_wait(&user_lock);
        commandFound = true;
        for(int i = 0; i < users.size(); i++)
        {
            users.at(i).subject.clear();
            users.at(i).message.clear();
        }
        users.clear();
        sem_post(&user_lock);

        success = send_response(client, "OK\n");
        {
            stringstream ss;
            ss << success;
            debug("Msgd::send_response()::success:" + ss.str());
        }
    }

    if(!commandFound)
    {
        debug("Msgd::handle::commandNotFound:request:" + request);
        //throw error
        bool success = send_response(client,"error unexpected command");
        {
            stringstream ss;
            ss << success;
            debug("Msgd::send_response()::success:" + ss.str());
        }
        debug(message.toString());
        close(client);
        return;
    }

    sem_wait(&quu_lock);
    quu.push(client);
    sem_post(&quu_lock);
    sem_post(&quu_notEmpty);
}


Message
Msgd::parse(string request, int client)
{ 
    {
        debug("Msgd::parse()");
        stringstream ss;
        ss << client;
        debug("Msgd::parse()::client:" + ss.str());
        ss.str("");
        ss << request;
        debug("Msgd::parse()::request:" + ss.str());
    }
    

    Message message;
    message.parse_error = false;
    istringstream iss(request);

    getline(iss, message.command, ' ');

    bool commandFound = false;

    //is the command is a put command
    if(message.command == "put")
    {
        commandFound = true;

        debug("Msgd::parse()::put");

        //assign the parameters from the request into the message object
        getline(iss, message.param[0], ' ');
        getline(iss, message.param[1], ' ');
        getline(iss, message.value, ' ');

        if((message.param[0].length() == 0) || (message.param[1].length() == 0) ||
            (message.value.length() == 0))
        {
            message.parse_error = true;
            return message;
        }

        //aquire the message of the put command
        //grab the cache from the get_request
        sem_wait(&cache_lock);
        string client_cache = caches[client];
        sem_post(&cache_lock);

        //in the event that the cache is the same size as the needed string then we should be done
        if(client_cache.length() == atoi(message.value.c_str()))
        {
            {
                stringstream ss;
                debug("Msgd::parse()::put::cache == to value");
                ss << client_cache.length();
                debug("Msgd::parse()::put::cache.size:" + ss.str());
                ss.str("");
                debug("Msgd::parse()::put::cache:" + client_cache + "|END");
                ss << message.value;
                debug("Msgd::parse()::put::message.value:" + ss.str());
                ss.str("");

            }  

            message.message = client_cache;
            message.needed = false;
        }
        else if(client_cache.length() < atoi(message.value.c_str()))
        {
            //this is the event where not all the message make it trough the get_request
            //will have to call recv via a get_value call
            {
                stringstream ss;
                debug("Msgd::parse()::put::cache < to value");
                ss << client_cache.length();
                debug("Msgd::parse()::put::cache.size:" + ss.str());
                ss.str("");
                debug("Msgd::parse()::put::cache:" + client_cache + "|END");
                ss << message.value;
                debug("Msgd::parse()::put::message.value:" + ss.str());
                ss.str("");

            }  


        }
        else if(client_cache.length() > atoi(message.value.c_str()))
        {
            //this is the event where something broke, the cache is bigger than it should be
            
            {
                stringstream ss;
                debug("Msgd::parse()::put::cache > to value");
                ss << client_cache.length();
                debug("Msgd::parse()::put::cache.size:" + ss.str());
                ss.str("");
                debug("Msgd::parse()::put::cache:" + client_cache + "|END");
                ss << message.value;
                debug("Msgd::parse()::put::message.value:" + ss.str());
                ss.str("");

            }  
        }
    }


    //if the command is a list command
    if(message.command == "list")
    {
        commandFound = true;
        debug("Msgd::parse()::list");

        
        getline(iss, message.param[0], ' ');
        
        message.value = "0";
        message.needed = false;
        message.message = "";
    }

    //if the command is a get command
    if(message.command == "get")
    {
        commandFound = true;
        debug("Msgd::parse()::get");

        
        getline(iss, message.param[0], ' ');
        getline(iss, message.param[1], ' ');
        message.value = "0";
        message.needed = false;
        message.message = "";
    }


    //if the command is a reset command
    if(message.command == "reset")
    {
        debug("Msgd::parse()::reset");
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

    sem_wait(&user_lock);

    for(int i = 0; i < users.size(); i++)
    {
        if(users.at(i).name == message.param[0])
        {
            userFound = true;
            users.at(i).subject.push_back(message.param[1]);
            users.at(i).message.push_back(message.message);
        }
    }

    if(!userFound)
    {
        User newUser(message.param[0]);
        newUser.subject.push_back(message.param[1]);
        newUser.message.push_back(message.message);
        users.push_back(newUser);
    }

    sem_post(&user_lock);

    return "OK\n";
}



string
Msgd::handList(Message message)
{
    debug("Msgd::handList()");

    bool userFound = false;

    stringstream resp;

    sem_wait(&user_lock);

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

    sem_post(&user_lock);

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

    sem_wait(&user_lock);

    debug("Msgd::handGet()::primaryFOR");
    for(int i = 0; i < users.size(); i++)
    {
        if(users.at(i).name == message.param[0])
        {
            debug("Msgd::handGet()::User Found");
            userFound = true;

            if(index > users.at(i).subject.size())
            {
                stringstream ss;
                ss << users.at(i).subject.size();
                debug("Msgd::handGet()::subjectIndexOutOfBounds:subject.size:" + ss.str());

                sem_post(&user_lock);
                return "error index out of bounds\n";
            }
            else
            resp << users.at(i).read(index);
        }
    }

    if(!userFound)
    {
        debug("Msgd::handGet()::userNotFound");
        //send an error
        sem_post(&user_lock);
        return "error user not found\n";
    }



    sem_post(&user_lock);
    return resp.str();
}



string
Msgd::get_request(int client) 
{
    debug("Msgd::get_request()");
    char* buf_ = new char[buflen_+1];

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
            {
                // an error occurred, so break out
                delete buf_;
                return "";
            }
                
        } 
        else if (nread == 0) 
        {
            // the socket is closed
            delete buf_;
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
                // string dummy;
                // getline(iss, dummy, '\n');

                // postline.append(dummy + '\n');
                string dummy;
                getline(iss, dummy);

                if(iss.eof())
                    postline.append(dummy);
                else
                    postline.append(dummy + '\n');

            }
                

            debug("Msgd::get_request()::postline:BEGIN|" + postline + "|END");

            //newline found in the last grab from the recv function
            newlineFound = true;
            request = request.append(preline);
            sem_wait(&cache_lock);
            caches[client] = postline;
            sem_post(&cache_lock);
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
    delete buf_;
    return request;
}

bool
Msgd::send_response(int client, string response) 
{
    debug("Msgd::send_response()");
    debug("Msgd::send_response()::response:" + response);

    // prepare to send response
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) 
    {
        if ((nwritten = send(client, ptr, nleft, 0)) < 0) 
        {
            if (errno == EINTR)
            {
                // the socket call was interrupted -- try again
                continue;
            } 
            else 
            {
                // an error occurred, so break out
                perror("write");
                return false;
            }
        } 
        else if (nwritten == 0) 
        {
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
