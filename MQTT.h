#pragma once
#include <cstdint>
#include <string>
#include <string.h>
#include <iostream>
#include <list>

#define PORT_NUMBER 1884
#define BUFFER_SIZE 1024
#define TYPE_MASK 0xF0
#define FLAGS_MASK 0x0F

typedef enum {
    Reserved,   // 0
    CONNECT,    // 1-
    CONNACK,    // 2-
    PUBLISH,    // 3-
    PUBACK,     // 4-
    PUBREC,     // 5 QoS = 1
    PUBREL,     // 6 QoS = 2
    PUBCOMP,    // 7 QoS = 3
    SUBSCRIBE,  // 8-
    SUBACK,     // 9
    UNSUBSCRIBE,// 10
    UNSUBACK,   // 11
    PINGREQ,    // 12
    PINGRESP,   // 13
    DISCONNECT, // 14
    Reserved2,  // 15
} Type;

class Message {
    
    protected:

        Type type;              //type 4 bits
        uint8_t header_flags;   //flags 4 bits 
        int rem_length;         //1 to 4 bytes

        // set params to a given buffer
        void set_type(uint8_t*)const;
        void set_header_flags(uint8_t*)const;
        int set_remaining_length(uint8_t*)const;

        // get params from a given buffer
        void get_type(uint8_t*);
        void get_header_flags(uint8_t*);
        int get_remaining_length(uint8_t*);


    public:

        Message() = default;
        ~Message() = default;

        //pack the message to a given buffer
        virtual int pack (uint8_t * buffer){return 0;};
        void print_buffer(void);

        //Type get_type() const{return type;}
        //void set_type (Type type_){type = type_;}
        //void set_header_flag(uint8_t headers_flags_){header_flags=headers_flags_&0x0F;}

        //unpack a message from a given buffer 
        //virtual int unpack (uint8_t * buffer){return 0;};
};

bool receive_msg(int socket,  uint8_t* buffer); //false = ERROR
bool send_msg(Message* m, int socket); //false = ERROR
bool check_characters(std::string& name,uint16_t length); //false = ERROR



class CONNECT_msg:public Message{
    private:

        // Variable header
        uint8_t flags;           // 1 byte
        uint16_t keep_alive;     // 2 bytes

        // Payload
        uint16_t ID_length;           // 2 bytes
        std::string client_ID;        // ID_length bytes
        uint16_t will_topic_length;   // 2 bytes
        std::string will_topic;       // WTopic_length bytes
        uint16_t will_msg_length;     // 2 bytes
        std::string will_msg;         // WillMssg_length bytes
        uint16_t user_length;         // 2 bytes 
        std::string username;         // user_length bytes
        uint16_t password_length;     // 2 bytes
        std::string password;         // password_length bytes

        //Get buffer parameters
        int get_header(uint8_t* buffer, int& errors); //return # read bytes 
        int get_header_v(uint8_t* buffer, int& errors); //return # read bytes 
        void get_payload(uint8_t* buffer, int& errors);
        
        //Checks
        void check_flags(int& errors);
        int check_protocol_name (uint8_t* buffer, int& errors) const; //return # read bytes

        //Set parameters into buffer
        void set_protocol_name (uint8_t* buffer);
        int set_header(uint8_t* buffer);  
        int set_header_v(uint8_t* buffer); 
        int set_payload(uint8_t* buffer);

    public:

        CONNECT_msg(uint8_t* buffer, int& errors);
        CONNECT_msg(uint16_t keep_alive_, std::string client_ID_);
        CONNECT_msg(uint8_t flags_,uint16_t keep_alive_, std::string client_ID_,std::string will_topic_,std::string will_msg_);
        ~CONNECT_msg(){};

        int pack (uint8_t * buffer);
        //int unpack (uint8_t * buffer, int& errors);



};

class CONNACK_msg:public Message{
    private:    
        uint8_t flags;
        uint8_t return_code;
        int get_header(uint8_t* buffer, int& errors);
        int get_header_v(uint8_t* buffer, int& errors);

        int set_header(uint8_t* buffer);  
        int set_header_v(uint8_t* buffer); 

    public:
        CONNACK_msg(uint8_t flag_, uint8_t return_code_);
        CONNACK_msg(uint8_t* buffer, int& errors);
        ~CONNACK_msg(){};

