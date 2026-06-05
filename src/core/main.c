#include "db_manager.h"
#include "orchestrator.h"

#include <string.h>
#include <stdio.h>

const char* response_result_to_char(ResponseResults rr);


int main(int argc, char** argv){
    int rc = 0;
    Runtime* r = NULL;
    Collection* coll = NULL;

    if(argc != 3){
        printf("Unexpected argument count.\n");
        return -1;
    }

    
    char* command = argv[1];
    if(!command){
        printf("Command is expected.");
        return -1;
    }
    

    // Database init
    rc = db_init("runner.db");
    if(rc == SQLITE_ERROR){
        printf("Database initialization error.\n");
        return -1;
    }

    // create the default collection
    coll = create_collection("DEFAULT");
    if(!coll){
        printf("Collection creation error.\n");
        return -1;
    }

    rc = db_save_collection(coll);
    if(rc == SQLITE_ERROR){
        printf("Collection db save error.\n");
        return -1;
    }



    // Runtime Creation
    r = create_runtime(1);
    if(!r){
        printf("Runtime creation error.\n");
        return -1;
    }


    // GET COMMAND
    if(strcmp("GET", command) == 0){
        char* url = argv[2];
        // validate the url ??

        
        // create a request
        Request* req = create_request(
            GET,
            coll->id,
            url,
            NULL,
            NULL,
            0
        );
        if(!req){
            printf("Create request error.\n");
            return -1;
        }

        rc = db_save_request(req);
        if(rc == SQLITE_ERROR){
            printf("SQLITE request creation error.\n");
            return -1;
        }
        
        int req_id = req->id;

        WorkerTask* wt = create_http_worker_task(req);
        if(!wt){
            printf("Worker task creation error.\n");
            return -1;
        }   
        
        rc = dispatch_task(r,wt,WORKER);
        if(rc != ORCHESTRATOR_SUCCESS){
            printf("Dispatch error.\n");
            return -1;
        }

        Response* rep = wait_for_response(req_id, 5000);
        if(!rep){
            printf("Response timeout.\n");
            return -1;
        }

        printf("Response ID: %d\n",rep->id);
        printf("Request ID: %d\n", rep->request_id);
        printf("Result: %s\n", response_result_to_char(rep->result));
        printf("Status: %d\n", rep->status_code);
        printf("Duration: %.2f\n", rep->duration);
        printf("Content-Type: %s\n", rep->content_type ? rep->content_type : "-");
        printf("Headers: %s\n", rep->headers ? rep->headers : "-");
        printf("Body: %zu bytes\n", rep->body_len); 

        if(rep->body_len > 0){

            // I assumed body %90 is string but it can be void too we must handle that situation too    
            printf("--- Body Preview ---\n");

            size_t preview_len = rep->body_len < 500 ? rep->body_len : 500;
            fwrite(rep->body,1, preview_len, stdout);

            printf("\n---\n");
        }
        free_response(rep);
    }
    else {
        printf("Unsupported command: %s\n", command);
        return -1;
    }

    return 0;
}

const char* response_result_to_char(ResponseResults rr){
    switch (rr)
    {
    case RESPONSE_OK:
        return "OK";
    case RESPONSE_CURL_ERROR:
            return "CURL_ERROR";
    default:
        return "UNKNOWN";
        
    }
}




