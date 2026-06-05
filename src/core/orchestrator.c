#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "orchestrator.h"
#include "queue.h"
#include "worker.h"
#include "db_manager.h"

Runtime* create_runtime(int num_http_worker){
	if(num_http_worker <= 0) return NULL;

	Runtime* o = calloc(1, sizeof(Runtime));
	if(!o) return NULL;

	o->db_queue = create_queue();
	if(!o->db_queue)
		goto cleanup;

	o->request_queue = create_queue();
	if(!o->request_queue)
		goto cleanup;
	
	
	o->response_queue = create_queue();
	if(!o->response_queue)
		goto cleanup;
	

	
	
	o->http_worker_threads = (pthread_t*)malloc(sizeof(pthread_t) * num_http_worker);
	if(!o->http_worker_threads)
		goto cleanup;


	int rc = pthread_create(&o->db_worker_thread, NULL, db_worker_routine, o);
	if(rc != 0){
		goto cleanup;
	}
	o->db_worker_started = true;


	rc = pthread_create(&o->dispatcher_thread, NULL, dispatcher_routine, o);
	if(rc != 0){
		goto cleanup;
	}

	o->dispatcher_started = true;


	for(int i = 0; i<num_http_worker; i++){
		rc = pthread_create(&o->http_worker_threads[i], NULL, http_worker_routine, o);
		if(rc != 0)
			goto cleanup;
		
		o->http_worker_count++;
	}

	o->is_running = true;

	return o;


cleanup:
	destroy_runtime(o);
	return NULL;

}




// DB -> data must be DBTask*
// WORKER -> data must be WorkerTask*
int dispatch_task(Runtime* o, void* data, MessageType type){
	if(!o || !data){
		return ORCHESTRATOR_ERROR;
	}


	switch (type) {
		case DB:
			if(queue_push(o->db_queue, data) == QUEUE_ERROR)
				return ORCHESTRATOR_ERROR;
			break;
		case WORKER:
			if(queue_push(o->request_queue, data) == QUEUE_ERROR)
				return ORCHESTRATOR_ERROR;
			break;
		default:
			return ORCHESTRATOR_ERROR; 
	}
	return ORCHESTRATOR_SUCCESS; // data ownership queue'da
	// if fail caller has ownership
}


// it can be called on just runtime owner thread
void destroy_runtime(Runtime* o){
	if(!o) return;

	if(o->request_queue)
		queue_shutdown(o->request_queue);

	
	for(int i = 0; i<o->http_worker_count; i++){
		pthread_join(o->http_worker_threads[i], NULL);
	}


	if(o->response_queue)
		queue_shutdown(o->response_queue);

	if(o->dispatcher_started)
		pthread_join(o->dispatcher_thread, NULL);

	
	if(o->db_queue)
		queue_shutdown(o->db_queue);

	if(o->db_worker_started)
    	pthread_join(o->db_worker_thread, NULL);
	


	destroy_queue(o->request_queue);
    destroy_queue(o->response_queue);
    destroy_queue(o->db_queue);

	free(o->http_worker_threads);
	free(o);
}


void* dispatcher_routine(void* arg) {
	Runtime* o = (Runtime*)arg;

    if (!o) return NULL;

    printf("[Orchestrator] Döngü başladı, trafik yönetiliyor...\n");

    
    while (1) {
        Response* resp = (Response*)queue_pop(o->response_queue);

		if(!resp) break; // if response NULL shutdown signal zero work break 


		
		printf("[Orchestrator] HTTP Worker bir isteği tamamladı! Status: %d\n", resp->status_code); 
		
		

		DBTask* db_task = calloc(1, sizeof(DBTask));		
		if(!db_task) { // now just free the response
			free_response(resp);
			continue;
		}

		db_task->type = DB_TASK_SAVE_RESPONSE;
		db_task->data = resp;
		db_task->callback = NULL; // just save it
		
		int rc = queue_push(o->db_queue, db_task);
		if(rc == QUEUE_ERROR){
			free_response(resp);
			free(db_task);
			continue;
		}

		// owenership transger
		resp = NULL;
		db_task = NULL;
    }

	return NULL;
}


Response* wait_for_response(int request_id, int timeout_ms)
{
    int elapsed = 0;

    while (elapsed < timeout_ms) {
        Response* resp = db_get_latest_response_by_request_id(request_id);
        if (resp)
            return resp;

        usleep(100 * 1000);
        elapsed += 100;
    }

    return NULL;
}


// GUI -> IPC -> ORCHESTRATOR -> WORKER QUEUE | DB QUEUE -> Workerlarda queue dinlicek


// Client -> request_queue -> HTTP workers -> response_queue -> dispatcher -> db_queue -> DB worker -> database
