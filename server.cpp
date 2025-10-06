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
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<pthread.h>
#include<map>

#pragma comment(lib, "ws2_32.lib") // Link with winsock library

#define MAX_CLIENTS 10

SOCKET clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

struct thread_args{
    int client_sock;
};

void error(const char* msg){
    perror("Error occured: \n");
    exit(1);
}

// Thread to handle each client (receive-only)
void *handle_client(void *args){
    struct thread_args *targs = (struct thread_args *)args;
    char buffer[255];
    int client_sock = targs -> client_sock;
    free(args);
    int tid;
    for(int i=0; i< client_count; i++){
        if(clients[i] == client_sock){
            tid = i;
            break;
        }
    }

    // Communication Loop
    while(1){
        memset(buffer, 0, 255);
        
        int n = recv(client_sock, buffer, 255, 0);
        if(n == SOCKET_ERROR){
            printf("Error on reading! %d\n", WSAGetLastError());
            break;
        }
        printf("Client %d: %s\n", tid, buffer);
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("Client requested to end the chat.\n"); 
            break;            
        }
    }
    closesocket(client_sock);
    pthread_mutex_lock(&clients_lock);
    for(int i=0; i<client_count;i++){
        if(clients[i] == client_sock){
            for(int j=i; j<client_count-1; j++){
                clients[j] = clients[j+1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_lock);
    return NULL;
}

// Thread to handle server input and choose client to send message
void* server_input(void* arg){
    char buffer[255];
    while(1){
        int target;
        //printf("Enter client index to send message (0-%d): ", client_count-1);
        scanf("%d", &target);
        getchar(); // consume newline
        
        pthread_mutex_lock(&clients_lock);
        if(target < 0 || target >= client_count){
            printf("Invalid client index\n");
            continue;
        }
        SOCKET client_sock = clients[target];
        pthread_mutex_unlock(&clients_lock);

        printf("Message: ");
        fgets(buffer, 255, stdin);
        int n = send(clients[target], buffer, strlen(buffer), 0);
        if(n == SOCKET_ERROR){
            printf("Error sending message: %d\n", WSAGetLastError());
        }
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

    // Initialize Winsock
    if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        exit(1);
    }

    // Create Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == INVALID_SOCKET){
        printf("Error opening socket! %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }
    printf("Id: %d\n", sockfd);

    //Initialize server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    // net_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    printf("IP: %s\n", inet_ntoa(serv_addr.sin_addr));
    printf("Family: %d\n", serv_addr.sin_family);

    // Bind
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR){
        printf("Binding failed! Error code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        exit(1);
    }

    // Listen
    if(listen(sockfd, 5) == SOCKET_ERROR){
        printf("Listen  failed! Error code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        exit(1);
    }
    printf("Server listening on port %d...\n", portno);
    clilen = sizeof(cli_addr);

    // Launch server input thread
    pthread_t input_tid;
    pthread_create(&input_tid, NULL, server_input, NULL);
    pthread_detach(input_tid);

    while(1){
        // Accept client connection
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if(newsockfd == INVALID_SOCKET){
            printf("Accept failed! Error code: %d\n", WSAGetLastError());
            closesocket(sockfd);
            WSACleanup();
            exit(1);
        }
        printf("Accepted connection from %s:%d\n",
            inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        // Add to client list
        pthread_mutex_lock(&clients_lock);
        if(client_count < MAX_CLIENTS){
            clients[client_count++] = newsockfd;
        } else{
            printf("Max clients reached. Connection rejected.\n");
            closesocket(newsockfd);
            pthread_mutex_unlock(&clients_lock);
            continue;
        }
        pthread_mutex_unlock(&clients_lock);

        // Create client thread
        struct thread_args *targs = (struct thread_args *)malloc(sizeof(struct thread_args));
        targs -> client_sock = newsockfd;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void *)targs);
        pthread_detach(tid);
    }

    // Close Sockets
    closesocket(sockfd);
    WSACleanup();
    return 0;
}