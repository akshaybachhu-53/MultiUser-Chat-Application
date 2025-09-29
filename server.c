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

#pragma comment(lib, "ws2_32.lib") // Link with winsock library

void error(const char* msg){
    perror("Error occured: \n");
    exit(1);
}

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "Port No not provided. Program terminated\n");
        exit(1);
    }
    
    WSADATA wsa;
    SOCKET sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    int portno, n;
    int clilen;
    char buffer[255];

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

    // Communication Loop
    while(1){
        memset(buffer, 0, 255);
        
        n = recv(newsockfd, buffer, 255, 0);
        if(n == SOCKET_ERROR){
            printf("Error on reading! %d\n", WSAGetLastError());
            break;
        }
        printf("Client: %s\n", buffer);
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("Client requested to end the chat.\n"); 
            break;            
        }

        memset(buffer, 0, 255);
        fgets(buffer, 255, stdin);

        n = send(newsockfd, buffer, strlen(buffer), 0);
        if(n == SOCKET_ERROR){
            printf("Error on writing! %d\n", WSAGetLastError());
            break;
        }
        if(strncmp("Bye", buffer, 3) == 0){
            printf("Server requested to end the chat.\n");
            break;
        }
    }

    // Close Sockets
    closesocket(newsockfd);
    closesocket(sockfd);
    WSACleanup();

    return 0;
}