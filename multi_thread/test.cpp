#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>
#include<malloc.h>
#include<unistd.h>

int K = 10000, N = 100;
int sum = 0;

pthread_mutex_t* pmutex = NULL;

void* MyFunc(void* params) {
    int sleep_d = 100 * (rand() % 1000);
    usleep(sleep_d);
    printf("Hello MyFunc! %ld-%d-%lld\n", pthread_self(), *(int*)params, sleep_d);
    int i = *((int*)params);
    free(params);
    for(int j = i * K / N; j < (i + 1) * K / N; j++) {
        pthread_mutex_lock(pmutex);
        sum += j;
        pthread_mutex_unlock(pmutex);
    }

    return NULL;
}

int main() {
    pmutex = (pthread_mutex_t*) calloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(pmutex, NULL);
    pthread_t* pid = (pthread_t*)calloc(N, sizeof(pthread_t));
    for (int i = 0; i < N; i++) {
        int* param = (int*)calloc(1, sizeof(int));
        *param = i;
        pthread_create(&pid[i], NULL, MyFunc, (void*)param);
    }
    for(int i = 0; i < N; i++) {
        pthread_join(pid[i], NULL);
    }

    printf("Hello Main %ld\n", pthread_self);
    
    printf("SUM=%lld\n", sum);

    pthread_mutex_destroy(pmutex);
}