#include "Broker.h"
#include <strings.h>


/////////////////////////////////////////
////// class Clients_map_t methods //////
/////////////////////////////////////////


void Clients_map_t::add_client(Client_t *c){
    std::lock_guard<std::mutex> lock_map(mutex);    
    clients.emplace( std::make_pair(c->clientSocket,c));
}

void Clients_map_t::remove_client(int client_socket){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = clients.find(client_socket);
    if (search != clients.end()){
        clients.erase(search);
        std::cout<<"Client borrowed from client_map \n";
    }
    else 
        std::cout << "Client not found\n";
}

std::list<std::string> Clients_map_t::get_topics_from_a_client(int client){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = clients.find(client);
    if (search != clients.end())
        return search->second->topics;
    else {
        std::cout << "Client not found\n";
        std::list<std::string> a;
        return a;
    }
}

void Clients_map_t::add_topic_to_a_client(int client_socket,std::string new_topic){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = clients.find(client_socket);
    if (search != clients.end()){
        search->second->topics.push_back(new_topic);
    }
    else 
        std::cout << "Client not found\n";
}

uint8_t Clients_map_t::remove_topic_of_a_client(int client_socket,std::string topic_to_remove){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = clients.find(client_socket);
    if (search != clients.end()){
        std::list<std::string>::iterator it;
        it = std::find(search->second->topics.begin(), search->second->topics.end(),topic_to_remove);
        if(it != search->second->topics.end())
        {
            search->second->topics.erase(it);
            std::cout<<"Topic "<<topic_to_remove<< " eliminated from "<<client_socket<<std::endl;
            return 0;
        }
        else{ 
            std::cout<<client_socket<<" is not subscripted to "<<topic_to_remove<<std::endl;
            return 17;
        }
    }
    else 
        std::cout << "Client not found\n";
        return -1;
}

int Clients_map_t::size_c(){
    return clients.size();
}


////////////////////////////////////////
////// Class Topics_map_t methods //////
////////////////////////////////////////


bool Topics_map_t::add_topic(std::string newtopic){
    std::lock_guard<std::mutex> lock_map(mutex); 
    std::list<int> lista;   
    std::cout<<"Topic "<<newtopic<<" added to the topics_map\n";
    return topics.emplace(std::make_pair(newtopic,lista)).second;
    //retorna  1 si se inserto, 0 si no.
}

bool Topics_map_t::get_clients_from_topic(std::string topic, std::list<int>& return_value){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = topics.find(topic);
    if (search != topics.end()){
        return_value=search->second;
        return true;
    }
    else {
        std::cout << "Topic not found\n";
        return false;
    }
}

void Topics_map_t::remove_topic_if_empty(std::string topic){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = topics.find(topic);
    if ((search != topics.end()) && (search->second.empty())){
        topics.erase(search);
        std::cout<<"Topic "<<topic<<" borrowed because it is empty\n";
    }
    else 
        std::cout << "Topic have almost one subscription.\n";
}

bool Topics_map_t::add_client_to_a_topic(int new_client_socket,std::string topic){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = topics.find(topic);
    if (search != topics.end()){
        search->second.push_back(new_client_socket);
        std::cout<<new_client_socket<<" subscripted to topic "<<topic<<std::endl;
        return true;
    }
    else{
        std::cout <<new_client_socket<<": Topic not found in topics_map\n";
        return false;
    }
}

void Topics_map_t::remove_client_of_a_topic(std::string topic,int client_socket_to_remove){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = topics.find(topic);
    if (search != topics.end()){
        std::list<int>::iterator it;
        it = std::find(search->second.begin(), search->second.end(),client_socket_to_remove);
        if(it != search->second.end())
        {
            search->second.erase(it);
            std::cout<<"Client "<<client_socket_to_remove<< " eliminated from "<<topic<<" of the topics_map"<<std::endl;
        }
        else 
            std::cout<<"Client not subscribed to the topic "<<topic<<"\n";
    }
    else 
        std::cout << "Topic not found\n";
}

