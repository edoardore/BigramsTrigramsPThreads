#include <pthread.h>
#include <assert.h>
#include <malloc.h>
#include <dirent.h>
#include <stdio.h>

typedef struct __node_t {
    char *path;
    struct __node_t *next;
} node_t;
typedef struct __queue_t {
    node_t *head;
    node_t *tail;
    pthread_mutex_t headLock;
    pthread_mutex_t tailLock;
} queue_t;

void Queue_Init(queue_t *q);

void Queue_Enqueue(queue_t *q, char *path);

int Queue_Dequeue(queue_t *q, char **path);

void main() {
    struct __queue_t *queue;
    Queue_Init(queue);
    struct dirent *en;
    DIR *dr;
    char dir[] = "/Users/edore/CLionProjects/BigramsTrigramsParallel/Gutenberg/txt";
    dr = opendir(dir);
    if (dr) {
        int i = 0;
        while ((en = readdir(dr)) != NULL) {
            if (i > 1) {
                char *path = malloc(sizeof(char) * 200);
                snprintf(path, sizeof(path) * 200, "/Users/edore/CLionProjects/BigramsTrigrams/Gutenberg/txt/%s",
                         en->d_name);
                Queue_Enqueue(queue, path);
            }
            i++;
        }
    }
}


void Queue_Init(queue_t *q) {
    node_t *tmp = malloc(sizeof(node_t));
    tmp->next = NULL;
    q->head = q->tail = tmp;
    pthread_mutex_init(&q->headLock, NULL);
    pthread_mutex_init(&q->tailLock, NULL);
}

void Queue_Enqueue(queue_t *q, char *path) {
    node_t *tmp = malloc(sizeof(node_t));
    assert(tmp != NULL);
    tmp->path = path;
    tmp->next = NULL;
    pthread_mutex_lock(&q->tailLock);
    q->tail->next = tmp;
    q->tail = tmp;
    pthread_mutex_unlock(&q->tailLock);
}

int Queue_Dequeue(queue_t *q, char **path) {
    pthread_mutex_lock(&q->headLock);
    node_t *tmp = q->head;
    node_t *newHead = tmp->next;
    if (newHead == NULL) {
        pthread_mutex_unlock(&q->headLock);
        return -1; // queue was empty
    }
    *path = newHead->path;
    q->head = newHead;
    pthread_mutex_unlock(&q->headLock);
    free(tmp);
    return 0;
}