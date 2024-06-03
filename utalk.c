
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>

#include "terminal.c"
#include "chat.c"

#define MSGSTR "Enter a message: "

char buffer[1024];
char new_buffer[1024];

void new_message(char *message, char* sender, char* sender_color);
void read_message();
char* get_name();
int fd1[2];
int fd2[2];
char* name;

void intHandler() {
    // revert terminal settings
    struct termios Otty, Ntty;
    tcgetattr(0, &Otty);
    Ntty = Otty;
    Ntty.c_iflag = 0x6b02;
    Ntty.c_oflag = 0x3;
    Ntty.c_cflag = 0x4b00;
    Ntty.c_lflag = 0x200005cb;

    tcsetattr(0, TCSAFLUSH, &Ntty);
    free(name);

    if(shutdown(sockfd, SHUT_RDWR) == -1){
        perror("error shutting down socket");
    }
    close(sockfd);

    exit(0);
}

int main(int argc, char const *argv[])
{
    if(argc < 2){
        printf("Usage: %s <ip> <myip> [port]\n", argv[0]);
        return 1;
    }
    set_terminal();
    name = get_name();
    printf("\033[A\33[2KT\r");
    printf("Your name is: %s\n", name);
    print_colored(KNRM, "Enter a message:");
    const char *ip = argv[1];
    int port = 1234;
    if(argc >= 3){
        port = atoi(argv[2]);
    }

    int pipe_result = pipe(fd1);
    if(pipe_result == -1){
        perror("error creating pipe");
        return 1;
    }

    pipe_result = pipe(fd2);
    if(pipe_result == -1){
        perror("error creating pipe");
        return 1;
    }

    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        return 1;
    }

    if(pid == 0){
        close(fd1[0]);
        close(fd2[1]);
        while (1) {
            read_message();
        }
    } else {
        close(fd1[1]);
        close(fd2[0]);
        connectToChat(ip, port, fd1, fd2);
    }
        
    signal(SIGINT, intHandler);
    return 0;
}

void new_message(char *message, char* sender, char* sender_color){
    print_colored(sender_color, sender);
    printf(": ");
    print_colored(KYEL, message);
    print_colored(KNRM, MSGSTR);
}

char* get_name() {
    toggle_echo(1);
    name = malloc(11);
    memset(name, 0, sizeof(name));
    print_colored(KNRM, "Enter your name: ");
    int wrong = 0;
    while(1) {
        scanf("%s", name);
        if(strlen(name) > 10){
            print_colored(KRED, "Name cannot be longer than 10 characters\n");
            print_colored(KNRM, "Enter your name: ");
            memset(name, 0, sizeof(name));
            continue;
        } else {
            break;
        }
    }
    toggle_echo(0);
    return name;
}


void read_message() {
    char x;
    int i = 0;
    memset(buffer, 0, sizeof(buffer));
    struct winsize w;
    ioctl(1, TIOCGWINSZ, &w);
    short rows = 1;
    int len_total = strlen(MSGSTR);

    while (1)
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(fd2[0], &rfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        int ret = select(fd2[0] + 1, &rfds, NULL, NULL, &tv);
        if(ret == -1){
            perror("error select");
            return;
        } else if(ret == 0){
            continue;
        }
        if(FD_ISSET(0, &rfds)){
            read(0, &x, 1);
            buffer[i] = x;
            i++;
            if(i >= sizeof(buffer)){
                break;
            }
            len_total++;

            if(len_total == w.ws_col){
                rows++;
                len_total = 0;
            }
            if(x!= 65 && x != 66){
                putchar(x);
            }
            
            if(x == '\n'){
                break;
            }
        } else if(FD_ISSET(fd2[0], &rfds)){
            memset(new_buffer, 0, sizeof(new_buffer));
            int ret = read(fd2[0], new_buffer, sizeof(new_buffer));
            if(ret == -1){
                perror("error reading from pipe");
                return;
            }
            if(ret == 0){
                continue;
            }
            for (short j = 0; j < rows - 1; j++)
            {
                printf("\033[A\33[2KT\r");
            }
            printf("\33[2K\r");
            new_message(new_buffer+11, new_buffer, KBLU);
            print_colored(KNRM, buffer);
        }
    }

    for (short j = 0; j < rows; j++)
    {
        printf("\033[A\33[2KT\r");
    }
    if(i >= sizeof(buffer)){
        // print_colored(KRED, "Message too long, cannot send\n");
        return;
    }
    int ret = write(fd1[1], buffer, strlen(buffer));
    if(ret == -1){
        perror("error writing to pipe");
    }
}