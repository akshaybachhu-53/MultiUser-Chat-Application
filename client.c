/*
    Client side
    socket()
    connect()
    read()
    write()
    close()
*/

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<unistd.h>
#pragma comment(lib, "ws2_32.lib") // Link with winsock library

SOCKET sockfd;

void error(const char *msg) {
    fprintf(stderr, "%s. Error code: %d\n", msg, WSAGetLastError());
    exit(1);
}

// Send Messages
void *sendMessages(void* args){
    char buffer[1024];

    while(1) {
        printf("You: ");
        fflush(stdout);

        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        send(sockfd, buffer, strlen(buffer), 0);

        if(strcmp(buffer, "Bye") == 0) {
            printf("Exiting chat...\n");
            closesocket(sockfd);
            WSACleanup();
            exit(0);
        }
    }
    return NULL;
}

// Receive Messages
void *receiveMessages(void *args){
    char buffer[1024];

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if(n <= 0){
            printf("\nServer disconnected.\n");
            exit(0);
        }

        buffer[n] = '\0';
        printf("\nServer :%s\nYou: ", buffer);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]){
    if(argc < 3){
        fprintf(stderr, "All arguments not provided. Program terminated!");
        exit(1);
    }

    WSADATA wsa;
    struct sockaddr_in serv_addr;
    int portno, n;
    char buffer[1024];

    portno = atoi(argv[2]);

    // Initialize Winsock
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0){
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        exit(1);
    }

    // Create Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == INVALID_SOCKET){
        error("ERROR opening socket");
    }

    // Setup server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    // Convert hostname (argv[1]) IP string to binary
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(argv[1], NULL, &hints, &res) != 0){
        error("No such host");
    }

    struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;
    serv_addr.sin_addr = addr_in->sin_addr;
    freeaddrinfo(res);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        error("Connection Failed");
    }
    printf("Connected to server!\n");

    // Create two threads
    pthread_t sendThread, recvThread;

    pthread_create(&sendThread, NULL, sendMessages, NULL);
    pthread_create(&recvThread, NULL, receiveMessages, NULL);
    
    pthread_join(sendThread, NULL);
    pthread_join(recvThread, NULL);

    closesocket(sockfd);
    WSACleanup();

    return 0;
}