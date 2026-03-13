# C++ TCP Chat Room

## Overview

This project implements a simple **TCP-based chat room** using C++.
It includes a **server** and a **client**. Multiple clients can connect to the server, send messages, and receive messages from other users in real time.

The server handles multiple clients using **multithreading**, and messages are broadcast to all connected users except the sender.

The epoll-server handles multiple clients using Linux's **I/O Multiplexing** capabilities.

---

## Features

* TCP socket communication
* Multi-client support
* Multi-threaded server
* Username identification
* Message broadcasting to all connected clients

---

## Project Structure

epoll_server.cpp
server.cpp
server.h
client.cpp
chatRoom.h

* **epoll_server.cpp** - Implements the chat server(epoll-edition)
* **server.cpp** – Implements the chat server
* **server.h** – Shared server definitions and global variables
* **client.cpp** – Implements the chat client
* **chatRoom.h** – Client function declarations

```bash
.
├── CMakeLists.txt
├── chatRoom.cpp
├── chatRoom.h
├── server.cpp
├── server.h
├── epoll_server.cpp
└── build/
```

---

## How It Works

### Server

1. Creates a TCP socket.
2. Binds to port `8080`.
3. Listens for incoming client connections.
4. For each new connection, a new thread is created to handle the client.
5. The server stores `(socket, username)` pairs in a shared list.
6. When a client sends a message, the server broadcasts it to all other clients.

### Client

1. Connects to the server (`127.0.0.1:8080`).
2. Prompts the user to enter a username.
3. Sends the username to the server.
4. Starts two threads:

   * One for **sending messages**
   * One for **receiving messages**

---

## How Epoll-server Works

### 1. Initialization and Non-Blocking Setup
The server begins by creating a standard TCP socket. To achieve high concurrency, the socket is set to **Non-Blocking** mode using `fcntl`. This prevents the server from hanging on a single `accept` or `recv` call, allowing it to move quickly between different client events.

### 2. Epoll Instance & Edge-Triggered (ET) Registration
An `epoll` instance is created via `epoll_create1`. The server socket is added to the interest list with the `EPOLLET` (Edge-Triggered) flag. 
* **Why ET?** Unlike Level-Triggered mode, ET only notifies the server once when new data arrives. This reduces redundant event notifications and is significantly more efficient for high-load environments.

### 3. The Central Event Loop
The `epoll_event_loop` function runs an infinite loop calling `epoll_wait`. 
* When `epoll_wait` returns, it provides a list of file descriptors (FDs) that are ready for I/O.
* **Server FD Ready:** If the activity is on the listening socket, the server enters a loop to `accept4` all pending incoming connections until the queue is empty (indicated by `EAGAIN`).
* **Client FD Ready:** If activity is on a client socket, the server triggers the data handling logic.

### 4. Edge-Triggered Data Processing
Because the server uses Edge-Triggered mode, the `handle_client_event` function must read **all** available data in a single event trigger. 
* It uses a `while(true)` loop with `recv` to drain the kernel buffer.
* If `recv` returns `EAGAIN` or `EWOULDBLOCK`, it means there is no more data to read for now, and the server safely returns to the event loop.

### 5. Thread-Safe User Management
The server maintains a mapping of file descriptors to usernames (`fd_to_name`). To ensure data consistency:
* **`std::shared_mutex`**: This allows a "Multiple Readers, Single Writer" approach. 
* When broadcasting messages, the server acquires a **shared lock** (read lock), allowing multiple threads to broadcast simultaneously.
* When a user joins or leaves (modifying the map), it acquires a **unique lock** (write lock) to prevent race conditions.

### 6. Chat Protocol Logic
The server implements a simple state-based logic for every client:
1. **Registration Phase**: The very first string a client sends is recorded as their username.
2. **Messaging Phase**: Every subsequent message is prefixed with the sender's name (e.g., `@Alice: Hello!`) and broadcasted to every other active file descriptor found in the map.
3. **Disconnection**: If `recv` returns 0, the server removes the user from the map and closes the socket, automatically cleaning up resources.

---

## Build
```bash
mkdir build
cd build
cmake..
make
```

---

## Run

Start the server first:
```bash
./server
```
Then start one or more clients:
```bash
./client
```
Enter a username and start chatting.

---

## Example

Client A:

Enter username: Alice

Client B:

Enter username: Bob

Bob sends:

Hello everyone

Alice sees:

@Bob: Hello everyone

---

## Notes

* The server listens on **port 8080**.
* The current client implementation connects to **localhost (127.0.0.1)**.
* The server uses a **mutex** to protect the shared client list when multiple threads access it.

---
