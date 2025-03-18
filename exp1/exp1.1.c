#include <stdio.h>
#include <unistd.h>

int main(){
    int p1;
    p1 = fork();
    if (p1 == 0) printf("I am child, pid: %d\n", getpid());
    else printf("I am parent, pid: %d, child pid: %d\n", getpid(), p1);
}