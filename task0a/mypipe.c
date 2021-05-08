#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

int main(int argc, char **argv) {
    int n;
    int pid, p[2];
    char buff[6];

    if (pipe(p) < 0) //eror in pipe, pipe returns -1
        exit(1); 

    pid = fork();
    if (pid > 0) { //child case
        write(p[1], "Hello", 6);
        close(p[1]);
        _exit(0);
    }
    else {
        while ((n = read(p[0], buff, 6)) > 0)
            printf("%s\n", buff);
        if (n == -1) exit(2);
    }   
    return 0;
}