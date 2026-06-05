#ifndef WORKER_H    
#define WORKER_H

void* http_worker_routine(void* arg);
void* db_worker_routine(void* arg);

#endif