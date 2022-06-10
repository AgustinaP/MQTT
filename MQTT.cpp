#include "MQTT.h"
#include <unistd.h>
#include <strings.h>
#include <iostream>


/////////////////////////////
////// Message Methods //////
/////////////////////////////


void Message::set_type(uint8_t* buffer)const{
    buffer[0] = ((this->type << 4) & TYPE_MASK) | (buffer[0] & FLAGS_MASK);
    return;
}

void Message::set_header_flags(uint8_t* buffer)const{
    buffer[0] = (buffer[0] & TYPE_MASK) | (this->header_flags & FLAGS_MASK);
    return;
}

int Message::set_remaining_length (uint8_t* buffer)const{
    int i=0;
    uint8_t encodedByte;
    int X = this->rem_length;
    do{
        encodedByte = X%128;
        X = X/128;
        if(X > 0){
            encodedByte = encodedByte | 128;
        }
        buffer[i++] = encodedByte;
    } while (X>0);
    return i;
}

void Message::get_type(uint8_t* buffer){
    this->type = (Type) ((*buffer & TYPE_MASK)>> 4);
}

void Message::get_header_flags(uint8_t* buffer){
    this->header_flags =  (*buffer & FLAGS_MASK); 
}

int Message::get_remaining_length(uint8_t* buffer){
    int multiplier = 1, value = 0, i = 0;
    uint8_t encoded;
    do{
        encoded = buffer[i];
        value += (encoded & 127)*multiplier;
        multiplier *= 128;
        if(multiplier > 128*128*128){
            return -1;
        }
        i++;
    } while ((encoded & 128) != 0);
    
    this->rem_length = value;
    return i; //num of remaining length byte 
}

void Message::print_buffer(void){
    uint8_t buffer[BUFFER_SIZE];
    int largo = this->pack(buffer);
    int i =0;
    for (i; i < largo-1 ;i++)
        std::cout<<(int)buffer[i]<<" ";
    std::cout<<(int)buffer[i]<<std::endl;  
}


////////////////////////////
////// Some functions //////
////////////////////////////


bool receive_msg(int socket, uint8_t* buffer){
    bzero(buffer, BUFFER_SIZE);
    int n = read(socket, buffer, 1023);
    if (n < 0)
        return false; // ERROR
    return true;
}

bool send_msg(Message* m, int socket){
    uint8_t buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    int largo = m->pack(buffer);
    int n = write(socket, buffer, largo);
    if (n < 0)
        return false;
    return true;
}

 bool check_characters(std::string& name, uint16_t length){
    for(uint16_t i;i<length;i++){
        if(name[i]<48 || (name[i]>57 && name[i]<65) || (name[i]>90 && name[i]<97) || name[i]>122){
        std::cout<<"ERROR. Stringa can contain only the characters 0123456789abcdefghilklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"<<std::endl;
        return false;
        }
    }
    return true;
 }


/////////////////////////////////
////// CONNECT_msg methods //////
/////////////////////////////////


CONNECT_msg::CONNECT_msg( uint16_t keep_alive_, std::string client_ID_){
    int j =0;
    //Fixed header
    this->type = CONNECT;
    this->header_flags = 0;
    
    //Variable header
    j+=7;

    this->flags = 0b00000010;
    j++;
    this->keep_alive = keep_alive_;
    j+=2;

    //Payload
    this->ID_length = client_ID_.size();
    j+=2;
    this->client_ID = client_ID_;
    j+=this->ID_length;


    this->rem_length =j;
}

CONNECT_msg::CONNECT_msg(uint8_t flags_, uint16_t keep_alive_, std::string client_ID_,std::string will_topic_,std::string will_msg_){
       int j =0;
    //Fixed header
    this->type = CONNECT;
    this->header_flags = 0;
    
    //Variable header
    j+=7;

    this->flags = flags_;
    if(flags_ != 0b00000110 && flags_ != 0b00100110){
        std::cout<<"ERROR. Check Flags. It have to be 0b00000110 or 0b00100110 to use this contructor\n";
        return;
    }
    j++;
    this->keep_alive = keep_alive_;
    j+=2;

    //Payload
    this->ID_length = client_ID_.size();
    j+=2;
    this->client_ID = client_ID_;
    j+=this->ID_length;

    this->will_topic_length = will_topic_.size();
    j+=2;

    this->will_topic = will_topic_;
    j+=this->will_topic_length;

    this->rem_length =j; 
}