void Topics_map_t::print_topics(){
    std::lock_guard<std::mutex> lock_map(mutex); 
    for (auto it = topics.begin(); it != topics.end(); ++it){
        std::cout << it->first << ": ";
        for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2){
            std::cout << *it2 << " ";
        }
        std::cout << std::endl;
    }
}


////////////////////////////////////////////
////// Class Retain_msg_map_t methods //////
////////////////////////////////////////////


void Retain_msg_map_t::add_retained_msg(std::string topic,std::string new_retain_msg){
    std::lock_guard<std::mutex> lock_map(mutex);
        
    auto search = retained_msgs.find(topic);
    if (search != retained_msgs.end())
        search->second=new_retain_msg;
    else
        retained_msgs.emplace(std::make_pair(topic,new_retain_msg));
    //if the topic doesn't exist, it will be created and it will take the value of new_retain_msg. 
    //if the topic exist, the retain_msg will take the value of new_retain_msg.
    std::cout << "Topic "<<topic<<" now has the retained message: "<< new_retain_msg<<std::endl; 
}

void Retain_msg_map_t::remove_retained_msg(std::string topic){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = retained_msgs.find(topic);
    if (search != retained_msgs.end()){
        retained_msgs.erase(search);
        std::cout<<"Retained message borrowed\n";
    }
    else 
        std::cout << "Retained message not found\n";
}

int Retain_msg_map_t::get_reteined_msg_from_a_topic(std::string topic, std::string& retain_){
    std::lock_guard<std::mutex> lock_map(mutex); 
    auto search = retained_msgs.find(topic);
    if (search != retained_msgs.end()){
        retain_ = search->second;
        return 1;
    }
    else{ 
        return 0;
    }
}


////////////////////////////////////////
////// Controlled queue_t methods //////
////////////////////////////////////////
 

template <typename T> void queue_t<T>::push(T item) {
    std::unique_lock<std::mutex> lock_queue(q_mutex);
    while (queue.size() >= QUEUE_SIZE) {
        q_condv.wait(lock_queue);
    }

    queue.push(item);
    lock_queue.unlock();

    if(queue.size() == 1){
        //Hay un elemento, desbloqueo acceso.
        q_condv.notify_one();
    }
    return;
}

template <typename T> T queue_t<T>::pop() {
    std::unique_lock<std::mutex> lock_queue(q_mutex);
    while (queue.empty()){
        //Espero hasta que aparezca al menos un elemento
        q_condv.wait(lock_queue);
    }
    T val = queue.front();
    queue.pop();
    lock_queue.unlock();
    
    if(queue.size() == QUEUE_SIZE-1){
        //Si al sacar este elemento liberé un lugar, avisa
        q_condv.notify_one();
    }
    return val;
}


////////////////////////////
////// Broker methods //////
////////////////////////////


