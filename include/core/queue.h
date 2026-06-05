#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>

#define QUEUE_SUCCESS 1
#define QUEUE_ERROR 0

typedef struct QueueNode    
{
    void* data;
    struct QueueNode* next;
}QueueNode;

typedef struct{
    QueueNode* head;
    QueueNode* tail;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int size;
    bool is_shutdown;
}Queue;

Queue* create_queue();
int queue_push(Queue* q, void* data);
void* queue_pop(Queue* q);
void queue_shutdown(Queue* q); 
void destroy_queue(Queue* q);  




#endif