CONNECT_msg::CONNECT_msg(uint8_t* buffer, int& errors){
    int i=0;
    errors = 0;
    // Get fixed header from the buffer
    i += get_header(&buffer[i],errors);
    if(errors== -1){return;}

    // Get variable header from the buffer
    i+= get_header_v(&buffer[i],errors);
    if(errors == -1){return;}

    //Payload
    get_payload(&buffer[i],errors);
    return;
}

int CONNECT_msg::get_header(uint8_t* buffer, int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    if(header_flags != 0){
        std::cout<< "ERROR. CONNECT header flags MUST be set to 0\n";
        errors = -1;
        return k;
    }
    k += this->get_remaining_length(&buffer[k]); 
    return k;
}

int CONNECT_msg::get_header_v(uint8_t* buffer, int& errors){
    int k =0;
    k+= this->check_protocol_name(&buffer[k],errors);
    if(errors == -1){return k;}
    
    this->flags = buffer[k++];
    this->check_flags(errors);
    if(errors == -1){return k;}

    this->keep_alive = ((buffer[k] <<8) & 0xFF00) | ((buffer[k+1]) & 0x00FF );
    k += 2;
    return k;
}

void CONNECT_msg::get_payload(uint8_t* buffer, int& errors){
    int i = 0;
    //Client Identifier

    this->ID_length = (buffer[i]<<8) & 0xFF00 | buffer[i+1] & 0x00FF;
    if(this->ID_length == 0 && this->flags & 0b00000010 == 0 ){
        std::cout<<"ERROR. The Client MUST set CleanSession to 1, If the Client supplies a zero-byte ClientId"<<std::endl;
        errors = -1;
        return;
    }
    i +=2;

    std::string name ((char*)&buffer[i], ID_length);
    if(!check_characters(name,ID_length)){
        errors = 2;
        return;
    }
    this->client_ID = name;
    i+=ID_length;

  
    if(this->flags !=0){
        if((this->flags & 0b00000100) == 1 ){
            //Will topic
            this->will_topic_length = (buffer[i]<<8) & 0xFF00 | buffer[i+1] & 0x00FF;
            i +=2;
            std::string will_topic_ ((char*)&buffer[i], will_topic_length);
            this->will_topic = will_topic_;
            i+=will_topic_length;
            //Will message
            this->will_msg_length = (buffer[i]<<8) & 0xFF00 | buffer[i+1] & 0x00FF;
            i +=2;
            std::string will_msg_ ((char*)&buffer[i], will_topic_length);
            this->will_msg = will_msg_;
            i+=will_msg_length;
        }
        if((this->flags & 0b10000000) == 1 ){
            //Username
            this->user_length = (buffer[i]<<8) & 0xFF00 | buffer[i+1] & 0x00FF;
            i +=2;
            std::string username_ ((char*)&buffer[i], user_length);
            this->username = username_;
            i+=user_length;
        }
        if((this->flags & 0b11000000) == 1 ){
            //Password
            this->password_length = (buffer[i]<<8) & 0xFF00 | buffer[i+1] & 0x00FF;
            i +=2;
            std::string password_ ((char*)&buffer[i], password_length);
            this->password = password_;
            i+=password_length;
        }

    }

    return;    
}

void CONNECT_msg::check_flags(int& errors){
    if(this->flags & 0x01 == 1 ){
        std::cout<<"ERROR. The reserved Flag MUST be set to 0"<<std::endl;
        errors = -1;
        return ;
    }
    if((this->flags & 0x04 == 0) && (((this->flags & 0x18) != 0)||((this->flags & 0x20) != 0 ))){
        std::cout<<"ERROR. If Will Flag is 0, Will QoS and Will Retain Flag MUST be set to 0"<<std::endl;
        errors = -1;
        return ;
    }
    if((this->flags & 0x80) == 0 && (this->flags & 0x60) != 0){
        std::cout<<"ERROR. If Name Flag is 0, Password Flag MUST be set to 0"<<std::endl;
        errors = -1;
        return;
    }
}

int CONNECT_msg::check_protocol_name (uint8_t* buffer, int& errors)const{
    if(buffer[0] == 0 && buffer[1] == 4 && buffer[2] == 0x4D && buffer[3] == 0x51 && buffer[4] == 0x54 && buffer[5] == 0x54  ){
        if(buffer[6] == 4)
           return 7; // all okay 
        else{
            errors = 1;
            return 7;
        }
    }
    std::cout<<"ERROR. Protocol name is wrong"<<std::endl;
    errors = -1;
    return 7;
}

