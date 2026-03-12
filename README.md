# C++ TCP Chat Room

## Overview

This project implements a simple **TCP-based chat room** using C++.
It includes a **server** and a **client**. Multiple clients can connect to the server, send messages, and receive messages from other users in real time.

The server handles multiple clients using **multithreading**, and messages are broadcast to all connected users except the sender.

---

## Features

* TCP socket communication
* Multi-client support
* Multi-threaded server
* Username identification
* Message broadcasting to all connected clients

---

## Project Structure

server.cpp
server.h
client.cpp
chatRoom.h

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
