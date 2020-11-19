#include <pthread.h>
#include <assert.h>
#include <malloc.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdatomic.h>


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
    int countBigram['z' - 'a' + 1]['z' - 'a' + 1];
    pthread_mutex_t arrayLock;
} bigramArray;

typedef struct __trigram_array {
    int countTrigram['z' - 'a' + 1]['z' - 'a' + 1]['z' - 'a' + 1];
    pthread_mutex_t arrayLock;
} trigramArray;


typedef struct __context {
    queue_f *fileQueue;
    queue_p *pathQueue;
    bigramArray *countBigram;
    trigramArray *countTrigram;
    atomic_int nProducer;
    atomic_int nQueue;

} context;


void Path_Queue_Init(queue_p *q);

void Path_Queue_Enqueue(queue_p *q, char *path);

int Path_Queue_Dequeue(queue_p *q, char **path);

void File_Queue_Init(queue_f *q);

void File_Queue_Enqueue(queue_f *q, FILE *file);

int File_Queue_Dequeue(queue_f *q, FILE **file);

void addBigram(bigramArray *countBigram, int c0, int c1);

void *producer(context *ctx);

void *consumer(context *ctx);

int64_t currentTimeMillis();

void setContext(queue_p *pathQueue, queue_f *fileQueue, context *ctx, bigramArray *countBigram,
                trigramArray *countTrigram, atomic_int *nProducer, atomic_int *nQueue);

void initBigramArray(bigramArray *countBigram);

void printResults(bigramArray countBigram, trigramArray countTrigram);

void initTrigramArray(trigramArray *countTrigram);

void addTrigram(trigramArray *countTrigram, int t0, int t1, int t2);


int main() {
    int nProd = 3;
    int nCons = 2;
    atomic_int nProducer = nProd;
    atomic_int nQueue = 0;
    int64_t start = currentTimeMillis();
    queue_p pathQueue;
    Path_Queue_Init(&pathQueue);
    queue_f fileQueue;
    File_Queue_Init(&fileQueue);
    bigramArray countBigram;
    initBigramArray(&countBigram);
    trigramArray countTrigram;
    initTrigramArray(&countTrigram);
    context ctx;
    setContext(&pathQueue, &fileQueue, &ctx, &countBigram, &countTrigram, &nProducer, &nQueue);
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
    pthread_t thread[nProd + nCons];
    for (int n = 0; n < nProd; n++) {
        pthread_create(&thread[n], NULL, (void *) producer, &ctx);
    }
    for (int n = 0; n < nCons; n++) {
        pthread_create(&thread[n], NULL, (void *) consumer, &ctx);
    }
    for (int n = 0; n < nProd + nCons; n++) {
        pthread_join(thread[n], NULL);
    }
    int64_t time = currentTimeMillis() - start;
    printResults(countBigram, countTrigram);
    printf("Tempo totale di esecuzione: ms ""%"PRId64"\n", time);
    return 0;
}


int64_t currentTimeMillis() {
    struct timeval time;
    gettimeofday(&time, NULL);
    int64_t s1 = (int64_t) (time.tv_sec) * 1000;
    int64_t s2 = (time.tv_usec / 1000);
    return s1 + s2;
}

void *producer(context *ctx) {
    char *path = malloc(sizeof(char) * 200);
    while (Path_Queue_Dequeue(ctx->pathQueue, &path) != -1) {
        FILE *file = fopen(path, "r");
        if (file != NULL) {
            File_Queue_Enqueue(ctx->fileQueue, file);
            ++ctx->nQueue;
        }
    }
    --ctx->nProducer;
}

void *consumer(context *ctx) {
    FILE *file;
    while (1) {
        if (ctx->nProducer == 0 && ctx->nQueue == 0)
            break;
        if (File_Queue_Dequeue(ctx->fileQueue, &file) == 0) {
            --ctx->nQueue;
            int c0 = EOF, c1;
            int t0 = EOF, t1, t2;
            while ((c1 = getc(file)) != EOF) {
                t2 = c1;
                if (c1 >= 'a' && c1 <= 'z' && c0 >= 'a' && c0 <= 'z') {
                    addBigram(ctx->countBigram, c0, c1);
                }
                c0 = c1;
                if (t2 >= 'a' && t2 <= 'z' && t1 >= 'a' && t1 <= 'z' && t0 >= 'a' && t0 <= 'z') {
                    addTrigram(ctx->countTrigram, t0, t1, t2);
                }
                t0 = t1;
                t1 = t2;
            }
        }
    }
}


void initBigramArray(bigramArray *countBigram) {
    pthread_mutex_init(&countBigram->arrayLock, NULL);
    int size = 'z' - 'a' + 1;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            countBigram->countBigram[i][j] = 0;
        }
    }
}

void initTrigramArray(trigramArray *countTrigram) {
    pthread_mutex_init(&countTrigram->arrayLock, NULL);
    int size = 'z' - 'a' + 1;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < size; k++) {
                countTrigram->countTrigram[i][j][k] = 0;
            }
        }
    }
}

void addBigram(bigramArray *countBigram, int c0, int c1) {
    pthread_mutex_lock(&countBigram->arrayLock);
    countBigram->countBigram[c0 - 'a'][c1 - 'a']++;
    pthread_mutex_unlock(&countBigram->arrayLock);
}

void addTrigram(trigramArray *countTrigram, int t0, int t1, int t2) {
    pthread_mutex_lock(&countTrigram->arrayLock);
    countTrigram->countTrigram[t0 - 'a'][t1 - 'a'][t2 - 'a']++;
    pthread_mutex_unlock(&countTrigram->arrayLock);
}


void printResults(bigramArray countBigram, trigramArray countTrigram) {
    int c0, c1, c2;
    printf("Bigrammi:\n");
    for (c0 = 'a'; c0 <= 'z'; c0++) {
        for (c1 = 'a'; c1 <= 'z'; c1++) {
            int n = countBigram.countBigram[c0 - 'a'][c1 - 'a'];
            if (n) {
                printf("%c%c: %d\n", c0, c1, n);
            }
        }
    }
    printf("Trigrammi: \n");
    for (c0 = 'a'; c0 <= 'z'; c0++) {
        for (c1 = 'a'; c1 <= 'z'; c1++) {
            for (c2 = 'a'; c2 <= 'z'; c2++) {
                int n = countTrigram.countTrigram[c0 - 'a'][c1 - 'a'][c2 - 'a'];
                if (n) {
                    printf("%c%c%c: %d\n", c0, c1, c2, n);
                }
            }
        }
    }
}

void setContext(queue_p *pathQueue, queue_f *fileQueue, context *ctx, bigramArray *countBigram,
                trigramArray *countTrigram,
                atomic_int *nProducer, atomic_int *nQueue) {
    ctx->pathQueue = pathQueue;
    ctx->fileQueue = fileQueue;
    ctx->countBigram = countBigram;
    ctx->countTrigram = countTrigram;
    ctx->nProducer = *nProducer;
    ctx->nQueue = *nQueue;
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