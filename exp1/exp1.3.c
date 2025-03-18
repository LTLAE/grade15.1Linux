#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* ThreadFunction(void* arg){
    for (int timer = 0; timer < 10; timer++){
        printf("ThreadFunction: %d\n", timer);
        sleep(1);
    }
    printf("ThreadFunction: Done. Exiting thread.\n");
    pthread_exit(NULL);
}

int main(){
    pthread_t thread;
    if (pthread_create(&thread, NULL, ThreadFunction, NULL)) {
        printf("Error creating thread\n");
        return 1;
    }
    for (int timer = 0; timer < 5; timer++){
        printf("Main: %d\n", timer);
        sleep(1);
    }
    printf("Main: Waiting for thread to finish\n");
    pthread_join(thread, NULL);
    printf("Main: Thread has finished\n");
}