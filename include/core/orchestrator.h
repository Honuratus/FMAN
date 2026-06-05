#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

#include "queue.h"
#include "models.h"

#define ORCHESTRATOR_SUCCESS 1
#define ORCHESTRATOR_ERROR 0

typedef enum{
	DB,
	WORKER
}MessageType;

typedef void (*DbCallback)(void* result, void* context);

typedef enum{
	DB_TASK_SAVE_COLLECTION,
    DB_TASK_SAVE_REQUEST,
    DB_TASK_SAVE_RESPONSE,
    DB_TASK_UPDATE_STATUS,
	DB_TASK_GET_NEXT_REQUEST,
    DB_TASK_SHUTDOWN // Worker'ı güvenli bir şekilde kapatmak için
}DbOpType;

typedef struct{
	DbOpType type;
	void* data;
	DbCallback callback;
	void* context;
}DBTask;



typedef struct {
    int request_id;
    int new_status_code;
} UpdateStatusPayload;




typedef struct{
	Queue* db_queue;
	Queue* request_queue;
	Queue* response_queue; 


	pthread_t* http_worker_threads;
    int http_worker_count;
    
    pthread_t db_worker_thread;
	pthread_t dispatcher_thread;

	bool is_running;
	bool db_worker_started;
	bool dispatcher_started;

}Runtime;


Runtime* create_runtime(int num_http_worker);
void destroy_runtime(Runtime* r);

void* dispatcher_routine(void* arg);

int dispatch_task(Runtime* o, void* data, MessageType type);

Response* wait_for_response(int request_id, int timeout_ms);

#endif
