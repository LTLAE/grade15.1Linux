#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int p1,p2;

int main(){
    int fd[2];
    char outpipe[100],inpipe[100];
    pipe(fd);   //create a pipe

    p1 = fork();
    if(p1 == 0) {
        lockf(fd[1],1,0);
        sprintf(outpipe,"child 1 process is sending message!"); // write message to outpipe array
        write(fd[1],outpipe,50);     // write message to pipe
        lockf(fd[1],0,0);
        exit(0);
    } else {
        p2 = fork();
        if(p2 == 0) {
            lockf(fd[1],1,0);
            sprintf(outpipe,"child 2 process is sending message!"); // write message to outpipe array
            write(fd[1],outpipe,50);    // write message to pipe
            lockf(fd[1],0,0);
            exit(0);
        } else {
            wait(0);    // Main: wait for child 1
            read(fd[0],inpipe,50);   // Main: read message from pipe
            printf("%s\n",inpipe);
            wait(0);    // Main: wait for child 2
            read(fd[0],inpipe,50);
            printf("%s\n",inpipe);
            exit(0);

        }
    }
}