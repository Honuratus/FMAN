#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // usleep
#include <sqlite3.h>

#include "db_manager.h"
#include "orchestrator.h"
#include "worker.h"
#include "models.h"

#define TEST_DB_NAME "test_runtime.db"
#define TEST_URL "http://127.0.0.1:8080/bin"
#define TEST_TIMEOUT_MS 5000
#define POLL_INTERVAL_MS 100

static Runtime* rt = NULL;
Collection* coll;
int collection_id;

static int wait_until_response_count_at_least(int request_id, int expected_count, int timeout_ms)
{
    int elapsed = 0;

    while (elapsed < timeout_ms) {
        int count = db_count_responses_by_request_id(request_id);

        if (count >= expected_count) {
            return 1;
        }

        usleep(POLL_INTERVAL_MS * 1000);
        elapsed += POLL_INTERVAL_MS;
    }

    return 0;
}



void setUp(void)
{
    int rc = db_init(TEST_DB_NAME);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    rc = db_clear_all();
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    rt = create_runtime(2);
    TEST_ASSERT_NOT_NULL(rt);


    coll = calloc(1, sizeof(Collection));
    coll->name = strdup("test collection");
    coll->name_len = strlen(coll->name);

    rc = db_save_collection(coll);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    collection_id = coll->id;
    
}

void tearDown(void)
{
    if (rt) {
        destroy_runtime(rt);
        rt = NULL;
    }

    free_collection(coll);
    db_clear_all();
    db_close();
}

void test_single_get_request_should_flow_through_runtime_and_be_saved(void)
{
    Request* req = create_request(
        GET,
        collection_id,
        TEST_URL,
        NULL,
        NULL,
        0
    );

    TEST_ASSERT_NOT_NULL(req);

    int rc = db_save_request(req);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    int req_id = req->id;
    TEST_ASSERT_TRUE(req_id > 0);

    WorkerTask* task = create_http_worker_task(req);

    rc = dispatch_task(rt, task, WORKER);
    TEST_ASSERT_EQUAL(ORCHESTRATOR_SUCCESS, rc);

    /*
        dispatch success sonrası ownership runtime'a geçti.
        Artık test req/task free etmeyecek.
    */

    int found = wait_until_response_count_at_least(req_id, 1, TEST_TIMEOUT_MS);
    TEST_ASSERT_TRUE_MESSAGE(found, "Response DB'ye timeout içinde düşmedi.");

    Response* resp = db_get_latest_response_by_request_id(req_id);
    TEST_ASSERT_NOT_NULL(resp);

    TEST_ASSERT_EQUAL(req_id, resp->request_id);
    TEST_ASSERT_EQUAL(RESPONSE_OK, resp->result);
    TEST_ASSERT_EQUAL(200, resp->status_code);
    TEST_ASSERT_TRUE(resp->body_len > 0);

    free_response(resp);
}

void test_runtime_should_shutdown_cleanly_without_tasks(void)
{
    /*
        Bu testin amacı:
        Runtime açılıp hiç iş almadan destroy_runtime ile deadlock olmadan kapanıyor mu?
        Burada özel assert yok; test process takılmazsa başarılı.
    */

    TEST_ASSERT_NOT_NULL(rt);
}

void test_invalid_url_should_still_save_error_response(void)
{
    Request* req = create_request(
        GET,
        collection_id,
        "http://invalid.invalid/",
        NULL,
        NULL,
        0
    );

    TEST_ASSERT_NOT_NULL(req);

    int rc = db_save_request(req);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    int req_id = req->id;
    TEST_ASSERT_TRUE(req_id > 0);

    WorkerTask* task = create_http_worker_task(req);

    rc = dispatch_task(rt, task, WORKER);
    TEST_ASSERT_EQUAL(ORCHESTRATOR_SUCCESS, rc);

    int found = wait_until_response_count_at_least(req_id, 1, TEST_TIMEOUT_MS);
    TEST_ASSERT_TRUE_MESSAGE(found, "Curl error response DB'ye düşmedi.");

    Response* resp = db_get_latest_response_by_request_id(req_id);
    TEST_ASSERT_NOT_NULL(resp);

    TEST_ASSERT_EQUAL(req_id, resp->request_id);
    TEST_ASSERT_EQUAL(RESPONSE_CURL_ERROR, resp->result);

    free_response(resp);
}

void test_multiple_get_requests_should_all_be_saved(void)
{
    const int request_count = 20;

    for (int i = 0; i < request_count; i++) {
        Request* req = create_request(
            GET,
            collection_id,
            TEST_URL,
            NULL,
            NULL,
            0
        );

        TEST_ASSERT_NOT_NULL(req);

        int rc = db_save_request(req);
        TEST_ASSERT_EQUAL(SQLITE_OK, rc);

        WorkerTask* task = create_http_worker_task(req);

        rc = dispatch_task(rt, task, WORKER);
        TEST_ASSERT_EQUAL(ORCHESTRATOR_SUCCESS, rc);
    }

    int elapsed = 0;

    while (elapsed < TEST_TIMEOUT_MS) {
        int count = db_count_all_responses();

        if (count >= request_count) {
            TEST_ASSERT_TRUE(1);
            return;
        }

        usleep(POLL_INTERVAL_MS * 1000);
        elapsed += POLL_INTERVAL_MS;
    }

    TEST_FAIL_MESSAGE("Multiple request test timeout: tüm response'lar DB'ye düşmedi.");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_runtime_should_shutdown_cleanly_without_tasks);
    RUN_TEST(test_single_get_request_should_flow_through_runtime_and_be_saved);
    RUN_TEST(test_invalid_url_should_still_save_error_response);
    RUN_TEST(test_multiple_get_requests_should_all_be_saved);

    return UNITY_END();
}