void CONNECT_msg::set_protocol_name (uint8_t* buffer){
    buffer[0] = 0;
    buffer[1] = 4;     // protocol name length 
    buffer[2] = 'M'; 
    buffer[3] = 'Q'; 
    buffer[4] = 'T'; 
    buffer[5] = 'T'; 
    buffer[6] = 4;    // version
}

int CONNECT_msg::set_payload(uint8_t* buffer){
    int i = 0;
    buffer[i++] =(uint8_t)(this->ID_length >>8);
    buffer[i++] = (uint8_t)(this->ID_length);
    memcpy(&buffer[i],this->client_ID.c_str(),this->ID_length);
    i += this->ID_length;
    if((this->flags & 0b00000100) == 1 ){
            //Will topic
            buffer[i++] =(uint8_t)(this->will_topic_length >>8);
            buffer[i++] = (uint8_t)(this->will_topic_length);
            memcpy(&buffer[i],this->will_topic.c_str(),this->will_topic_length);
            i += this->will_topic_length;

            //Will message
            buffer[i++] =(uint8_t)(this->will_msg_length >>8);
            buffer[i++] = (uint8_t)(this->will_msg_length);
            memcpy(&buffer[i],this->will_msg.c_str(),this->will_msg_length);
            i += this->will_msg_length;
    }
    if((this->flags & 0b10000000) == 1 ){
        //Username
        buffer[i++] =(uint8_t)(this->user_length >>8);
        buffer[i++] = (uint8_t)(this->user_length);
        memcpy(&buffer[i],this->username.c_str(),this->user_length);
        i += this->user_length;
    }
    if((this->flags & 0b11000000) == 1 ){
        //Password
        buffer[i++] =(uint8_t)(this->password_length >>8);
        buffer[i++] = (uint8_t)(this->password_length);
        memcpy(&buffer[i],this->password.c_str(),this->password_length);
        i += this->password_length;
    }

    return i;
}

int CONNECT_msg::set_header(uint8_t* buffer){
    int k =0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}

int CONNECT_msg::set_header_v(uint8_t* buffer){
    int k =0;
    this->set_protocol_name(&buffer[k]);
    k+=7;
    buffer[k++] = this->flags;
    buffer[k++] =(uint8_t)(this->keep_alive >>8);
    buffer[k++] = (uint8_t)(this->keep_alive);
    return k;
}

int CONNECT_msg::pack (uint8_t * buffer){
    int i =0;
    //fixed header
    i += this->set_header(&buffer[i]);
    //header variable
    i += this->set_header_v(&buffer[i]);
    //payload
    i += this->set_payload(&buffer[i]);
    return i;
}

/*

int CONNECT_msg::unpack(uint8_t * buffer, int& errors){
    int i=0;
    this-> get_type(&buffer[i]);
    this-> get_header_flags(&buffer[i++]);
    int j = this-> get_remaining_length(&buffer[i]);
    i+=j;
    if(this->check_protocol_name(&buffer[i])!=0){
        std::cout<<"ERROR. protocol name "<<std::endl;
        return -1;
    }
    i+=7;
    this->flags = buffer[i++];
    if(this->flags & 0x01!=0){
      std::cout<<"ERROR. Flag 0 "<<std::endl;
        return -1;
    }

    this->keep_alive = ((buffer[i] <<8) & 0xFF00) | ((buffer[i+1]) & 0x00FF );
    i += 2;

    //Payload
    get_payload(&buffer[i]);
    return 0;
}
*/


/////////////////////////////////
////// CONNACK_msg methods //////
/////////////////////////////////


CONNACK_msg::CONNACK_msg( uint8_t flag_, uint8_t return_code_){
    //Fixed header
    this->type = CONNACK;
    this->header_flags = 0;
    this->rem_length = 2;
    this->flags = flag_;
    this->return_code = return_code_;
}

CONNACK_msg::CONNACK_msg(uint8_t* buffer, int& errors){
    int i=0;
    errors = 0;
    // Get fixed header from the buffer
    i += get_header(&buffer[i],errors);
    if(errors!= 0){return;}

    // Get variable header from the buffer
    i+= get_header_v(&buffer[i],errors);
    if(errors != 0){return;}
    return;
}

