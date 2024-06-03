#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

extern void new_message(char *message, char* sender, char* color);
extern char* name;
int sockfd;

void connectToChat(const char *ip, int port, int fd1[2], int fd2[2]) {
    struct sockaddr_in addr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("Error creating socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(myip);

    if(bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0){
        perror("Error binding socket");
        exit(1);
    }

    char recvbuffer[1024];
    char sendbuffer[1024];
    while(1){
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd1[0], &readfds);
        FD_SET(sockfd, &readfds);
        int maxFD = fd1[0] > sockfd ? fd1[0] + 1 : sockfd + 1;
        int sel = select(maxFD, &readfds, NULL, NULL, &tv);
        if(sel == -1){
            perror("Error select");
            exit(1);
        } else if(sel == 0){
            continue;
        } else if(FD_ISSET(sockfd, &readfds)) {
            memset(recvbuffer, 0, sizeof(recvbuffer));
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            if(recvfrom(sockfd, recvbuffer, 1024, 0, (struct sockaddr *) &client_addr, &client_addr_len) > 0){
                char *sender_name = recvbuffer;
                if(strncmp(sender_name, name, sizeof(name)) != 0){
                    write(fd2[1], recvbuffer, sizeof(recvbuffer));
                }
            } else {
                perror("Error reading from socket");
                continue;
            }
        } else if (FD_ISSET(fd1[0], &readfds)) {
            memset(sendbuffer, 0, sizeof(sendbuffer));
            int ret = read(fd1[0], sendbuffer+11, 1024 - 12);
            if(ret == -1){
                perror("Error reading from pipe");
                continue;
            }
            memcpy(sendbuffer, name, 10);
            ret = sendto(sockfd, sendbuffer, sizeof(sendbuffer), 0, (struct sockaddr *) &addr, sizeof(addr));
            if(ret == -1) {
                perror("Error sending to socket");
                continue;
            } else {
                new_message(sendbuffer+11, sendbuffer, KGRN);
            }
        }
    }
}
