#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int ready = 0;

static void *produce(void *argument)
{
    (void)argument;

    for (;;) {
        sleep(1);
        pthread_mutex_lock(&mutex);
        if (ready == 1) {
            pthread_mutex_unlock(&mutex);
            continue;
        }
        ready = 1;
        printf("provided\n");
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

static void *consume(void *argument)
{
    (void)argument;

    for (;;) {
        pthread_mutex_lock(&mutex);
        while (ready == 0) {
            pthread_cond_wait(&condition, &mutex);
        }
        ready = 0;
        printf("consumed\n");
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void)
{
    pthread_t producer;
    pthread_t consumer;

    setvbuf(stdout, NULL, _IOLBF, 0);
    if (pthread_create(&producer, NULL, produce, NULL) != 0 ||
        pthread_create(&consumer, NULL, consume, NULL) != 0) {
        fprintf(stderr, "Не удалось создать потоки\n");
        return 1;
    }

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    return 0;
}
