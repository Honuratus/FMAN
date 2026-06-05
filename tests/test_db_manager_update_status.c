#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "db_manager.h"
#include "sqlite3.h"

Request* req = NULL;
Collection* dummy_coll = NULL;

void setUp(){
    db_init(":memory:");

    dummy_coll = (Collection*)malloc(sizeof(Collection));
    dummy_coll->name = strdup("Dummy Collection For Response Test");
    dummy_coll->name_len = strlen(dummy_coll->name);
    db_save_collection(dummy_coll);

    req = (Request*)malloc(sizeof(Request));
    req->collection_id = dummy_coll->id;
    req->method = GET;
    req->body = NULL;
    req->body_len = 0;
    req->headers = NULL;
    req->headers_len = 0;
    req->url = strdup("https://www.example.com");
    req->url_len = strlen(req->url);
}

void tearDown(){
    db_close();
    if(req) free_request(req);
    if(dummy_coll){
        if(dummy_coll->name) free(dummy_coll->name);
        free(dummy_coll);
    }
}

void test_db_update_status_success(){
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, db_save_request(req));

    Request* new_req;
    new_req = db_get_next_pending_request();
    TEST_ASSERT_NOT_NULL(new_req);

    int target_id = new_req->id;

    int rc = db_update_request_status(target_id, 2);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);

    rc = db_update_request_status(target_id, 3);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);


    rc = db_update_request_status(target_id, 0);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);

    Request* verify_req = db_get_next_pending_request();
    TEST_ASSERT_NOT_NULL(verify_req);
    TEST_ASSERT_EQUAL_INT(target_id, verify_req->id);


    free_request(new_req);
    free_request(verify_req);
}



int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db_update_status_success);
    
    return UNITY_END();
}