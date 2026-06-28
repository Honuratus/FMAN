#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "db_manager.h"
#include "models.h"

#define TEST_DB_PATH "test_runtime.db"

void setUp(void)
{
    remove(TEST_DB_PATH);
}

void tearDown(void)
{
    db_close();
    remove(TEST_DB_PATH);
}

static Collection* create_and_save_default_collection(void)
{
    int rc;

    Collection* coll = create_collection("default");
    TEST_ASSERT_NOT_NULL(coll);

    rc = db_save_collection(coll);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(coll->id > 0);

    return coll;
}

void test_db_save_request_reuses_same_request_for_same_method_url_collection(void)
{
    int rc;

    rc = db_init(TEST_DB_PATH);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    Collection* coll = create_and_save_default_collection();

    Request* req1 = create_request(GET, coll->id ,"http://127.0.0.1:8080/ok", NULL, NULL, 0);
    TEST_ASSERT_NOT_NULL(req1);

    req1->collection_id = coll->id;

    rc = db_save_request(req1);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(req1->id > 0);

    int first_request_id = req1->id;

    Request* req2 = create_request(GET, coll->id, "http://127.0.0.1:8080/ok", NULL, NULL, 0);
    TEST_ASSERT_NOT_NULL(req2);

    req2->collection_id = coll->id;

    rc = db_save_request(req2);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(req2->id > 0);

    int second_request_id = req2->id;

    TEST_ASSERT_EQUAL_INT(first_request_id, second_request_id);

    free_request(req1);
    free_request(req2);
    free_collection(coll);
}

void test_db_save_request_creates_new_request_for_different_url(void)
{
    int rc;

    rc = db_init(TEST_DB_PATH);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    Collection* coll = create_and_save_default_collection();

    Request* req1 = create_request(GET,coll->id,  "http://127.0.0.1:8080/ok", NULL, NULL, 0);
    TEST_ASSERT_NOT_NULL(req1);

    req1->collection_id = coll->id;

    rc = db_save_request(req1);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(req1->id > 0);

    Request* req2 = create_request(GET,coll->id, "http://127.0.0.1:8080/header", NULL, NULL, 0);
    TEST_ASSERT_NOT_NULL(req2);

    req2->collection_id = coll->id;

    rc = db_save_request(req2);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(req2->id > 0);

    TEST_ASSERT_NOT_EQUAL(req1->id, req2->id);

    free_request(req1);
    free_request(req2);
    free_collection(coll);
}

void test_db_save_request_creates_new_request_for_different_method_same_url(void)
{
    int rc;

    rc = db_init(TEST_DB_PATH);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    Collection* coll = create_and_save_default_collection();

    Request* req1 = create_request(GET, coll->id, "http://127.0.0.1:8080/ok", NULL, NULL, 0);
    TEST_ASSERT_NOT_NULL(req1);

    req1->collection_id = coll->id;

    rc = db_save_request(req1);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(req1->id > 0);

    Request* req2 = create_request(POST, coll->id, "http://127.0.0.1:8080/ok", NULL, NULL, 0);
    TEST_ASSERT_NOT_NULL(req2);

    req2->collection_id = coll->id;

    rc = db_save_request(req2);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(req2->id > 0);

    TEST_ASSERT_NOT_EQUAL(req1->id, req2->id);

    free_request(req1);
    free_request(req2);
    free_collection(coll);
}

void test_db_save_request_creates_new_request_for_different_collection_same_method_url(void)
{
    int rc;

    rc = db_init(TEST_DB_PATH);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    Collection* coll1 = create_collection("default");
    TEST_ASSERT_NOT_NULL(coll1);

    rc = db_save_collection(coll1);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(coll1->id > 0);

    Collection* coll2 = create_collection("smoke");
    TEST_ASSERT_NOT_NULL(coll2);

    rc = db_save_collection(coll2);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(coll2->id > 0);

    Request* req1 = create_request(GET,coll1->id, "http://127.0.0.1:8080/ok", NULL, NULL, 0);
    TEST_ASSERT_NOT_NULL(req1);

    req1->collection_id = coll1->id;

    rc = db_save_request(req1);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(req1->id > 0);

    Request* req2 = create_request(GET,coll2->id, "http://127.0.0.1:8080/ok", NULL, NULL, 0);
    TEST_ASSERT_NOT_NULL(req2);

    req2->collection_id = coll2->id;

    rc = db_save_request(req2);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);
    TEST_ASSERT_TRUE(req2->id > 0);

    TEST_ASSERT_NOT_EQUAL(req1->id, req2->id);

    free_request(req1);
    free_request(req2);
    free_collection(coll1);
    free_collection(coll2);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_db_save_request_reuses_same_request_for_same_method_url_collection);
    RUN_TEST(test_db_save_request_creates_new_request_for_different_url);
    RUN_TEST(test_db_save_request_creates_new_request_for_different_method_same_url);
    RUN_TEST(test_db_save_request_creates_new_request_for_different_collection_same_method_url);

    return UNITY_END();
}