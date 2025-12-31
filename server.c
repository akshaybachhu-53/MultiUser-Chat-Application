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

typedef struct ClientNode {
    sock_t sock;
    struct ClientNode* next;
} ClientNode;

ClientNode *clients_head = NULL;
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

 /* ---------------- Add client ------------- */
void add_client(sock_t s) {
    ClientNode *node = (ClientNode*)malloc(sizeof(ClientNode));
    node->sock = s;
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
void *client_handler(void* arg) {
    sock_t client_sock = *((sock_t*)arg);
    free(arg);

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
        printf("Client(%d): %s\n", (int)client_sock, buffer);

        // If client wants to end conversation
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("Client(%d) said Bye. Closing connection.\n", (int)client_sock);
            remove_client(client_sock);
            closesocket(client_sock);
            return NULL;
        }

        // Server reply
        char reply[1200];
        snprintf(reply, sizeof(reply), "Server: Received \"%s\"\n", buffer);
        send(client_sock, reply, (int)strlen(reply), 0);
    }
    return NULL;
}

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "Port No not provided. Program terminated\n");
        exit(1);
    }
    
    WSADATA wsa;
    SOCKET sockfd, newsockfd;
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
    clilen = sizeof(cli_addr);

    // 6. Accept Client
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd == INVALID_SOCKET) {
        printf("Accept failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("Client connected: %s:%d\n",
           inet_ntoa(cli_addr.sin_addr),
           ntohs(cli_addr.sin_port));


    // Communication loop
    while (1) {
        memset(buffer, 0, sizeof(buffer));

        int n = recv(newsockfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[n] = '\0';
        printf("Client: %s\n", buffer);

        if (strcmp(buffer, "Bye") == 0) {
            printf("Chat closed.\n");
            break;
        }

        // Send same message back (echo)
        send(newsockfd, buffer, strlen(buffer), 0);
    }

    // Close Sockets CleanUp
    closesocket(newsockfd);
    closesocket(sockfd);
    WSACleanup();
    return 0;
}