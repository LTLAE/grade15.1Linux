#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

sem_t semaphore;
int sharedData = 0;

void *thread1(void *arg){
    while(1){
        sem_wait(&semaphore);   // lock data
        if (sharedData >= 5) // exit condition
        {
            printf("Thread 1: sharedData reached 5, exiting...\n");
            sem_post(&semaphore);   //release data and exit
            pthread_exit(NULL);
        }
        printf("Thread 1 reading shared data.\n");
        printf("Thread 1: sharedData %d -> %d\n", sharedData, sharedData + 1);
        sharedData++;
        sleep (1);
        sem_post(&semaphore);   //release data
    }
}

void *thread2(void *arg){
    while(1){
        sem_wait(&semaphore);   // lock data
        if (sharedData >= 5) // exit condition
        {
            printf("Thread 2: sharedData reached 5, exiting...\n");
            sem_post(&semaphore);   //release data and exit
            pthread_exit(NULL);
        }
        printf("Thread 2 reading shared data.\n");
        printf("Thread 2: sharedData %d -> %d\n", sharedData, sharedData + 1);
        sharedData++;
        sleep (1);
        sem_post(&semaphore);   // release data
    }
}

int main(){
    sem_init(&semaphore, 0, 1); // init semaphore to 1 (available)
    printf("Main: creating thread 1 and 2.\n");
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_join(t1, NULL);
    printf("Thread 1 finished.\n");
    pthread_join(t2, NULL);
    printf("Thread 2 finished.\n");
    sem_destroy(&semaphore);
    printf("Main: semaphore destroyed.\n");
    printf("sharedData: %d\n", sharedData);
}