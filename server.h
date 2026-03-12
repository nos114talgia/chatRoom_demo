#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <string>
#include <mutex>

#define PORT 8080

inline std::vector<std::pair<int, std::string>> client_sockets_and_usernames; // user list
inline std::mutex client_sockets_and_usernames_mutex;

extern void handle_client(int);

#endif