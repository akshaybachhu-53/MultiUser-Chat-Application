/*
Client Side
socket()
connect()
read()
write()
close()
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<winsock2.h>
#include<ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Link with winsock library

void error(const char *msg) {
    fprintf(stderr, "%s. Error code: %d\n", msg, WSAGetLastError());
    WSACleanup();
    exit(1);
}

int main(int argc, char *argv[]){
    if(argc < 3){
        fprintf(stderr, "Port No. not provide. Program terminated\n");
        exit(1);
    }

    WSADATA wsa;
    SOCKET sockfd, newsockfd;
    struct sockaddr_in serv_addr;
    int portno, n;
    char buffer[256];

    portno = atoi(argv[2]);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        exit(1);
    }

    // Create Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == INVALID_SOCKET){
        error("ERROR opening socket");
    }

    // Setup  server address
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

    // Communication loop
    while (1) {
        memset(buffer, 0, 255);
        fgets(buffer, 255, stdin);
        n = send(sockfd, buffer, strlen(buffer), 0);
        if (n == SOCKET_ERROR) {
            error("Error on writing");
        }
        if(strncmp("Bye", buffer, 3) == 0){
            break;
        }

        memset(buffer, 0, 255);
        n = recv(sockfd, buffer, 255, 0);
        if (n == SOCKET_ERROR) {
            error("Error on reading");
        }
        printf("Server: %s", buffer);
        if (strncmp("Bye", buffer, 3) == 0) {
            break;
        }
    }

    closesocket(sockfd);
    WSACleanup();

    return 0;
}