int CONNACK_msg::get_header(uint8_t* buffer, int& errors){
    int k = 0;
    get_type(&buffer[k]);
    get_header_flags(&buffer[k++]);
    /*if(header_flags != 0){
        std::cout<< "ERROR. CONNACK header flags MUST be set to 0\n";
        errors = -1;
        return k;
    }*/
    k += get_remaining_length(&buffer[k]); 
    return k;  
}

int CONNACK_msg::get_header_v(uint8_t* buffer, int& errors){
    int k = 0;
    this->flags = buffer[k++];
    this->return_code = buffer[k++];
    if(this->return_code != 0 && this->flags != 0){
        std::cout<< "ERROR. Return code MUST be set in 0 if Session Present flag is set to 0\n";
        errors = -1;
        return k;
    }
    return k;
}

int CONNACK_msg::pack(uint8_t* buffer){
    int i =0;
    //fixed header
    i += this->set_header(&buffer[i]);
    //header variable
    i += this->set_header_v(&buffer[i]);
    return i;
}

int CONNACK_msg::set_header(uint8_t* buffer){
    int k =0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
 }

int CONNACK_msg::set_header_v(uint8_t* buffer){
    int k =0;
    buffer[k++] = this->flags;
    buffer[k++] = this->return_code;
    return k;
}


////////////////////////////////////
////// DISCONNECT_msg methods //////
////////////////////////////////////


DISCONNECT_msg::DISCONNECT_msg(uint8_t* buffer,int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    if(header_flags != 0){
        std::cout<< "ERROR. DISCONNECT header flags MUST be set to 0\n";
        errors = -1;
        return;
    }
    this->get_remaining_length(&buffer[k]); 
    return;
}

DISCONNECT_msg::DISCONNECT_msg(void){
    this->type = DISCONNECT;
    this->header_flags = 0x00;
    this->rem_length = 0;
}


int DISCONNECT_msg::pack(uint8_t * buffer){
    return this->set_header(&buffer[0]);
}

int DISCONNECT_msg::set_header(uint8_t* buffer){
    int k =0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}


/////////////////////////////////
////// PINGREQ_msg methods //////
/////////////////////////////////


PINGREQ_msg::PINGREQ_msg(void){
    this->type = PINGREQ;
    this->header_flags = 0x00;
    this->rem_length = 0;
}

PINGREQ_msg::PINGREQ_msg(uint8_t* buffer, int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    this->get_remaining_length(&buffer[k]); 
    return;
}

int PINGREQ_msg::set_header(uint8_t* buffer){
    int k =0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}

int PINGREQ_msg::pack(uint8_t * buffer){
    return this->set_header(&buffer[0]);
}


//////////////////////////////////
////// PINGRESP_msg methods //////  
//////////////////////////////////


PINGRESP_msg::PINGRESP_msg(void){
    this->type = PINGRESP;
    this->header_flags = 0x00;
    this->rem_length = 0;
}

PINGRESP_msg::PINGRESP_msg(uint8_t* buffer, int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    this->get_remaining_length(&buffer[k]); 
    return;
}

int PINGRESP_msg::set_header(uint8_t* buffer){
    int k =0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}

int PINGRESP_msg::pack(uint8_t * buffer){
    return this->set_header(&buffer[0]);
}


///////////////////////////////////
////// SUBSCRIBE_msg methods //////
///////////////////////////////////

SUBSCRIBE_msg::SUBSCRIBE_msg(uint16_t packet_ID_, std::list<std::string> topic_, std::list<uint8_t> QoS_){
    // Fixed header
    this->type = SUBSCRIBE;
    this->header_flags = 2;
    int j = 0;
    // Variable header
    this->packet_ID = packet_ID_;
    j += 2;
    
    this->QoS.assign(QoS_.begin(), QoS_.end());


    //Payload
    for (std::list<std::string>::iterator it = topic_.begin(); it != topic_.end(); it++){
        this->topic.push_back(*it);
        this->topic_length.push_back((uint16_t)(*it).size());
        j += (*it).size()+3; // 2 bytes for topic length, 1 byte for QoS
    }   
    this->rem_length = j;
}

