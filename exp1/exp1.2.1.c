#include <stdio.h>
#include <unistd.h>

int main(){
    int p1;
    p1 = fork();
    if (p1 == 0)
    {
        printf("I am the child. Calling exp1.2.2\n");
        execlp("./exp1.2.2", NULL);
    } else
    {
        printf("I am the parent. My child is %d\n", p1);
    }
}