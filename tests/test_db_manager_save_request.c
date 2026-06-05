#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "db_manager.h"
#include "sqlite3.h"

Request* req = NULL;
Collection* dummy_coll = NULL; // Foreign Key hatasını önlemek için sahte koleksiyon eklendi

void setUp(){
    db_init(":memory:");

    // --- 1. Sahte (Dummy) Collection Oluştur ve Kaydet ---
    dummy_coll = (Collection*)malloc(sizeof(Collection));
    dummy_coll->name = strdup("Dummy Collection For Request Test");
    dummy_coll->name_len = strlen(dummy_coll->name);
    db_save_collection(dummy_coll);

    // --- 2. Request Varsayılan Ayarları ---
    req = (Request*)malloc(sizeof(Request));
    req->collection_id = dummy_coll->id; // ÖNEMLİ: 5 yerine veritabanındaki gerçek ID'yi atıyoruz
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
    
    // Request temizliği (Kendi yazdığın fonksiyonu kullanıyoruz)
    if(req) {
        free_request(req);
        req = NULL;
    }

    // Sahte Collection temizliği
    if(dummy_coll) {
        if(dummy_coll->name) free(dummy_coll->name);
        free(dummy_coll);
        dummy_coll = NULL;
    }
}

// HAPPY PATH
// ALSO CHECKS HEADER NULL AND BODY NULL
void test_db_manager_save_request_success(){
    int rc = db_save_request(req);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);
    TEST_ASSERT_GREATER_THAN(0, req->id);
}

// Request Pointer NULL
void test_db_manager_save_request_null_request(){
    free_request(req);
    req = NULL;
    int rc = db_save_request(req);
    TEST_ASSERT_EQUAL_INT(SQLITE_ERROR, rc);
}

// URL Null
void test_db_manager_save_request_null_url(){
    free(req->url);
    req->url = NULL;
    req->url_len = 0;
    int rc = db_save_request(req);
    // Veritabanı şemasında URL alanı NOT NULL olarak tanımlanmışsa bu test SQLITE_ERROR bekler
    TEST_ASSERT_EQUAL_INT(SQLITE_ERROR, rc);
}

// LARGE BODY
void test_db_manager_save_request_large_body(){
    size_t size = 100 * 1024 * 1024; // 100 MB
    void* large_body = malloc(size);
    if(!large_body) return;
    memset(large_body, '0', size);

    req->body = large_body;
    req->body_len = size;

    int rc = db_save_request(req);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);

    free(large_body);
    req->body = NULL;
}

// empty string
void test_db_manager_save_request_empty_string(){
    free(req->url);
    req->url = strdup("");
    
    // Header ve body NULL'dı setUp'ta, o yüzden string atıyoruz
    req->headers = strdup("");
    req->body = strdup("");
    
    req->url_len = 0;
    req->body_len = 0;
    req->headers_len = 0;

    int rc = db_save_request(req);
    // NOT NULL kısıtlaması empty string'leri ('') genellikle kabul eder
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db_manager_save_request_success);
    RUN_TEST(test_db_manager_save_request_null_request);
    RUN_TEST(test_db_manager_save_request_null_url);
    RUN_TEST(test_db_manager_save_request_large_body);
    RUN_TEST(test_db_manager_save_request_empty_string);
    
    return UNITY_END();
}