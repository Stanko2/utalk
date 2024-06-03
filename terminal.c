
#include <termios.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"



void print_colored(char *color, char *text){
    printf("%s%s%s", color, text, KNRM);
}

void set_terminal(){
    struct termios term;
    tcgetattr(0, &term);

    term.c_lflag &= ~ECHO;
    term.c_lflag &= ~ICANON;
    // term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    setvbuf(stdout, NULL, _IONBF, (size_t) 0);
    tcsetattr(0, TCSANOW, &term);
    setvbuf(stdin, NULL, _IONBF, 0);
    // printf("\e[?25l");
}

void toggle_echo(int enable){
    struct termios term;
    tcgetattr(0, &term);

    if(enable){
        term.c_lflag |= ECHO;
    } else {
        term.c_lflag &= ~ECHO;
    }
    tcsetattr(0, TCSANOW, &term);
}