Broker::Broker(){
    active_client = 0;
    ////// Create a socket  //////
    // socket(int domain, int type, int protocol)
    serverSocket =  socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) 
        perror("ERROR opening socket");

    //////  Clear and set variables  //////
    bzero((char *) &server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;  
    server_addr.sin_addr.s_addr = INADDR_ANY;  
    server_addr.sin_port = htons(PORT_NUMBER);

    ////// Binding  //////
    if (bind(serverSocket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
        {perror("ERROR on binding");
        exit(1);
        }
    
    ////// Listening  //////
     // Here, we set the maximum size for the backlog queue to SERVER_BACKLOG.
    if( listen(serverSocket,SERVER_BACKLOG)<0)
            perror("ERROR on listening");
    else
        std::cout<<"Broker is listening\n";
}

Broker::~Broker(){
    close(serverSocket);
}

int Broker::get_serverSocket()const{
     return serverSocket;
}

int Broker::get_number_of_clients(){
    return active_client;
}

void Broker::add_client(Client_t* c){
    clients_map.add_client(c);
    active_client++;
    std::cout<<"Client "<<c->clientSocket<<" was added to the clients_map\n";
}

void Broker::remove_client(Client_t* c){
    std::list<std::string> topicos = clients_map.get_topics_from_a_client(c->clientSocket);
    this->remove_topics(c,topicos);
    clients_map.remove_client(c->clientSocket);
    close(c->clientSocket);
    delete c;
    active_client--;
    std::cout<<"Client eliminated \n";
}

void Broker::add_topics(Client_t* c,std::list<std::string>& topics){
    std::list<std::string>::iterator list_it;

    for(list_it = topics.begin();list_it != topics.end();list_it++){
        std::cout<<c->clientSocket<<" is subscribing to the topic "<<*list_it<<std::endl;
        clients_map.add_topic_to_a_client(c->clientSocket,*list_it);
        if(!topics_map.add_client_to_a_topic(c->clientSocket,*list_it)){
            topics_map.add_topic(*list_it);
            topics_map.add_client_to_a_topic(c->clientSocket,*list_it);
        }
    }
}

void Broker::send_publish_retained_msg(Client_t* c,std::list<std::string>& topics){
    std::list<std::string>::iterator list_it;
    for(list_it = topics.begin();list_it != topics.end();list_it++){
        std::string retain_msg;
        if(retain_msgs_map.get_reteined_msg_from_a_topic(*list_it,retain_msg)){
            std::cout<<"Retained message found for topic "<<*list_it<<std::endl;
            PUBLISH_msg msg(*list_it,1,retain_msg,2);
            send_msg(&msg,c->clientSocket);
        }
    }
}


PUBLISH_msg Broker::get_publish(){
    return publish_queue.pop();
}

void Broker::add_publish_msg(PUBLISH_msg& msg){
    publish_queue.push(msg);
    if(msg.is_reteined()){
        std::string topic = msg.get_topic();
        std::string retain_msg = msg.get_payload();
        retain_msgs_map.add_retained_msg( topic , retain_msg);
    }
}

bool Broker::get_clients_from_topic (std::string topic_,std::list<int>& return_list)
{
    return  topics_map.get_clients_from_topic( topic_, return_list );

}

void Broker::print_topics(){
    topics_map.print_topics();
}

std::list<uint8_t>  Broker::remove_topics(Client_t* c, std::list<std::string>& topics){
    std::list<uint8_t> return_codes;
    std::list<std::string>::iterator list_it;
    for(list_it = topics.begin();list_it != topics.end();list_it++){
        std::cout<<c->clientSocket<<" is unsubscribing from the topic "<<*list_it<<std::endl;
        int ret_=clients_map.remove_topic_of_a_client(c->clientSocket,*list_it);
        if(ret_ == 0){
            topics_map.remove_client_of_a_topic(*list_it,c->clientSocket);
            topics_map.remove_topic_if_empty(*list_it);
        }
        return_codes.push_back(ret_);
        
    }

    return return_codes;
}


///////////////////////////////
////// Routine functions //////
///////////////////////////////


void accept_routine(Broker* br){
    std::cout<<"Running accept_routine\n";
    while(true){
        sockaddr_in client_addr;
        socklen_t clientLen = sizeof(client_addr);

        int clientSocket = accept(br->get_serverSocket(), (struct sockaddr *) &client_addr, &clientLen);
        if(clientSocket<0){
            perror("Error on accept the client");
            close(clientSocket);
        }
        if(br->get_number_of_clients() < MAX_NUM_CLIENTS){
            std::cout<<"New client: "<<clientSocket<<std::endl;
            Client_t* c = new Client_t; //new client
            c->clientSocket = clientSocket;
            br->add_client(c);
            std::thread handle(&handle_client_routine,br,c);
            handle.detach();
        }
        else{
            std::cout<<"The server is already attending the maximum number of clients\n";
            close(clientSocket);
        }
    }
    return;
}

void handle_client_routine(Broker* br,Client_t* c){
    uint8_t buffer[1024];
    int e;
    bool socketIsOpen = true; 

    if(!receive_msg(c->clientSocket,buffer))
        perror("ERROR receiving CONNECT msg");

    if((buffer[0]>> 4) != 1){
        std::cout<< "ERROR. First message must be CONNECT\n";
        br->remove_client(c);
        return;
    }

    std::cout<< c->clientSocket<<": CONNECT\n";
    CONNECT_msg m(buffer, e);

    if(e == -1){
        std::cout<<"ERROR on CONNECT message\n";
        br-> remove_client(c);
        return;
    }
    
    std::cout<< c->clientSocket<< ": CONNACK\n";
    CONNACK_msg m_ack(0x01,e); // Only New Sessions has been implemented
    //m_ack.print_buffer();
    socketIsOpen = send_msg(&m_ack, c->clientSocket);    
    if(!socketIsOpen)
        perror("ERROR sending CONNACK messsage\n");

    while(socketIsOpen){
        socketIsOpen = receive_msg(c->clientSocket,buffer);
        if(!socketIsOpen)
            perror("ERROR receiving message");
        switch(buffer[0]>> 4){
            case CONNECT:{
                std::cout<<"ERROR. This client is already connected\n";
                socketIsOpen = false;
                break;}
            
            case SUBSCRIBE:{
                std::cout<< c->clientSocket<< ": SUSCRIBE\n";
                SUBSCRIBE_msg sus_m (buffer, e);
                std::list<std::string>& topics = sus_m.get_topics();
                br->add_topics(c,topics);
            
                std::cout<< c->clientSocket<< ": SUBACK\n";
                std::list<uint8_t> return_codes(topics.size(),0);
                SUBACK_msg suba_m(sus_m.get_packet_ID(),return_codes);
                socketIsOpen = send_msg(&suba_m, c->clientSocket);
                br->send_publish_retained_msg(c,topics);
                break;}
            
            case UNSUBSCRIBE:{
                std::cout<< c->clientSocket<< ": UNSUBSCRIBE\n";
                UNSUBSCRIBE_msg usus_m(buffer, e);
                std::list<std::string>& topics = usus_m.get_topic_info();
                std::list<uint8_t> return_codes = br->remove_topics(c,topics); 

                std::cout<< c->clientSocket<< ": UNSUBACK\n";
                  
                UNSUBACK_msg unsuback_m(usus_m.get_packet_ID(),return_codes);
                socketIsOpen = send_msg(&unsuback_m, c->clientSocket);
                break;
            }
            case PUBLISH:{
                std::cout<<"PUBLISH\n";
                PUBLISH_msg pub_m(buffer, e);
                br->add_publish_msg(pub_m); 
                break;
            }
            case DISCONNECT:{
                std::cout<<c->clientSocket<<": DISCONNECT\n";
                socketIsOpen = false;
                break;
            }

            case PINGREQ:{
                std::cout<<c->clientSocket<<": PINGREQ\n";

                std::cout<<c->clientSocket<<": PINGRESP\n";
                PINGRESP_msg pingresp_m; 
                send_msg(&pingresp_m, c->clientSocket);
                break;
            }
            default:
                std::cout<<c->clientSocket<<": Invalid message (" << (int)buffer[0]<<")"<<std::endl;
                socketIsOpen = false;
                break;
        }
    
    }
    br->remove_client(c);
    
    return;
}

void consume_routine(Broker* br){
    std::cout<<"Running consume_routine\n";
    std::list<int>::iterator list_it;
    std::list<int> subscriptors;
    while(true){
        PUBLISH_msg pub = br->get_publish();
        std::string topic = pub.get_topic();
        if(br->get_clients_from_topic(topic, subscriptors)){
            for(list_it = subscriptors.begin(); list_it != subscriptors.end(); list_it++)
            send_msg(&pub,*list_it);  
        }
    }
} 

