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
#include <sys/epoll.h>
#include <fcntl.h>
#include <shared_mutex>
#include <unordered_map>
#define PORT 8080
#define MAX_EVENTS 1024

std::unordered_map<int, std::string> fd_to_name;     // user list
std::unordered_map<int, std::string> client_buffer;  // user buffer
std::shared_mutex client_info_mutex;

void handle_client_event(int);
void epoll_event_loop(int, int);

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
    
    // 3. utilize epoll to handle multiple clients
    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0){
        std::cerr << "Error creating epoll" << std::endl;
        return -1;
    }
    struct epoll_event server_event;
    server_event.data.fd = server_socket;
    server_event.events = EPOLLIN | EPOLLET;

    // 4. set server socket to non-blocking
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_event.data.fd, &server_event) < 0){
        std::cerr << "Error adding server socket to epoll" << std::endl;
        close(server_socket);
        return -1;
    }

    epoll_event_loop(server_socket, epoll_fd);

    close(server_socket);
    return 0;
}

void epoll_event_loop(int server_socket, int epoll_fd){ 
    std::vector<struct epoll_event> events(MAX_EVENTS);
    while(true){ 
        int n = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        if(n == -1){
            if(errno == EINTR){
                std::cerr << "Interrupted by signal" << std::endl;
                continue;
            }else{
                std::cerr << "Error waiting for events" << std::endl;
                return;
            }
        }else{
            for(int i = 0; i < n; i++){
                int fd = events[i].data.fd;
                if(fd == server_socket){
                    struct sockaddr_in client_address;
                    std::memset(&client_address, 0, sizeof(client_address));
                    socklen_t client_address_len = sizeof(client_address);
                    while(true){
                        int client_socket = accept4(server_socket, (struct sockaddr*)(&client_address), &client_address_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if(client_socket < 0){
                            if(errno == EWOULDBLOCK || errno == EAGAIN){
                                break;
                            }else if(errno == EINTR){
                                std::cerr << "Interrupted by signal" << std::endl;
                                continue;
                            }else{
                                std::cerr << "Error accepting client" << std::endl;
                                break;
                            }
                        }
                        struct epoll_event client_event = {};
                        client_event.data.fd = client_socket;
                        client_event.events = EPOLLIN | EPOLLET;
                        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &client_event) < 0){
                            std::cerr << "Error adding client socket to epoll" << std::endl;
                            close(client_socket);
                            continue;
                        }
                        std::cout << "New client connected" << std::endl;
                        
                    }
                }else if(events[i].events & EPOLLIN){
                    handle_client_event(fd);
                }
            }
        }
    }
    
}

void handle_client_event(int client_fd){
    char buffer[1024];
    while(true){
        ssize_t bytes = recv(client_fd, buffer, 1024, 0);
        if(bytes > 0){
            std::vector<std::string> complete_lines;
            {
                std::unique_lock<std::shared_mutex> lock(client_info_mutex);
                std::string& buf = client_buffer[client_fd];
                buf.append(buffer, bytes);
                ssize_t pos;
                while((pos = buf.find('\n')) != std::string::npos){
                    complete_lines.push_back(buf.substr(0, pos));
                    buf.erase(0, pos + 1);
                }
            }

            for(auto& line : complete_lines){
                if(!line.empty() && line.back() == '\r'){
                    line.pop_back();
                }
                if(line.empty()){
                    continue;
                }
                bool is_registration = false;
                std::string sender_name;
                {
                    std::unique_lock<std::shared_mutex> write_lock(client_info_mutex);
                    auto it = fd_to_name.find(client_fd);
                    if(it == fd_to_name.end()){
                        fd_to_name[client_fd] = line;
                        sender_name = line;
                        is_registration = true;
                    }else{
                        sender_name = it->second;
                        is_registration = false;
                    }
                }
                if(is_registration){
                    std::string welcome = "Welcome " + sender_name + "!\n";
                    send(client_fd, welcome.c_str(), welcome.size(), 0);
                    std::cout << "[Log] New User registered: " << sender_name << " (FD: " << client_fd << ")" << std::endl;
                }else{
                    std::string broadcast_msg = "@" + sender_name + ": " + line + "\n";
                    std::shared_lock<std::shared_mutex> read_lock(client_info_mutex);
                    for (const auto& [target_fd, name] : fd_to_name) {
                        if (target_fd != client_fd) {
                            send(target_fd, broadcast_msg.c_str(), broadcast_msg.size(), 0);
                        }
                    }
                }
            }
        }else if(bytes == 0){
            std::unique_lock<std::shared_mutex> write_lock(client_info_mutex);
            std::cout << "Client " << fd_to_name[client_fd] << " disconnected" << std::endl;
            fd_to_name.erase(client_fd);
            client_buffer.erase(client_fd);
            write_lock.unlock();
            close(client_fd);
            break;
        }else{
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                break;
            }else if(errno == EINTR){
                continue;
            }else{
                std::cerr << "Error receiving message from client" << std::endl;
                std::unique_lock<std::shared_mutex> write_lock(client_info_mutex);
                fd_to_name.erase(client_fd);
                client_buffer.erase(client_fd);
                close(client_fd);
                break;
            }

        }
    }
}