SUBSCRIBE_msg::SUBSCRIBE_msg(uint8_t* buffer, int& errors){
    int k = 0; 
    // Fixed header
    k+=this->get_header(&buffer[k],errors);

    // Variable header

    int j = 0; // to store the length of the payload + variable header
    this->packet_ID = (buffer[k+j]<<8 & 0xF0) | (buffer[k+j+1] & 0x0F);
    j += 2;

    // Payload
   
    while(j < this->rem_length){
        j += this->get_topic_info(&buffer[k+j],errors);
    }    

}

int SUBSCRIBE_msg::get_header(uint8_t* buffer, int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    if(header_flags != 2){
        std::cout<< "ERROR. SUBSCRIBE header flags MUST be set to 2\n";
        errors = -1;
        return k;
    }
    k += this->get_remaining_length(&buffer[k]);
    if(this->rem_length ==0){
        std::cout<< "ERROR. SUBSCRIBE message must have a non-zero remaining length\n";
        errors = -1;
        return k;
    }
    return k;
}

int SUBSCRIBE_msg::get_topic_info(uint8_t* buffer, int& errors){
    int l = 0;

    uint16_t topic_length_ = (buffer[l]<<8 & 0xF0) | (buffer[l+1] & 0x0F);
    this->topic_length.push_back(topic_length_);
    l += 2;
    std::string topic_ ( (char*) &buffer[l], topic_length_);
    this->topic.push_back(topic_);
    l += topic_length_;
    this->QoS.push_back(buffer[l]);
    l++;
    return l;
}

int SUBSCRIBE_msg::set_header(uint8_t* buffer){
    int k =0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}

int SUBSCRIBE_msg::set_payload(uint8_t* buffer){
    int k = 0;
    std::list<uint8_t>::iterator   it_QoS = this->QoS.begin();
    for (std::list<std::string>::iterator it = topic.begin(); it != topic.end(); it++){
        buffer[k++] = (uint8_t) it->size()>>8; 
        buffer[k++] = (uint8_t) it->size();
        memcpy(&buffer[k], it->c_str(), it->size());
        k += it->size();
        buffer[k++] = *it_QoS;
        it_QoS++;
    }
    return k;
}

int SUBSCRIBE_msg::pack(uint8_t * buffer){
    int k = 0;
    //Fixed header
    k += this->set_header(&buffer[k]);
    //Variable header
    buffer[k++] = (uint8_t) (this->packet_ID>>8);
    buffer[k++] = (uint8_t) (this->packet_ID);
    //payload
    k += this->set_payload(&buffer[k]);
    return k;
}

uint16_t SUBSCRIBE_msg::get_packet_ID(void){
    return this->packet_ID;
}

std::list<std::string>& SUBSCRIBE_msg::get_topics(){
    return this->topic;
}


////////////////////////////////
////// SUBACK_msg methods //////
////////////////////////////////

SUBACK_msg::SUBACK_msg(uint16_t packet_ID_, std::list<uint8_t> return_code_){
    // Fixed header 
    this->type = SUBACK;
    this->header_flags = 0;
    int j = 0;
    // Variable header
    this->packet_ID = packet_ID_;
    j += 2;
    this->return_code.assign(return_code_.begin(), return_code_.end());
    j += this->return_code.size();
    this->rem_length = j;
}

SUBACK_msg::SUBACK_msg(uint8_t* buffer, int& errors){
    int k = 0; 
    // Fixed header 
    k+=this->get_header(&buffer[k],errors);
    // Variable header
    this->packet_ID = (buffer[k]<<8 & 0xF0) | (buffer[k+1] & 0x0F);
    k += 2;
    // Payload
    int left_bytes = this->rem_length - 2;
    if(left_bytes == 0){
        std::cout<< "ERROR. SUBACK MUST have almost one return code\n";
        errors = -1;
        return;
    }
    while(left_bytes > 0){
        this->return_code.push_back(buffer[k++]);
        left_bytes --;
    };
}

int SUBACK_msg::get_header(uint8_t* buffer, int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    k += this->get_remaining_length(&buffer[k]);
    return k;
}

int SUBACK_msg::set_header(uint8_t* buffer){
    int k =0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}

int SUBACK_msg::pack(uint8_t * buffer){
    int k = 0;
    //Fixed header
    k += this->set_header(&buffer[k]);
    //Variable header
    buffer[k++] = (uint8_t) (this->packet_ID>>8);
    buffer[k++] = (uint8_t) (this->packet_ID);
    //Payload
    for(std::list<uint8_t>::iterator it = this->return_code.begin(); it != this->return_code.end(); it++){
        buffer[k++] = *it;
    }
    return k;
}

