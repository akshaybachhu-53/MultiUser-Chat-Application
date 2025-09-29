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
#include<unistd.h>

#pragma comment(lib, "ws2_32.lib") // Link with winsock library

struct thread_args{
    int client_sock;
};
void error(const char* msg){
    perror("Error occured: \n");
    exit(1);
}
void *handle_client(void *args){
    struct thread_args *targs = (struct thread_args *)args;
    char buffer[255];
    int client_sock = targs -> client_sock;
    free(args);
    pthread_t tid = pthread_self();

    // Communication Loop
    while(1){
        memset(buffer, 0, 255);
        
        int n = recv(client_sock, buffer, 255, 0);
        if(n == SOCKET_ERROR){
            printf("Error on reading! %d\n", WSAGetLastError());
            break;
        }
        printf("Client %lu: %s\n", (unsigned long)tid, buffer);
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("Client requested to end the chat.\n"); 
            break;            
        }

        memset(buffer, 0, 255);
        fgets(buffer, 255, stdin);

        n = send(client_sock, buffer, strlen(buffer), 0);
        if(n == SOCKET_ERROR){
            printf("Error on writing! %d\n", WSAGetLastError());
            break;
        }
        if(strncmp("Bye", buffer, 3) == 0){
            printf("Server requested to end the chat.\n");
            break;
        }
    }
    closesocket(client_sock);
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
        // int *client_sock = (int*)malloc(sizeof(int));
        // *client_sock = newsockfd;
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