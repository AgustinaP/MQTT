#include "MQTT.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define portno 1884
#define BUF_SIZE 1024

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUF_SIZE];
    if (argc < 2) {
       fprintf(stderr,"usage %s hostname \n", argv[0]);
       exit(0);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }


    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    CONNECT_msg m(1,"USER1");
    send_msg(&m,sockfd);
    std::cout<<"Connected to the broker"<<std::endl;
    m.print_buffer();
    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, 1023);
    int e ;
    CONNACK_msg cm((uint8_t *)buffer,e);
    std::cout<<"CONNACK_msg"<<std::endl;
    cm.print_buffer();

    bzero(buffer, BUFFER_SIZE);
    PINGREQ_msg m1;
    send_msg(&m1,sockfd);
    std::cout<<"PINGREQ_msg"<<std::endl;
    m1.print_buffer();

    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, 1023);
    PINGRESP_msg m2((uint8_t *)buffer,e);
    std::cout<<"PINGRESP_msg"<<std::endl;
    m2.print_buffer();

    bzero(buffer, BUFFER_SIZE);
    std::list<std::string> topic;
    topic.push_back("TOPIC1");
    topic.push_back("TOPIC2");
    std::list<uint8_t> qos;
    qos.push_back(0);
    qos.push_back(0);
    SUBSCRIBE_msg m3(7,topic,qos);
    send_msg(&m3,sockfd);
    std::cout<<"SUBSCRIBE_msg"<<std::endl;
    m3.print_buffer();

    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, 1023);
    SUBACK_msg m4((uint8_t *)buffer,e);
    std::cout<<"SUBACK_msg"<<std::endl;
    m4.print_buffer();


    bzero(buffer, BUFFER_SIZE);
    std::list<std::string> topic2;
    topic2.push_back("TOPIC2");
    topic2.push_back("TOPIC3");
    UNSUBSCRIBE_msg m5(1,topic2);
    send_msg(&m5,sockfd);
    std::cout<<"UNSUBSCRIBE_msg"<<std::endl;
    m5.print_buffer();

    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, 1023);
    UNSUBACK_msg m6((uint8_t *)buffer,e);
    std::cout<<"UNSUBACK_msg"<<std::endl;
    m6.print_buffer();

    bzero(buffer, BUFFER_SIZE);
    std::string topic3="TOPIC1";
    std::string payload="Hello World!";    
    uint8_t flags_pub=0x01;
    PUBLISH_msg m7(topic3,flags_pub,payload,9);
    //std::cout<<"hasta aca llegue\n";
    send_msg(&m7,sockfd);
    //std::cout<<"hasta aca llegue\n";
    std::cout<<"PUBLISH_msg"<<std::endl;
    m7.print_buffer();

        


    bzero(buffer, BUFFER_SIZE);
    DISCONNECT_msg md;
    send_msg(&md,sockfd);
    std::cout<<"DISCONNECT_msg"<<std::endl;
    md.print_buffer();
    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, 1023);
       
    close(sockfd);
}
 // g++ Client1.cpp  MQTT.cpp -o client -lpthread