#include <pthread.h>
#include <assert.h>
#include <malloc.h>
#include <dirent.h>
#include <stdio.h>

typedef struct __node_file {
    FILE *file;
    struct __node_file *next;
} node_f;
typedef struct __queue_f {
    node_f *head;
    node_f *tail;
    pthread_mutex_t headLock;
    pthread_mutex_t tailLock;
} queue_f;

typedef struct __node_path {
    char *path;
    struct __node_path *next;
} node_p;
typedef struct __queue_p {
    node_p *head;
    node_p *tail;
    pthread_mutex_t headLock;
    pthread_mutex_t tailLock;
} queue_p;

typedef struct __bigram_array {
    int countBigrams['z' - 'a' + 1]['z' - 'a' + 1];
    pthread_mutex_t arrayLock;
} bigramArray;


typedef struct __context {
    queue_f *fileQueue;
    queue_p *pathQueue;
    bigramArray *countBigram;

} context;


void Path_Queue_Init(queue_p *q);

void Path_Queue_Enqueue(queue_p *q, char *path);

int Path_Queue_Dequeue(queue_p *q, char **path);

void File_Queue_Init(queue_f *q);

void File_Queue_Enqueue(queue_f *q, FILE *file);

int File_Queue_Dequeue(queue_f *q, FILE **file);

void addBigram(bigramArray *countBigrams, int c0, int c1);

void *producer(context *ctx);

void *consumer(context *ctx);

void setContext(queue_p *pathQueue, queue_f *fileQueue, context *ctx, bigramArray *countBigram);

void initBigramArray(bigramArray *countBigram);

void printResults(bigramArray countBigram);

int main() {
    queue_p pathQueue;
    Path_Queue_Init(&pathQueue);
    queue_f fileQueue;
    File_Queue_Init(&fileQueue);
    bigramArray countBigram;
    initBigramArray(&countBigram);
    context ctx;
    setContext(&pathQueue, &fileQueue, &ctx, &countBigram);
    struct dirent *en;
    DIR *dr;
    char dir[] = "/Users/edore/CLionProjects/BigramsTrigramsParallel/Gutenberg/txt";
    dr = opendir(dir);
    if (dr) {
        int i = 0;
        while ((en = readdir(dr)) != NULL) {
            if (i > 1) {
                char *path = malloc(sizeof(char) * 200);
                snprintf(path, sizeof(path) * 200,
                         "/Users/edore/CLionProjects/BigramsTrigramsParallel/Gutenberg/txt/%s", en->d_name);
                Path_Queue_Enqueue(&pathQueue, path);
            }
            i++;
        }
    }
    pthread_t thread[4];
    pthread_create(&thread[1], NULL, (void *) producer, &ctx);
    pthread_create(&thread[2], NULL, (void *) producer, &ctx);
    pthread_create(&thread[3], NULL, (void *) consumer, &ctx);
    pthread_create(&thread[4], NULL, (void *) consumer, &ctx);
    pthread_join(thread[1], NULL);
    pthread_join(thread[2], NULL);
    pthread_join(thread[3], NULL);
    pthread_join(thread[4], NULL);
    printResults(countBigram);
    return 0;
}


void *producer(context *ctx) {
    char *path = malloc(sizeof(char) * 200);
    while (Path_Queue_Dequeue(ctx->pathQueue, &path) != -1) {
        FILE *file = fopen(path, "r");
        if (file != NULL) {
            File_Queue_Enqueue(ctx->fileQueue, file);
        }
    }
}

void *consumer(context *ctx) {
    FILE *file;
    while (File_Queue_Dequeue(ctx->fileQueue, &file) != -1) {
        int c0 = EOF, c1;
        while ((c1 = getc(file)) != EOF) {
            if (c1 >= 'a' && c1 <= 'z' && c0 >= 'a' && c0 <= 'z') {
                addBigram(ctx->countBigram, c0, c1);
            }
            c0 = c1;
        }
    }
}

void initBigramArray(bigramArray *countBigram) {
    pthread_mutex_init(&countBigram->arrayLock, NULL);
    int size = 'z' - 'a' + 1;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            countBigram->countBigrams[i][j] = 0;
        }
    }
}

void addBigram(bigramArray *countBigrams, int c0, int c1) {
    pthread_mutex_lock(&countBigrams->arrayLock);
    countBigrams->countBigrams[c0 - 'a'][c1 - 'a']++;
    pthread_mutex_unlock(&countBigrams->arrayLock);
}

void printResults(bigramArray countBigram) {
    int c0, c1;
    for (c0 = 'a'; c0 <= 'z'; c0++) {
        for (c1 = 'a'; c1 <= 'z'; c1++) {
            int n = countBigram.countBigrams[c0 - 'a'][c1 - 'a'];
            if (n) {
                printf("%c%c: %d\n", c0, c1, n);
            }
        }
    }
}

void setContext(queue_p *pathQueue, queue_f *fileQueue, context *ctx, bigramArray *countBigram) {
    ctx->pathQueue = pathQueue;
    ctx->fileQueue = fileQueue;
    ctx->countBigram = countBigram;
}

void Path_Queue_Init(queue_p *q) {
    node_p *tmp = malloc(sizeof(node_p));
    tmp->next = NULL;
    q->head = q->tail = tmp;
    pthread_mutex_init(&q->headLock, NULL);
    pthread_mutex_init(&q->tailLock, NULL);
}

void Path_Queue_Enqueue(queue_p *q, char *path) {
    node_p *tmp = malloc(sizeof(node_p));
    assert(tmp != NULL);
    tmp->path = path;
    tmp->next = NULL;
    pthread_mutex_lock(&q->tailLock);
    q->tail->next = tmp;
    q->tail = tmp;
    pthread_mutex_unlock(&q->tailLock);
}

int Path_Queue_Dequeue(queue_p *q, char **path) {
    pthread_mutex_lock(&q->headLock);
    node_p *tmp = q->head;
    node_p *newHead = tmp->next;
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

void File_Queue_Init(queue_f *q) {
    node_f *tmp = malloc(sizeof(node_f));
    tmp->next = NULL;
    q->head = q->tail = tmp;
    pthread_mutex_init(&q->headLock, NULL);
    pthread_mutex_init(&q->tailLock, NULL);
}

void File_Queue_Enqueue(queue_f *q, FILE *file) {
    node_f *tmp = malloc(sizeof(node_f));
    assert(tmp != NULL);
    tmp->file = file;
    tmp->next = NULL;
    pthread_mutex_lock(&q->tailLock);
    q->tail->next = tmp;
    q->tail = tmp;
    pthread_mutex_unlock(&q->tailLock);
}

int File_Queue_Dequeue(queue_f *q, FILE **file) {
    pthread_mutex_lock(&q->headLock);
    node_f *tmp = q->head;
    node_f *newHead = tmp->next;
    if (newHead == NULL) {
        pthread_mutex_unlock(&q->headLock);
        return -1; // queue was empty
    }
    *file = newHead->file;
    q->head = newHead;
    pthread_mutex_unlock(&q->headLock);
    free(tmp);
    return 0;
}