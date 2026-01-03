/*
Server Side
socket()
bind()
listen()
accept()
read()
write()
close()
*/

/* ------------------ Multi-Client TCP Server (Windows) ------------------ */
#include<stdio.h>
#include<string.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<pthread.h>
#include<string.h>
#include<time.h>     

#pragma comment(lib, "ws2_32.lib") // Link with winsock library

typedef SOCKET sock_t;

// Function prototypes
void broadcast(SOCKET sender, char *msg);
void send_private(const char *target_user, const char *msg, sock_t sender);
void server_loop(SOCKET sockfd);


typedef struct ClientNode {
    sock_t sock;
    char username[32];
    struct ClientNode* next;
} ClientNode;

ClientNode *clients_head = NULL;
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

const char* get_username_by_socket(sock_t s) {
    ClientNode *c = clients_head;
    while (c) {
        if (c->sock == s) {
            return c->username;
        }
        c = c->next;
    }
    return "Unknown";
}

/* ---------------- Add client ------------- */
void add_client(sock_t s) {
    ClientNode *node = (ClientNode*)malloc(sizeof(ClientNode));
    node->sock = s;
    node->username[0] = '\0';
    node->next = NULL;

    pthread_mutex_lock(&clients_lock);
    node->next = clients_head;
    clients_head = node;
    pthread_mutex_unlock(&clients_lock);
}

/* ---------------- Remove Client ---------------- */
void remove_client(sock_t s) {
    pthread_mutex_lock(&clients_lock);
    ClientNode *prev = NULL, *cur = clients_head;
    while(cur) {
        if(cur->sock == s) {
            if(prev) prev->next = cur->next;
            else clients_head = cur->next; // if s is head itself
            free(cur);
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    pthread_mutex_unlock(&clients_lock);
}

/* ---------------- Client Handler Thread -------------- */
void *client_handler(void* args) {
    sock_t client_sock = *((sock_t*)args);
    free(args);

    char buffer1[1024];

    // first message = username
    int n = recv(client_sock, buffer1, sizeof(buffer1) - 1, 0);
    if (n <= 0) {
        closesocket(client_sock);
        return NULL;
    }

    buffer1[n] = '\0';

    if (strncmp(buffer1, "@u ", 3) == 0) {
        char uname[32];
        sscanf(buffer1 + 3, "%31s", uname);

        pthread_mutex_lock(&clients_lock);
        ClientNode *c = clients_head;
        while (c) {
            if (c->sock == client_sock) {
                strcpy(c->username, uname);
                break;
            }
            c = c->next;
        }
        pthread_mutex_unlock(&clients_lock);

        printf("Client registered as: %s\n", uname);
    }

    char buffer[1024];

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);

        if(n <= 0){
            printf("Client disconnected (sock=%d)\n", (int)client_sock);
            remove_client(client_sock);
            closesocket(client_sock);
            return NULL;
        }

        buffer[n] = '\0';

        // If client wants to end conversation
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("Client(%d) said Bye. Closing connection.\n", (int)client_sock);
            remove_client(client_sock);
            closesocket(client_sock);
            return NULL;
        }

        // Broadcast Message
        if (strncmp(buffer, "@b ", 3) == 0){
            broadcast(client_sock, buffer + 3);
        } else if (strncmp(buffer, "@p ", 3) == 0) {
            char target[32];
            char msg[1024];

            // extract: username + message
            if (sscanf(buffer + 3, "%31s %[^\n]", target, msg) == 2) {
                send_private(target, msg, client_sock);
            }
        }
    }
    return NULL;
}

/* -------------- Broadcast messages to all clients --------------- */
void broadcast(SOCKET sender, char *msg) {
    pthread_mutex_lock(&clients_lock);

    const char *sender_name = get_username_by_socket(sender);
    char out[1100];

    snprintf(out, sizeof(out), "%s: %s", sender_name, msg);

    ClientNode *c = clients_head;
    while (c) {
        if (c->sock != sender) {
            send(c->sock, out, strlen(out), 0);
        }
        c = c->next;
    }

    pthread_mutex_unlock(&clients_lock);
}

/* --------------- Server Sends Private Message To Other Client ------------- */
void send_private(const char *target_user, const char *msg, sock_t sender) {
    pthread_mutex_lock(&clients_lock);

    const char *sender_name = get_username_by_socket(sender);
    char out[1100];

    snprintf(out, sizeof(out), "%s: %s", sender_name, msg);

    ClientNode* c = clients_head;
    while (c) {
        if (strcmp(c->username, target_user) == 0) {
            send(c->sock, out, strlen(out), 0);
            break;
        }
        c = c->next;
    }

    pthread_mutex_unlock(&clients_lock);
}

/* -------------- Server Broadcasts it's message to all  clients ---------------- */
void *server_input_thread(void *arg) {
    char msg[1024];

    while (1) {
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = 0;

        // Broadcast admin message to all clients
        pthread_mutex_lock(&clients_lock);

        ClientNode *c = clients_head;
        while (c) {
            send(c->sock, msg, strlen(msg), 0);
            c = c->next;
        }

        pthread_mutex_unlock(&clients_lock);
    }
    return NULL;
}

void server_loop(SOCKET sockfd) {
    struct sockaddr_in cli_addr;
    int clilen;

    while (1) {
        clilen = sizeof(cli_addr);
        SOCKET client = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (client == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        printf("Client connected: %s:%d\n",
               inet_ntoa(cli_addr.sin_addr),
               ntohs(cli_addr.sin_port));

        add_client(client);

        pthread_t tid;
        SOCKET *p = malloc(sizeof(SOCKET));
        *p = client;

        pthread_create(&tid, NULL, client_handler, p);
        pthread_detach(tid);
    }
}


int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "Port No not provided. Program terminated\n");
        exit(1);
    }
    
    WSADATA wsa;
    SOCKET sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    int portno;
    int clilen;
    char buffer[1024];

    // 1. Initialize Winsock
    if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        exit(1);
    }

    // 2. Create Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == INVALID_SOCKET){
        printf("Error opening socket! %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }
    printf("Id: %d\n", sockfd);

    // 3. Initialize server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    printf("IP: %s\n", inet_ntoa(serv_addr.sin_addr));
    printf("Family: %d\n", serv_addr.sin_family);

    // 4. Bind
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR){
        printf("Binding failed! Error code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        exit(1);
    }

    // 5. Listen
    if(listen(sockfd, 5) == SOCKET_ERROR){
        printf("Listen  failed! Error code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        exit(1);
    }
    printf("Server listening on port %d...\n", portno);

    pthread_t admin_thread;
    pthread_create(&admin_thread, NULL, server_input_thread, NULL);
    pthread_detach(admin_thread);

    // Communication
    server_loop(sockfd);

    // Close Sockets CleanUp
    closesocket(sockfd);
    WSACleanup();
    return 0;
}