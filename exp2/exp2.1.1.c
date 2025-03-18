#include <stdio.h>
#include <unistd.h>

int main(){
    printf("I am the exp2.1.1. Calling exp2.1.2\n");
    execlp("./exp2.1.2", NULL);
}