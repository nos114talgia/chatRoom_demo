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
#include "server.h"

int main(){
    // 1. Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0){
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Bind socket to address and start listening
    struct sockaddr_in server_address;
    std::memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address)) < 0){
        std::cerr << "Error binding socket" << std::endl;
        return -1;
    }

    if(listen(server_socket, 128) < 0){
        std::cerr << "Error listening" << std::endl;
        return -1;
    }
    std::cout << "Server listening on port " << PORT << std::endl;
    
    // 3. Accept connections
    struct sockaddr_in client_address;
    while(true){
        std::memset(&client_address, 0, sizeof(client_address));
        socklen_t client_address_len = sizeof(client_address);

        int client_socket = accept(server_socket, (struct sockaddr*)(&client_address), &client_address_len);
        if(client_socket < 0){
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }
        char ip_str[INET_ADDRSTRLEN];
        if(inet_ntop(AF_INET, &client_address.sin_addr, ip_str, INET_ADDRSTRLEN)){
            std::cout << "Client connected from " << ip_str << ":" << ntohs(client_address.sin_port) << std::endl;
        }else{
            std::cout << "Client connected from unknown" << std::endl;
        }

        // 4. Start thread
        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();

    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket){
    char username[256];
    int username_len = recv(client_socket, username, 255, 0);
    if( username_len < 0){
        std::cerr << "Error receiving username" << std::endl;
        close(client_socket);
        return;
    }else if(username_len == 0){
        std::cerr << "Client disconnected" << std::endl;
        close(client_socket);
        return;
    }else{
        username[username_len] = '\0';
        std::cout << "Username: " << username << std::endl;
    }

    std::pair<int, std::string> user_info = std::make_pair(client_socket, username);
    {
        std::lock_guard<std::mutex> lock(client_sockets_and_usernames_mutex);
        client_sockets_and_usernames.push_back(user_info);
    }
    while(true){
        char buffer[1024];
        int read_size = recv(client_socket, buffer, 1023, 0);

        if(read_size <= 0){
            if(read_size < 0){
                std::cerr << "Error receiving message" << std::endl;
            }else{
                std::cout << "Client disconnected" << std::endl;
            }
            break;
        }else{
            buffer[read_size] = '\0';
        }
        // send the message to all other users
        {
            std::lock_guard<std::mutex> lock(client_sockets_and_usernames_mutex);
            std::string message = "@" + std::string{username} + ": " + std::string{buffer};
            for(const auto& user : client_sockets_and_usernames){
                if(user.first == client_socket){
                    continue;
                }
                send(user.first, message.c_str(), message.size(), 0);
            }
        }
    }

    // delete user info
    {
        std::lock_guard<std::mutex> lock(client_sockets_and_usernames_mutex);

        auto it = std::find_if(client_sockets_and_usernames.begin(), client_sockets_and_usernames.end(), 
        [&user_info](const std::pair<int, std::string>& p){return p.first == user_info.first;});

        if(it != client_sockets_and_usernames.end()){
            client_sockets_and_usernames.erase(it);
        }
    }
    close(client_socket);

}