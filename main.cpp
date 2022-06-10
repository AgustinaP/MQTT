#include "Broker.h"

int main(){
    Broker* br = new Broker();
    std::thread consume_thr = std::thread(&consume_routine, br);
    accept_routine(br);

    consume_thr.join();
    pthread_exit(NULL);
    return 0;
}


//g++ main.cpp Broker.cpp  MQTT.cpp -o main -lpthread

