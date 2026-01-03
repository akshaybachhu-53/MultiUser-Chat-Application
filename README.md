# 🗨️ Multi-Client Chat Application (C, TCP, Threads)

A **multi-client chat application** built in **C** using **TCP sockets**, **pthreads**, and **Winsock (Windows)**.  
Clients communicate through a central server using **private** or **broadcast** messages with **username-based routing**.

---

## 🚀 Features

- Multi-client support (thread-per-client)
- TCP client–server architecture
- Private (`@p`) and Broadcast (`@b`) messaging
- Username registration
- Clean client controls: **p / b / q / e**
- Server-side message formatting: `username: message`
- Modular, readable code (logic refactored into functions)

---

### Server
- Accepts clients using `accept()`
- Spawns a thread per client
- Maintains a linked list of connected clients
- Routes messages (private/broadcast)

### Client
- Receive thread always listening
- Main thread handles input & sending
- Simple command-based UX

---

## 💬 Messaging Protocol

| Command | Description |
|------|------------|
| `@u <username>` | Register username |
| `@p <user> <msg>` | Private message |
| `@b <msg>` | Broadcast message |
| `q` | Quit current chat (stay connected) |
| `e` | Exit client (disconnect) |

---

## 🧩 Client Menu
Choose option:
p - Private chat
b - Broadcast
q - Quit current chat
e - Exit client


---

## 🛠️ Tech Stack

- **Language:** C
- **Networking:** TCP sockets
- **Concurrency:** pthreads
- **Platform:** Windows (Winsock2)
- **Compiler:** gcc (MSYS2 / MinGW)

---

## ▶️ Build & Run

### Compile Server
gcc server.c -o server.exe -lws2_32 -lpthread

### Compile Client
gcc client.c -o client.exe -lws2_32 -lpthread


Run Server
./server.exe 8080

Run Client(s)
./client.exe 127.0.0.1 8080
Run multiple clients in separate terminals.

Author
Akshay Bachhu