#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "db_manager.h"
#include "sqlite3.h"

Response* resp = NULL;
Request* dummy_req = NULL;
Collection* dummy_coll = NULL;

void setUp(){
    // Her testten önce temiz bir in-memory DB başlatılır
    db_init(":memory:");
    
    // --- 1. Sahte (Dummy) Collection Oluştur ve Kaydet ---
    // Foreign Key hatası almamak için hiyerarşiyi baştan kuruyoruz.
    dummy_coll = (Collection*)malloc(sizeof(Collection));
    dummy_coll->name = strdup("Dummy Collection For Response Test");
    dummy_coll->name_len = strlen(dummy_coll->name);
    db_save_collection(dummy_coll);

    // --- 2. Sahte (Dummy) Request Oluştur ve Kaydet ---
    dummy_req = (Request*)malloc(sizeof(Request));
    dummy_req->collection_id = dummy_coll->id; // Gerçek collection ID'sini atadık
    dummy_req->method = GET;                   // Enum'a göre uyarlayabilirsin
    dummy_req->url = strdup("http://dummy.com");
    dummy_req->url_len = strlen(dummy_req->url);
    dummy_req->headers = NULL;
    dummy_req->headers_len = 0;
    dummy_req->body = NULL;
    dummy_req->body_len = 0;
    db_save_request(dummy_req);

    // --- 3. Response Varsayılan Ayarları ---
    resp = (Response*)malloc(sizeof(Response));
    resp->request_id = dummy_req->id; // ÖNEMLİ: Rastgele 15 yerine artık veritabanındaki gerçek Request ID'sini kullanıyoruz!
    resp->status_code = 200;
    resp->duration = 120;
    resp->headers = strdup("Content-Type: application/json");
    resp->headers_len = strlen((char*)resp->headers);
    resp->body = strdup("{\"status\": \"ok\"}");
    resp->body_len = strlen((char*)resp->body);
    resp->id = 0;
}

void tearDown(){
    db_close();
    
    // Bellek temizliği (Response)
    if(resp) {
        if(resp->headers) free((void*)resp->headers);
        if(resp->body) free(resp->body);
        free(resp);
    }
    
    // Bellek temizliği (Dummy Request)
    if(dummy_req) {
        if(dummy_req->url) free(dummy_req->url);
        free(dummy_req);
    }
    
    // Bellek temizliği (Dummy Collection)
    if(dummy_coll) {
        if(dummy_coll->name) free(dummy_coll->name);
        free(dummy_coll);
    }
}

// HAPPY PATH - Response Başarılı Kayıt
void test_db_manager_save_response_success(){
    int rc = db_save_response(resp);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);
    TEST_ASSERT_GREATER_THAN(0, resp->id);
}

// Response Pointer NULL
void test_db_manager_save_response_null_pointer(){
    free((void*)resp->headers);
    free(resp->body);
    free(resp);
    resp = NULL;
    
    int rc = db_save_response(resp);
    TEST_ASSERT_EQUAL_INT(SQLITE_ERROR, rc);
}

// LARGE BODY - Yüksek boyutlu response body testi
void test_db_manager_save_response_large_body(){
    size_t size = 50 * 1024 * 1024; // 50 MB
    void* large_body = malloc(size);
    if(!large_body) return; 
    memset(large_body, 'A', size);

    free(resp->body);
    resp->body = large_body;
    resp->body_len = size;

    int rc = db_save_response(resp);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);
}

// EMPTY FIELDS - Headers ve Body boş string olduğunda
void test_db_manager_save_response_empty_fields(){
    free((void*)resp->headers);
    free(resp->body);
    
    resp->headers = strdup("");
    resp->headers_len = 0;
    resp->body = strdup("");
    resp->body_len = 0;

    int rc = db_save_response(resp);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db_manager_save_response_success);
    RUN_TEST(test_db_manager_save_response_null_pointer);
    RUN_TEST(test_db_manager_save_response_large_body);
    RUN_TEST(test_db_manager_save_response_empty_fields);
    
    return UNITY_END();
}