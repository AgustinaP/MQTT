#pragma once
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <list>
#include <thread>
#include <netdb.h>
#include <errno.h>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#include "MQTT.h"

#define BUFFER_SIZE 1024


void error(const char *msg)
{
    perror(msg);
    exit(0);
}

class Client{
    private:

        struct sockaddr_in serv_addr;
        struct hostent *server;
        char buffer[BUFFER_SIZE];
        std::string user_name;
        
        int e;


    public:

        bool isConected;
        int socketfd; 
        std::list<std::string> topics;
        std::mutex coutMutex;

        Client();
        ~Client(){close(socketfd);};
        
        void connect_broker(uint16_t keep_alive_,std::string user_name);
        void disconnect_broker();
        void ping_broker();
        void subscribe_broker(uint16_t,std::list<std::string> ,std::list<uint8_t>);
        void unsubscribe_broker(uint16_t,std::list<std::string>);
        void publish_broker(std::string topic_,uint8_t flags_,std::string payload_,uint16_t packet_ID_);
        
};

Client::Client(){
    isConected = false;
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL) 
        error("ERROR, no such host\n");
    
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(PORT_NUMBER);
    if (connect(socketfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
}

void Client::connect_broker(uint16_t keep_alive_,std::string user_name_){
    this->user_name = user_name_;
    CONNECT_msg connect_m(keep_alive_,user_name_);
    send_msg(&connect_m,this->socketfd);
    bzero(buffer, BUFFER_SIZE);
    int n = read(socketfd, buffer, BUFFER_SIZE);
    CONNACK_msg connack_m((uint8_t *)buffer,e);
    if(connack_m.isConnected()==0){
        isConected = true;
    }
    else{
        std::cout<<"ERROR: "<<connack_m.isConnected()<<std::endl;
    }
}

void Client::disconnect_broker(){
    DISCONNECT_msg disconnect_m;
    send_msg(&disconnect_m,socketfd);
}

void Client::ping_broker(){
    PINGREQ_msg ping_m;
    send_msg(&ping_m,socketfd);
}

void Client::subscribe_broker(uint16_t packet_ID_,std::list<std::string> topics_ ,std::list<uint8_t> QoS_){
    SUBSCRIBE_msg subscribe_m(packet_ID_,topics_,QoS_);
    send_msg(&subscribe_m,socketfd);
    std::list<std::string>::iterator it;
    for(it=topics_.begin();it!=topics_.end();it++){
        this->topics.push_back(*it);
    }
}

void Client::unsubscribe_broker(uint16_t packet_ID_,std::list<std::string> topics_){
    UNSUBSCRIBE_msg unsubscribe_m(packet_ID_,topics_);
    send_msg(&unsubscribe_m,socketfd);
    std::list<std::string>::iterator it;
    for(it=topics_.begin();it!=topics_.end();it++){
        auto search = std::find(topics.begin(), topics.end(),*it);
        if(search!=topics.end()){
            topics.erase(search);
        }
        else{
            std::cout<<"ERROR: Topic not found"<<std::endl;
        }
    }     
}

void Client::publish_broker(std::string topic_,uint8_t flags_,std::string payload_,uint16_t packet_ID_){
    // std::string topic_, uint8_t flags_, std::string payload_,uint16_t packet_ID_
    PUBLISH_msg publish_m(topic_,flags_,payload_,packet_ID_);
    send_msg(&publish_m,socketfd);
}

void receive_routine(Client* client);

void send_routine(Client* client){
    int option = 0;
    std::cout<<"Enter keep alive: "<<std::endl;
    uint16_t keep_alive;
    std::cin>>keep_alive;
    std::cout<<"Enter user name: "<<std::endl;
    std::string user_name;
    std::cin>>user_name;
    client->connect_broker(keep_alive,user_name);
    std::cout<<"Connected to broker"<<std::endl;
    std::thread receive_t(&receive_routine,client);
    while(client->isConected){

        do{
        client->coutMutex.lock();
        std::cout<<"Choose an option: "<<std::endl;
        std::cout<<"1. Disconnect"<<std::endl;
        std::cout<<"2. Ping broker"<<std::endl;
        std::cout<<"3. Subscribe"<<std::endl;
        std::cout<<"4. Unsubscribe"<<std::endl;
        std::cout<<"5. Publish"<<std::endl;
        std::cout<<"6. See subscriptions"<<std::endl;
        client->coutMutex.unlock();
        std::cin>>option;
        }while(option>=7 || option<=0);

        switch(option){
            case 1:{
                client->disconnect_broker();
                client->isConected = false;
                break;
            }
            case 2:{
                client->ping_broker();
                break;
            }
            case 3:{
                std::list<std::string> topics;
                std::list<uint8_t> QoS;
                client->coutMutex.lock();
                std::cout<<"Enter number of topics: "<<std::endl;
                client->coutMutex.unlock();
                int n;
                std::cin>>n;
                for(int i=0;i<n;i++){
                    client->coutMutex.lock();
                    std::cout<<"Enter topic: "<<std::endl;
                    client->coutMutex.unlock();
                    std::string topic;
                    std::cin>>topic;
                    topics.push_back(topic);
                    client->coutMutex.lock();
                    std::cout<<"Enter QoS: "<<std::endl;
                    client->coutMutex.unlock();
                    uint8_t QoS_;
                    std::cin>>QoS_;
                    QoS.push_back(QoS_);
                }
                client->subscribe_broker(0,topics,QoS);
                break;
            }
            case 4:{
                std::list<std::string> topics;
                client->coutMutex.lock();
                std::cout<<"Enter number of topics: "<<std::endl;
                client->coutMutex.unlock();
                int n;
                std::cin>>n;
                for(int i=0;i<n;i++){
                    client->coutMutex.lock();
                    std::cout<<"Enter topic: "<<std::endl;
                    client->coutMutex.unlock();
                    std::string topic;
                    std::cin>>topic;
                    topics.push_back(topic);
                }
                client->unsubscribe_broker(0,topics);
                break;
            }
            case 5:{
                std::string topic;
                client->coutMutex.lock();
                std::cout<<"Enter topic: "<<std::endl;
                client->coutMutex.unlock();
                std::cin>>topic;
                uint8_t flags;
                client->coutMutex.lock();
                std::cout<<"Enter flags (1: retain): "<<std::endl;
                client->coutMutex.unlock();
                std::cin>>flags;
                std::string payload;   
                client->coutMutex.lock();
                std::cout<<"Enter payload: "<<std::endl;
                client->coutMutex.unlock();
                std::cin>>payload;
                client->publish_broker(topic,flags,payload,0);
                break;
            }
            case 6:{
                std::cout<<"Subscriptions: "<<std::endl;
                std::list<std::string>::iterator it;
                for(it=client->topics.begin();it!=client->topics.end();it++){
                    std::cout<<*it<<std::endl;
                }
                break;
            }
        }
    }
    receive_t.join();
    return;
}

void receive_routine(Client* client){
    client->coutMutex.lock();
    std::cout<<"Running receive_routine"<<std::endl;
    client->coutMutex.unlock();
    char buffer[BUFFER_SIZE];
    while(client->isConected){
        client->coutMutex.lock();
        std::cout<<"Waiting for messages"<<std::endl;
        client->coutMutex.unlock();
        int i = receive_msg(client->socketfd,(uint8_t *)buffer);
        if(i){
            client->coutMutex.lock();
            std::cout<<"Message received"<<std::endl;
            std::cout<<"Packet type: "<<(Type) ((*buffer & TYPE_MASK)>> 4)<<std::endl;
            client->coutMutex.unlock();
        switch((Type) ((*buffer & TYPE_MASK)>> 4)){
            case PINGRESP: {
                client->coutMutex.lock();
                std::cout<<"PINGRESP"<<std::endl;
                client->coutMutex.unlock();
                break;
            }
            case PUBACK: {
                client->coutMutex.lock();
                std::cout<<"PUBACK"<<std::endl;
                client->coutMutex.unlock();
                break;
            }
            case SUBACK: {
                client->coutMutex.lock();
                std::cout<<"SUBACK"<<std::endl;
                client->coutMutex.unlock();
                break;
            }
            case UNSUBACK: {
                client->coutMutex.lock();
                std::cout<<"UNSUBACK"<<std::endl;
                client->coutMutex.unlock();
                break;
            }  
            case PUBLISH: {
                int e;
                client->coutMutex.lock();
                std::cout<<"PUBLISH"<<std::endl;
                client->coutMutex.unlock();
                PUBLISH_msg publish_m((uint8_t *)buffer,e);
                client->coutMutex.lock();
                std::cout<<"Topic received: "<<publish_m.get_topic()<<std::endl;
                std::cout<<"Payload received: "<<publish_m.get_payload()<<std::endl;
                client->coutMutex.lock();
                break;
            }
            case 0: {
                client->coutMutex.lock();
                std::cout<<"Bye Bye."<<std::endl;
                client->coutMutex.unlock();
                client->isConected = false;
                break;
            }
            default: {
                client->coutMutex.lock();
                std::cout<<"ERROR. This message is wrong ("<< (int) (buffer[0]>> 4)<<")"<<std::endl;
                client->coutMutex.unlock();
                client->isConected = false;
                break;
            }
        }
        }

    }
}