/////////////////////////////////////
////// UNSUBSCRIBE_msg methods //////
/////////////////////////////////////

UNSUBSCRIBE_msg::UNSUBSCRIBE_msg(uint16_t packet_ID_, std::list<std::string> topic_){
    // Fixed header 
    this->type = UNSUBSCRIBE;
    this->header_flags = 2;
    int j = 0;
    // Variable header
    this->packet_ID = packet_ID_;
    j += 2;
    
    for (std::list<std::string>::iterator it = topic_.begin(); it != topic_.end(); it++){
        this->topic.push_back(*it);
        this->topic_length.push_back((uint16_t)(*it).size());
        j += (*it).size()+2; // 2 bytes for topic length
    }   
    this->rem_length = j;
}

UNSUBSCRIBE_msg::UNSUBSCRIBE_msg(uint8_t* buffer, int& errors){
    int k = 0; 
    // Fixed header 
    k+=this->get_header(&buffer[k],errors);
    // Variable header
    this->packet_ID = (buffer[k]<<8 & 0xF0) | (buffer[k+1] & 0x0F);
    k += 2;
    // Payload
    int left_bytes = this->rem_length - 2;
    if(left_bytes == 0){
        std::cout<< "ERROR. UNSUBSCRIBE MUST have at least one topic\n";
        errors = -1;
        return;
    }
    while(left_bytes > 0){
        int r;
        r = get_payload(&buffer[k],errors);
        left_bytes -= r;
        k += r;
    }
}

int UNSUBSCRIBE_msg::get_payload(uint8_t* buffer, int& errors){
        int l = 0;
        std::string topic_;
        uint16_t topic_length_;
        topic_length_ = (buffer[l]<<8 & 0xF0) | (buffer[l+1] & 0x0F);
        l += 2;
        topic_.assign(&buffer[l], &buffer[l+topic_length_]);
        l += topic_length_;
        this->topic.push_back(topic_);
        this->topic_length.push_back(topic_length_);

        return topic_length_+2;
}

int UNSUBSCRIBE_msg::get_header(uint8_t* buffer, int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    k += this->get_remaining_length(&buffer[k]);
    return k;
}

int UNSUBSCRIBE_msg::set_header(uint8_t* buffer){
    int k = 0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}

int UNSUBSCRIBE_msg:: set_payload(uint8_t* buffer){
    int k = 0;  
    for(std::list<std::string>::iterator it = this->topic.begin(); it != this->topic.end(); it++){
        buffer[k++] = (uint8_t) (it->size()>>8);
        buffer[k++] = (uint8_t) ( it->size());
        for(int i = 0; i < it->size(); i++){
            buffer[k++] = (*it)[i];
        }
    }
    return k;
}

int UNSUBSCRIBE_msg::pack(uint8_t* buffer){
    int k = 0; 
    // Fixed header 
    k += this->set_header(&buffer[k]);
    // Variable header
    buffer[k++] = (uint8_t) (this->packet_ID>>8);
    buffer[k++] = (uint8_t) (this->packet_ID);
    // Payload
    k += set_payload(&buffer[k]);

    return k;
}
    
std::list<std::string>& UNSUBSCRIBE_msg::get_topic_info(void){
    return this->topic;
}

uint16_t UNSUBSCRIBE_msg::get_packet_ID(){
    return this->packet_ID;
}


//////////////////////////////////
////// UNSUBACK_msg methods //////
//////////////////////////////////

UNSUBACK_msg::UNSUBACK_msg(uint16_t packet_ID_, std::list<uint8_t> return_code_){
    // Fixed header 
    this->type = UNSUBACK;
    this->header_flags = 0;
    int j = 0;
    // Variable header
    this->packet_ID = packet_ID_;
    j += 2;
    
    this->return_code.assign(return_code_.begin(), return_code_.end());
    j += this->return_code.size();
    this->rem_length = j;
}

UNSUBACK_msg::UNSUBACK_msg(uint8_t* buffer, int& errors){
    int k = 0; 
    // Fixed header 
    k+=this->get_header(&buffer[k],errors);
    // Variable header
    this->packet_ID = (buffer[k]<<8 & 0xF0) | (buffer[k+1] & 0x0F);
    k += 2;
    // Payload
    int left_bytes = this->rem_length - 2;
    if(left_bytes == 0){
        std::cout<< "ERROR. UNSUBACK MUST have almost one return code\n";
        errors = -1;
        return;
    }
    while(left_bytes > 0){
        this->return_code.push_back(buffer[k++]);
        left_bytes --;
    };
}

