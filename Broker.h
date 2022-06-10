#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <algorithm>
#include <errno.h>

#include "MQTT.h"

#define SERVER_BACKLOG 5
#define BUFFER_SIZE 1024
#define MAX_NUM_CLIENTS 5
#define QUEUE_SIZE 10

struct Client_t{
    int clientSocket; 
    std::list<std::string> topics;
};

class Clients_map_t{
    private:
        std::map<int,Client_t*> clients; 
        std::mutex mutex;
    public:
        void add_client(Client_t *c);
        void remove_client(int client_socket);
        std::list<std::string> get_topics_from_a_client(int client);
        void add_topic_to_a_client(int client_socket,std::string new_topic);
        uint8_t remove_topic_of_a_client(int client_socket,std::string topic_to_remove);
        int size_c();
};

class Topics_map_t{
    private:
        std::map<std::string,std::list<int>> topics; 
        std::mutex mutex;
    public:
        bool add_topic(std::string newtopic);
        bool get_clients_from_topic(std::string topic,std::list<int>& return_list);
        void remove_topic_if_empty(std::string topic_);
        bool add_client_to_a_topic(int new_client_socket,std::string topic);
        void remove_client_of_a_topic(std::string topic,int client_socket_to_remove);
        void print_topics();
};

class Retain_msg_map_t{
    private:
        std::map<std::string,std::string> retained_msgs;
        std::mutex mutex;
    public:
        void add_retained_msg(std::string topic,std::string new_retain_msg);
        void remove_retained_msg(std::string topic);
        int get_reteined_msg_from_a_topic(std::string topic,std::string& retain);
};

//Generic queue with controlled access
template <typename T> class queue_t {
    private:
        std::queue<T> queue;
        std::mutex q_mutex;
        std::condition_variable q_condv;
    public:

    queue_t() = default;
    ~queue_t() = default;

    void push(T task);
    T pop();
};

class Broker{
    private:
        //////  Conexión TCP  //////
        int serverSocket;
        struct sockaddr_in server_addr;

        //////  Base de datos  //////

        queue_t<PUBLISH_msg> publish_queue;
        Clients_map_t clients_map;
        Topics_map_t topics_map;
        Retain_msg_map_t retain_msgs_map;
        int active_client;

    public:
        Broker();
        ~Broker();

        int get_serverSocket()const;
        int get_number_of_clients();
        // add clients to the clients_map
        void add_client(Client_t* c);
        // remove clients from the clients_map
        void remove_client(Client_t* c);
        // add topics to the topics_map and add topics to clients
        void add_topics(Client_t* c, std::list<std::string>& topics);
        // remove topics from the topics_map and remove topics from clients
        std::list<uint8_t> remove_topics(Client_t* c, std::list<std::string>& topics);
        
        void add_publish_msg(PUBLISH_msg& msg);
        PUBLISH_msg get_publish();
        bool get_clients_from_topic(std::string,std::list<int>&);
        void print_topics();
        void send_publish_retained_msg(Client_t* c,std::list<std::string>& topics);

};

void accept_routine(Broker* br);
void handle_client_routine(Broker* br,Client_t* c);
void consume_routine(Broker* br);