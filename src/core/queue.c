#include "queue.h"

#include <stdlib.h>


Queue* create_queue(){
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if(!q) return NULL;
    
    q->tail = NULL;
    q->head = NULL;
    q->size = 0;
    q->is_shutdown = false;

    if (pthread_mutex_init(&q->lock, NULL) != 0){
        free(q);
        return NULL;
    }

    if (pthread_cond_init(&q->cond, NULL) != 0) {
        pthread_mutex_destroy(&q->lock);
        free(q);
        return NULL;
    }

    return q;
}   

int queue_push(Queue* q, void* data){
    if(!q || !data) return QUEUE_ERROR;

    QueueNode* qn = (QueueNode*)malloc(sizeof(QueueNode));
    if(!qn) return QUEUE_ERROR;

    qn->data = data;
    qn->next = NULL;

    pthread_mutex_lock(&q->lock);

    // closed queue
    if (q->is_shutdown) {
        pthread_mutex_unlock(&q->lock);
        free(qn);
        return QUEUE_ERROR; 
    }

    // queue is empty
    if(!q->head){
        q->head = qn;
        q->tail = qn;
    }
    else{
        q->tail->next = qn;
        q->tail = qn;
    }
    
    q->size++;

    pthread_cond_signal(&q->cond);

    pthread_mutex_unlock(&q->lock);

    
    return QUEUE_SUCCESS;
}


void* queue_pop(Queue* q){
    if(!q) return NULL;

    pthread_mutex_lock(&q->lock);

    while (q->size == 0 && !q->is_shutdown) {
        pthread_cond_wait(&q->cond, &q->lock);
    }

    if (q->is_shutdown && q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL; 
    }

    QueueNode* qn = q->head;
    void* data = qn->data;

    q->head = q->head->next;
    
    if (q->head == NULL) {
        q->tail = NULL;
    }

    q->size--;

    free(qn);
    pthread_mutex_unlock(&q->lock);

    return data;    
}

void queue_shutdown(Queue* q) {
    pthread_mutex_lock(&q->lock);
    q->is_shutdown = true; 
    pthread_cond_broadcast(&q->cond); 
    pthread_mutex_unlock(&q->lock);
}

void destroy_queue(Queue* q){
    if(!q) return;

    pthread_mutex_lock(&q->lock);

    QueueNode* current = q->head;
    while(current){
        QueueNode* qn = current->next;
        // if (current->data) free(current->data);

        free(current);
        current = qn;
    }

    pthread_mutex_unlock(&q->lock);

    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);

    free(q);
}