        int pack (uint8_t * buffer);
        bool isConnected(){return return_code;}
       
};

class DISCONNECT_msg:public Message{
    private:
        int set_header(uint8_t* buffer);
    public:
    DISCONNECT_msg(uint8_t * buffer,int& errors);
    DISCONNECT_msg();
    ~DISCONNECT_msg(){};
    
    int pack(uint8_t * buffer);
};

class PINGREQ_msg:public Message{
    private:
        int set_header(uint8_t* buffer);
    public:

        PINGREQ_msg(uint8_t * buffer,int& errors);
        PINGREQ_msg();
        ~PINGREQ_msg(){};

        int pack(uint8_t * buffer);
};

class PINGRESP_msg:public Message{
    private:
        int set_header(uint8_t* buffer);
    public:
        PINGRESP_msg(uint8_t * buffer,int& errors);
        PINGRESP_msg();
        ~PINGRESP_msg(){};

        int pack(uint8_t * buffer);
};

class SUBSCRIBE_msg:public Message{
    private:

        uint16_t packet_ID;
        std::list<std::string> topic;
        std::list<uint16_t> topic_length;
        std::list<uint8_t> QoS;

        int get_header(uint8_t* buffer, int& errors);
        int get_topic_info(uint8_t* buffer, int& errors);

        int set_header(uint8_t* buffer);
        int set_header_v(uint8_t* buffer);
        int set_payload(uint8_t* buffer);


    public:
        SUBSCRIBE_msg(uint8_t* buffer, int& errors);
        SUBSCRIBE_msg(uint16_t packet_ID_, std::list<std::string> topic_, std::list<uint8_t> QoS_);
        ~SUBSCRIBE_msg(){};

        int pack(uint8_t * buffer);
        uint16_t get_packet_ID();
        std::list<std::string>& get_topics();


};

class SUBACK_msg:public Message{
    private:

        uint16_t packet_ID;
        std::list<uint8_t> return_code;
        int get_header(uint8_t* buffer, int& errors);
        int set_header(uint8_t* buffer);

    public:

        SUBACK_msg(uint8_t* buffer, int& errors);
        SUBACK_msg(uint16_t packet_ID_, std::list<uint8_t> return_code_);
        ~SUBACK_msg(){};

        int pack(uint8_t * buffer);

};

class UNSUBSCRIBE_msg:public Message{
    private:
        uint16_t packet_ID; 
        std::list<std::string> topic;
        std::list<uint16_t> topic_length;

        int get_header(uint8_t* buffer, int& errors);
        int set_header(uint8_t* buffer);
        int set_payload(uint8_t* buffer);
        int get_payload(uint8_t* buffer, int& errors);


    public:
        UNSUBSCRIBE_msg(uint8_t* buffer, int& errors);
        UNSUBSCRIBE_msg(uint16_t packet_ID_, std::list<std::string> topic_);
        ~UNSUBSCRIBE_msg(){};

        int pack(uint8_t * buffer);
        std::list<std::string>& get_topic_info();  
        uint16_t get_packet_ID(); 
};

class UNSUBACK_msg:public Message{
    private:

        uint16_t packet_ID;
        std::list<uint8_t> return_code;
        int get_header(uint8_t* buffer, int& errors);
        int set_header(uint8_t* buffer);


    public:

        UNSUBACK_msg(uint8_t* buffer, int& errors);
        UNSUBACK_msg(uint16_t packet_ID_, std::list<uint8_t> return_code_);
        ~UNSUBACK_msg(){};

        int pack(uint8_t * buffer);
};



class PUBLISH_msg:public Message{
    private:
        uint16_t packet_ID;

        std::string topic;
        uint16_t topic_length;

        std::string payload;
        uint16_t payload_length;

        int get_header(uint8_t* buffer, int& errors);
        int get_header_v_payload(uint8_t* buffer, int& errors);
      
        int set_header(uint8_t* buffer);
        int set_header_v_payload(uint8_t* buffer);

    public:
        PUBLISH_msg(uint8_t* buffer, int& errors);
        PUBLISH_msg(std::string topic_, uint8_t flags_, std::string payload_,uint16_t packet_ID_);
        ~PUBLISH_msg(){};

        int pack(uint8_t * buffer);
        bool is_reteined();
        uint16_t get_packet_ID();
        std::string get_topic();
        std::string get_payload();
};
