#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "db_manager.h"
#include "sqlite3.h"

Collection* coll = NULL;

void setUp(){
    // Her testten önce temiz bir in-memory DB başlatılır
    db_init(":memory:");
    
    // Collection Varsayılan Ayarları
    coll = (Collection*)malloc(sizeof(Collection));
    coll->name = strdup("Test Postman Collection");
    coll->name_len = strlen(coll->name);
    coll->id = 0;
}

void tearDown(){
    db_close();
    
    // Bellek temizliği
    if(coll) {
        if(coll->name) free(coll->name);
        free(coll);
    }
}

// HAPPY PATH - Collection Başarılı Kayıt
void test_db_manager_save_collection_success(){
    int rc = db_save_collection(coll);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc);
    TEST_ASSERT_GREATER_THAN(0, coll->id);
}

// Collection Pointer NULL
void test_db_manager_save_collection_null_pointer(){
    free(coll->name);
    free(coll);
    coll = NULL;
    
    int rc = db_save_collection(coll);
    TEST_ASSERT_EQUAL_INT(SQLITE_ERROR, rc);
}

// Collection Name Empty String ("")
void test_db_manager_save_collection_empty_name(){
    free(coll->name);
    coll->name = strdup("");
    coll->name_len = 0;
    
    int rc = db_save_collection(coll);
    TEST_ASSERT_EQUAL_INT(SQLITE_OK, rc); 
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db_manager_save_collection_success);
    RUN_TEST(test_db_manager_save_collection_null_pointer);
    RUN_TEST(test_db_manager_save_collection_empty_name);
    
    return UNITY_END();
}