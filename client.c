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

#pragma comment(lib, "ws2_32.lib") // Link with winsock library tells the linker to link the winsock library.

#define RESET   "\x1b[0m"
#define BLUE    "\x1b[34m"
#define WIDTH 60

SOCKET sockfd;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

void error(const char *msg) {
    fprintf(stderr, "%s. Error code: %d\n", msg, WSAGetLastError());
    exit(1);
}

/* --------------- Send Messages --------------- */
void chat_loop(void) {
    while (1) {   // ===== OUTER LOOP =====
        char mode_line[8];
        char mode;

        printf("\nChoose option:\n");
        printf("p - Private chat\n");
        printf("b - Broadcast\n");
        printf("q - Quit current chat\n");
        printf("e - Exit client\n");
        printf("Choice: ");
        fflush(stdout);

        if (!fgets(mode_line, sizeof(mode_line), stdin))
            continue;

        mode = mode_line[0];

        /* ----- EXIT CLIENT ----- */
        if (mode == 'e' || mode == 'E') {
            printf("Exiting client...\n");
            closesocket(sockfd);
            WSACleanup();
            exit(0);
        }

        /* ----- QUIT CURRENT CHAT ----- */
        if (mode == 'q' || mode == 'Q') {
            printf("No active chat. Choose again.\n");
            continue;
        }

        /* ----- INVALID OPTION ----- */
        if (mode != 'p' && mode != 'P' &&
            mode != 'b' && mode != 'B') {
            printf("Invalid choice.\n");
            continue;
        }

        /* ----- PRIVATE CHAT: ask username once ----- */
        char target[32] = {0};
        if (mode == 'p' || mode == 'P') {
            printf("Send to (username): ");
            fflush(stdout);
            fgets(target, sizeof(target), stdin);
            target[strcspn(target, "\n")] = 0;
        }

        printf("Type messages (q = quit this chat, e = exit client)\n");

        while (1) {   // ===== INNER LOOP =====
            char msg[1024];
            char packet[1200];

            fflush(stdout);

            if (!fgets(msg, sizeof(msg), stdin))
                break;

            msg[strcspn(msg, "\n")] = 0;

            /* ---- EXIT CLIENT ---- */
            if (strcmp(msg, "e") == 0) {
                printf("Exiting client...\n");
                closesocket(sockfd);
                WSACleanup();
                exit(0);
            }

            /* ---- QUIT CURRENT CHAT ---- */
            if (strcmp(msg, "q") == 0) {
                printf("Leaving current chat...\n");
                break;   // ⬅ back to OUTER loop
            }

            /* ---- SEND MESSAGE ---- */
            if (mode == 'p' || mode == 'P') {
                snprintf(packet, sizeof(packet),
                         "@p %s %s", target, msg);
            } else {
                snprintf(packet, sizeof(packet),
                         "@b %s", msg);
            }

            send(sockfd, packet, strlen(packet), 0);
        }
    }
}

/* --------------- Receive Messages ---------------- */
void *receiveMessages(void *args){
    char buffer[1024];

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if (n <= 0) {
            pthread_mutex_lock(&print_lock);
            printf("\nServer disconnected.\n");
            pthread_mutex_unlock(&print_lock);
            closesocket(sockfd);
            WSACleanup();
            pthread_exit(NULL);   // <-- terminate ONLY this thread
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

    // Create a receive thread
    pthread_t recvThread;
    pthread_create(&recvThread, NULL, receiveMessages, NULL);

    chat_loop();

    closesocket(sockfd);
    WSACleanup();

    return 0;
}