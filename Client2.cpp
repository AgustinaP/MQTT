#include "client.h"

int main(){
    Client* cl = new Client();
    send_routine(cl);
    pthread_exit(NULL);
    return 0;
}

