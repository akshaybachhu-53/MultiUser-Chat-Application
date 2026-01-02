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
#include<time.h>

#pragma comment(lib, "ws2_32.lib") // Link with winsock library tells the linker to link the winsock library.

#define RESET   "\x1b[0m"
#define BLUE    "\x1b[34m"
#define WIDTH 60

SOCKET sockfd;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char mode;
    char target[32];
} ChatInfo;


void error(const char *msg) {
    fprintf(stderr, "%s. Error code: %d\n", msg, WSAGetLastError());
    exit(1);
}

/* --------------- Send Messages --------------- */
void *sendMessages(void* args){
    ChatInfo *info = (ChatInfo *)args;
    char chat_mode = info->mode;
    char target[32];
    strcpy(target, info->target);

    char buffer[1024];
    char message[1100];

    while (1) {

        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        // Prepare message based on mode
        if (chat_mode == 'p' || chat_mode == 'P') {
            snprintf(message, sizeof(message), "@p %s %s", target, buffer);
        } else if (chat_mode == 'b' || chat_mode == 'B') {
            snprintf(message, sizeof(message), "@b %s", buffer);
        } else {
            continue;
        }

        send(sockfd, message, strlen(message), 0);

        if (strcmp(buffer, "Bye") == 0) {
            pthread_mutex_lock(&print_lock);
            printf("Exiting chat...\n");
            pthread_mutex_unlock(&print_lock);

            closesocket(sockfd);
            WSACleanup();
            exit(0);
        }
    }
    free(info);
    return NULL;
}

/* --------------- Receive Messages ---------------- */
void *receiveMessages(void *args){
    char buffer[1024];

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if(n <= 0){
            pthread_mutex_lock(&print_lock);
            printf("\nServer disconnected.\n");
            pthread_mutex_unlock(&print_lock);
            exit(0);
        }

        buffer[n] = '\0';
        pthread_mutex_lock(&print_lock);
        printf("\n%*s" BLUE "%s" RESET "\n", 50, "", buffer);
        fflush(stdout);
        pthread_mutex_unlock(&print_lock);
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

    char username[32];

    printf("Enter Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    // Send username to server
    char intro[64];
    snprintf(intro, sizeof(intro), "@u %s", username);
    send(sockfd, intro, strlen(intro), 0);

    char chat_mode;
    printf("Choose mode:\n");
    printf("p - Private Chat\n");
    printf("b - Broadcast to all\n");
    printf("Choice: ");
    scanf(" %c", &chat_mode);
    getchar();

    char target_user[32] = {0};

    if (chat_mode == 'p' || chat_mode == 'P') {
        printf("Enter username to chat with: ");
        fgets(target_user, sizeof(target_user), stdin);
        target_user[strcspn(target_user, "\n")] = 0;
    }

    // Create two threads
    pthread_t sendThread, recvThread;

    ChatInfo *info = malloc(sizeof(ChatInfo));
    info->mode = chat_mode;
    strcpy(info->target, target_user);

    pthread_create(&sendThread, NULL, sendMessages, info);
    pthread_create(&recvThread, NULL, receiveMessages, NULL);
    
    pthread_join(sendThread, NULL);
    pthread_join(recvThread, NULL);

    closesocket(sockfd);
    WSACleanup();

    return 0;
}