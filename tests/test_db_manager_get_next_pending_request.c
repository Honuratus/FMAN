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

    // dummy coll for foreign key
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
    if(dummy_coll) {
        if(dummy_coll->name) free(dummy_coll->name);
        free(dummy_coll);
    }
}



void test_db_manager_get_next_pending_request_success(){
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, db_save_request(req));
    int saved_id = req->id;

    Request* new_req;
    
    new_req = db_get_next_pending_request();
    TEST_ASSERT_NOT_NULL(new_req);

    TEST_ASSERT_EQUAL_INT(saved_id, new_req->id);
    
    Request* empty_req = db_get_next_pending_request();
    TEST_ASSERT_NULL(empty_req);


    free_request(new_req);
}


void test_db_manager_get_next_pending_request_fifo() {
    free(req->url);
    req->url = strdup("https://first-request.com");
    req->url_len = strlen(req->url);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, db_save_request(req));
    int first_id = req->id;

    
    free(req->url);
    req->url = strdup("https://second-request.com");
    req->url_len = strlen(req->url);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, db_save_request(req));
    int second_id = req->id;

    
    Request* r1 = db_get_next_pending_request();
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_INT(first_id, r1->id);

    Request* r2 = db_get_next_pending_request();
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL_INT(second_id, r2->id);

 
    Request* r3 = db_get_next_pending_request();
    TEST_ASSERT_NULL(r3);

 
    free_request(r1);
    free_request(r2);
}

void test_db_manager_get_next_pending_request_no_data() {
    Request* r = db_get_next_pending_request();
    TEST_ASSERT_NULL(r);
}

void test_db_manager_get_next_pending_request_null_fields() {
    req->headers = NULL;
    req->body = NULL;
    req->headers_len = 0;
    req->body_len = 0;
    db_save_request(req);

    Request* fetched = db_get_next_pending_request();
    
    TEST_ASSERT_NOT_NULL(fetched);
    TEST_ASSERT_NULL(fetched->headers);
    TEST_ASSERT_NULL(fetched->body);
    TEST_ASSERT_EQUAL_INT(0, fetched->headers_len);
    TEST_ASSERT_EQUAL_INT(0, fetched->body_len);

    free_request(fetched);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db_manager_get_next_pending_request_success);
    RUN_TEST(test_db_manager_get_next_pending_request_fifo);
    RUN_TEST(test_db_manager_get_next_pending_request_null_fields);
    RUN_TEST(test_db_manager_get_next_pending_request_no_data);

    return UNITY_END();
}