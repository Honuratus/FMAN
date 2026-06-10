#include "db_manager.h"
#include "orchestrator.h"
#include "cli_output.h"

#include <string.h>
#include <stdio.h>


static int run_get_command(const char* url){
    int rc = 0;
    int exit_code = -1;

    Runtime* r = NULL;
    Collection* coll = NULL;
    Response* resp = NULL;
    Request* req = NULL;
    WorkerTask* task = NULL;

    if (!url || *url == '\0') {
        printf("URL is required.\n");
        goto cleanup;
    }

    rc = db_init("runner.db");
    if (rc == SQLITE_ERROR) {
        printf("Database initialization error.\n");
        goto cleanup;
    }

    coll = create_collection("DEFAULT");
    if (!coll) {
        printf("Collection creation error.\n");
        goto cleanup;
    }

    rc = db_save_collection(coll);
    if (rc == SQLITE_ERROR) {
        printf("Collection db save error.\n");
        goto cleanup;
    }

    r = create_runtime(1);
    if (!r) {
        printf("Runtime creation error.\n");
        goto cleanup;
    }

    req = create_request(
        GET,
        coll->id,
        url,
        NULL,
        NULL,
        0
    );
    if (!req) {
        printf("Create request error.\n");
        goto cleanup;
    }


    rc = db_save_request(req);
    if (rc == SQLITE_ERROR) {
        printf("SQLITE request creation error.\n");
        goto cleanup;
    }

    int req_id = req->id;


    task = create_http_worker_task(req);
    if (!task) {
        printf("Worker task creation error.\n");
        goto cleanup;
    }

    req = NULL; // main -> orchestrator transfer

    rc = dispatch_task(r, task, WORKER);
    if (rc != ORCHESTRATOR_SUCCESS) {
        printf("Dispatch error.\n");
        goto cleanup;
    }

    task = NULL; // orchestrator -> runtime transfer

    resp = wait_for_response(req_id, 5000);
    if (!resp) {
        printf("Response timeout.\n");
        goto cleanup;
    }

    print_response_summary(resp);

    exit_code = 0;

cleanup:
    if(resp)
        free_response(resp);
    if(req)
        free_request(req);
    if(r)
        destroy_runtime(r);
    if(coll)
        free_collection(coll);
    
    db_close();

    return exit_code;
}   


static void print_usage(const char* program_name){
    printf("Usage:\n");
    printf("\t%s GET <url>\n", program_name);
    printf("\n");

    printf("\nExamples:\n");
    printf("\t%s GET https://example.com\n", program_name);
    printf("\t%s GET google.com\n", program_name);
    printf("\n");
}


int main(int argc, char** argv){
    if(argc != 3){
        print_usage(argv[0]);
        return -1;
    }

    char* command = argv[1];
    if(!command){
        printf("Command is expected.");
        return -1;
    }
    

    // GET COMMAND
    if(strcmp("GET", command) == 0){
        return run_get_command(argv[2]);
    }

    printf("Unsupported command: %s\n", command);
    print_usage(argv[0]);
    return 0;
}