int UNSUBACK_msg::get_header(uint8_t* buffer, int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    k += this->get_remaining_length(&buffer[k]);
    return k;
}

int UNSUBACK_msg::set_header(uint8_t* buffer){
    int k = 0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}

int UNSUBACK_msg::pack(uint8_t* buffer){
    int k = 0; 
    // Fixed header 
    k += this->set_header(&buffer[k]);
    // Variable header
    buffer[k++] = (uint8_t) (this->packet_ID>>8);
    buffer[k++] = (uint8_t) (this->packet_ID);
    // Payload
        for(std::list<uint8_t>::iterator it = this->return_code.begin(); it != this->return_code.end(); it++){
        buffer[k++] = *it;
    }
    return k;
}
    
////////////////////////////////    
////// PUBACK_msg methods //////
////////////////////////////////


PUBLISH_msg::PUBLISH_msg(std::string topic_, uint8_t flags_, std::string payload_,uint16_t packet_ID_){
    // Fixed header 
    this->type = PUBLISH;
    this->header_flags = flags_;
    int j = 0;
    // Variable header
    this->topic=topic_;
    this->topic_length=this->topic.size();
    j+= this->topic_length+2;// 2 bytes for topic length
    
    this->packet_ID = packet_ID_;
    j+=2;
    
    // Payload
    this->payload=payload_;
    j += this->payload_length = this->payload.size();
    this->rem_length = j;
   
}

PUBLISH_msg::PUBLISH_msg(uint8_t* buffer, int& errors){
    int k = 0; 
    // Fixed header 
    k+=this->get_header(&buffer[k],errors);
    int l=0;
    // Variable header y payload
    l=+this->get_header_v_payload(&buffer[k],errors);
    this->rem_length = l;
}

int PUBLISH_msg::get_header(uint8_t* buffer, int& errors){
    int k = 0;
    this->get_type(&buffer[k]);
    this->get_header_flags(&buffer[k++]);
    k += this->get_remaining_length(&buffer[k]);
    return k;
}

int PUBLISH_msg::get_header_v_payload(uint8_t* buffer, int& errors){
    int k = 0;
    this->topic_length = buffer[k]<<8 & 0xF0| buffer[k+1] & 0x0F;
    k += 2;
    
    this->topic.assign(&buffer[k], &buffer[k+this->topic_length]);
    k += this->topic_length;
    this->packet_ID = buffer[k]<<8 & 0xF0| buffer[k+1] & 0x0F;
    k += 2;
    
    this->payload.assign(&buffer[k], &buffer[k+this->rem_length-k]);
    k += this->payload_length = this->payload.size();

    return k;
}

int PUBLISH_msg::set_header(uint8_t* buffer){
    int k = 0;
    this->set_type(&buffer[k]);
    this->set_header_flags(&buffer[k++]);
    k += this->set_remaining_length(&buffer[k]);
    return k;
}

int PUBLISH_msg::set_header_v_payload(uint8_t* buffer){
    int k = 0;
    buffer[k++] = (uint8_t) (this->topic_length>>8) & 0xF0;
    buffer[k++] = (uint8_t) (this->topic_length)& 0x0F;

    memcpy(&buffer[k], this->topic.c_str(), this->topic.size());
    k += this->topic_length;

    buffer[k++] = (uint8_t) (this->packet_ID>>8)& 0xF0;
    buffer[k++] = (uint8_t) (this->packet_ID)&  0x0F;
    
    memcpy(&buffer[k], this->payload.c_str(), this->payload_length);
    k += this->payload_length;
    
    return k;
}

int PUBLISH_msg::pack(uint8_t* buffer){
    int k = 0; 
    // Fixed header 
    k += this->set_header(&buffer[k]);
    // Variable header y payload
    k += this->set_header_v_payload(&buffer[k]);
    return k;
}

bool PUBLISH_msg::is_reteined(){
    return (this->header_flags & 0x01);
}

uint16_t PUBLISH_msg::get_packet_ID(){
    return this->packet_ID;
}

std::string PUBLISH_msg::get_payload(){
    return this->payload;
}

std::string PUBLISH_msg::get_topic(){
    return this->topic;
}