#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include "chatRoom.h"

int main(){
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket == -1){
        std::cout << "Error creating socket" << std::endl;
        return -1;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) < 0){
        std::cout << "Error converting ip address" << std::endl;
        return -1;
    }
    if(connect(client_socket, (struct sockaddr*)(&server_address), sizeof(server_address)) < 0){
        std::cout << "Error connecting to server" << std::endl;
        return -1;
    }
    std::cout << "Connected to server" << std::endl;

    std::string username;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);

    send(client_socket, username.c_str(), username.length(), 0);

    std::thread receive_msg(receive_message, client_socket);
    receive_msg.detach();
    std::thread send_msg(send_message, client_socket);
    send_msg.join();  // keep it running, otherwise the mainf will close


    close(client_socket);
    return 0;
    
}

void send_message(int client_socket){ 
    while(true){
        std::string message;
        getline(std::cin, message);
        if(send(client_socket, message.c_str(), message.length(), 0) < 0){
            std::cout << "Error sending message" << std::endl;
            break;
        }
    }
    return;
}

void receive_message(int client_socket){
    while(true){
        char buffer[1024];
        memset(buffer, 0, 1024);
        int len = recv(client_socket, buffer, 1023, 0);
        if(len <= 0){
            std::cout << "Error receiving message" << std::endl;
            break;
        }
        buffer[len] = '\0';
        std::cout << buffer << std::endl;
    }
    return;
}