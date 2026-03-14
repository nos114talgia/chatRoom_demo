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
## How the Optimized Epoll-Server Works

### 1. Non-Blocking Foundation
The server initializes a TCP socket and immediately sets it to **Non-Blocking** mode using `fcntl`. This is critical for the Edge-Triggered model: it ensures that when the server performs I/O operations (like `accept4` or `recv`), it never hangs, allowing the system to handle thousands of concurrent "silent" connections without wasting CPU cycles.

### 2. Edge-Triggered (ET) Event Loop
The server utilizes `epoll` in **Edge-Triggered (ET)** mode (`EPOLLET`). 
*   **Efficiency:** Unlike Level-Triggered mode, ET only notifies the server when a state change occurs (e.g., new data arrives). 
*   **The "Drain" Requirement:** Because of ET, the server must fully "drain" the kernel buffers. For new connections, it loops `accept4` until `EAGAIN`. For data, it loops `recv` until no more bytes are available.

### 3. Application-Layer Buffering (Handling TCP Streams)
TCP is a byte-stream protocol, meaning message boundaries are not guaranteed (data may arrive in fragments or "stick" together). The server solves this using a **Per-Client Buffer**:
*   **Accumulation:** Raw bytes from `recv` are appended to a `std::string` buffer unique to each File Descriptor (`client_buffer[fd]`).
*   **Delimiter Parsing:** The server searches for the `\n` delimiter. Only when a complete line is found is the data moved to the processing stage. This ensures that even if a client sends a message in pieces, the server reconstructs it perfectly.

### 4. Optimized Lock Splitting Strategy
To maintain high performance under heavy load, the server employs a **Granular Locking** strategy using `std::shared_mutex` and C++17 `inline` variables:
*   **Phase A (Write Lock):** A short-lived `unique_lock` is used to update the client's private buffer and extract complete lines into a local thread-safe vector.
*   **Phase B (Write Lock):** If the user is unregistered, a `unique_lock` is used briefly to update the `fd_to_name` map.
*   **Phase C (Shared Lock):** During broadcasting, the server switches to a `shared_lock` (read lock). This allows multiple threads to read the user map and send messages simultaneously, preventing a slow `send` operation to one client from blocking the registration of another.

### 5. Robust Protocol Handling
The server implements a clean, line-based chat protocol:
*   **Registration:** The first complete line received (trimmed of `\r\n`) is registered as the client's identity.
*   **Broadcasting:** Every subsequent line is treated as a chat message. The server prefixes it with the sender's name (e.g., `@Bob: hello`) and iterates through the map to forward it to all other active peers.
*   **Cleanup:** Upon receiving 0 bytes (graceful close) or a TCP error, the server acquires a write lock to remove the user from the map and the buffer, ensuring no memory leaks or dangling file descriptors.

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
