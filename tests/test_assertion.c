#include "unity.h"

#include "assertion.h"
#include "models.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_status_assertion_should_pass_when_status_matches(void)
{
    Response resp = {0};
    resp.status_code = 200;

    Assertion as = {0};
    as.type = ASSERT_STATUS_EQ;
    as.expected.type = VALUE_INT;
    as.expected.as.int_value = 200;

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_TRUE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_STATUS_EQ, result.type);

    TEST_ASSERT_EQUAL(VALUE_INT, result.expected.type);
    TEST_ASSERT_EQUAL_INT(200, result.expected.as.int_value);

    TEST_ASSERT_EQUAL(VALUE_INT, result.actual.type);
    TEST_ASSERT_EQUAL_INT(200, result.actual.as.int_value);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_status_assertion_should_fail_when_status_does_not_match(void)
{
    Response resp = {0};
    resp.status_code = 404;

    Assertion as = {0};
    as.type = ASSERT_STATUS_EQ;
    as.expected.type = VALUE_INT;
    as.expected.as.int_value = 200;

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_STATUS_EQ, result.type);

    TEST_ASSERT_EQUAL(VALUE_INT, result.expected.type);
    TEST_ASSERT_EQUAL_INT(200, result.expected.as.int_value);

    TEST_ASSERT_EQUAL(VALUE_INT, result.actual.type);
    TEST_ASSERT_EQUAL_INT(404, result.actual.as.int_value);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_status_assertion_should_fail_when_expected_type_is_not_int(void)
{
    Response resp = {0};
    resp.status_code = 200;

    Assertion as = {0};
    as.type = ASSERT_STATUS_EQ;
    as.expected.type = VALUE_STRING;
    as.expected.as.string_value = "200";

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_STATUS_EQ, result.type);

    TEST_ASSERT_EQUAL(VALUE_STRING, result.expected.type);
    TEST_ASSERT_EQUAL_STRING("200", result.expected.as.string_value);

    /*
        Expected type yanlış olduğu için actual set edilmemiş olmalı.
        {0} initialize edildiği için VALUE_NONE bekliyoruz.
    */
    TEST_ASSERT_EQUAL(VALUE_NONE, result.actual.type);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_eval_assertion_should_fail_with_null_response(void)
{
    Assertion as = {0};
    as.type = ASSERT_STATUS_EQ;
    as.expected.type = VALUE_INT;
    as.expected.as.int_value = 200;

    AssertionResult result = eval_assertion(NULL, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_NOT_NULL(result.message);
}

void test_eval_assertion_should_fail_with_null_assertion(void)
{
    Response resp = {0};
    resp.status_code = 200;

    AssertionResult result = eval_assertion(&resp, NULL);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_NOT_NULL(result.message);
}

void test_unknown_assertion_type_should_fail(void)
{
    Response resp = {0};
    resp.status_code = 200;

    Assertion as = {0};
    as.type = (AssertionType)999;
    as.expected.type = VALUE_INT;
    as.expected.as.int_value = 200;

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL((AssertionType)999, result.type);
    TEST_ASSERT_NOT_NULL(result.message);
}

void test_body_contains_should_pass_when_text_exists(void)
{
    const char body[] = "<html><h1>Example Domain</h1></html>";

    Response resp = {0};
    resp.body = (unsigned char*)body;
    resp.body_len = strlen(body);

    Assertion as = {0};
    as.type = ASSERT_BODY_CONTAINS;
    as.expected.type = VALUE_BYTES;
    as.expected.as.byte_value.data = (const unsigned char*)"Example Domain";
    as.expected.as.byte_value.len = strlen("Example Domain");

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_TRUE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_BODY_CONTAINS, result.type);

    TEST_ASSERT_EQUAL(VALUE_BYTES, result.expected.type);
    TEST_ASSERT_EQUAL(strlen("Example Domain"), result.expected.as.byte_value.len);

    TEST_ASSERT_EQUAL(VALUE_BYTES, result.actual.type);
    TEST_ASSERT_EQUAL(resp.body_len, result.actual.as.byte_value.len);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_body_contains_should_fail_when_text_does_not_exist(void)
{
    const char body[] = "<html><h1>Example Domain</h1></html>";

    Response resp = {0};
    resp.body = (unsigned char*)body;
    resp.body_len = strlen(body);

    Assertion as = {0};
    as.type = ASSERT_BODY_CONTAINS;
    as.expected.type = VALUE_BYTES;
    as.expected.as.byte_value.data = (const unsigned char*)"Not Found Text";
    as.expected.as.byte_value.len = strlen("Not Found Text");

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_BODY_CONTAINS, result.type);

    TEST_ASSERT_EQUAL(VALUE_BYTES, result.actual.type);
    TEST_ASSERT_EQUAL(resp.body_len, result.actual.as.byte_value.len);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_body_contains_should_pass_with_binary_bytes(void)
{
    const unsigned char body[] = {
        0x01, 0x02, 0x7F, 0x00, 0xAB, 0xFF
    };

    const unsigned char expected[] = {
        0x7F, 0x00, 0xAB
    };

    Response resp = {0};
    resp.body = (unsigned char*)body;
    resp.body_len = sizeof(body);

    Assertion as = {0};
    as.type = ASSERT_BODY_CONTAINS;
    as.expected.type = VALUE_BYTES;
    as.expected.as.byte_value.data = expected;
    as.expected.as.byte_value.len = sizeof(expected);

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_TRUE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_BODY_CONTAINS, result.type);

    TEST_ASSERT_EQUAL(VALUE_BYTES, result.expected.type);
    TEST_ASSERT_EQUAL(sizeof(expected), result.expected.as.byte_value.len);

    TEST_ASSERT_EQUAL(VALUE_BYTES, result.actual.type);
    TEST_ASSERT_EQUAL(sizeof(body), result.actual.as.byte_value.len);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_body_contains_should_fail_with_missing_binary_bytes(void)
{
    const unsigned char body[] = {
        0x01, 0x02, 0x7F, 0x00, 0xAB, 0xFF
    };

    const unsigned char expected[] = {
        0xDE, 0xAD, 0xBE, 0xEF
    };

    Response resp = {0};
    resp.body = (unsigned char*)body;
    resp.body_len = sizeof(body);

    Assertion as = {0};
    as.type = ASSERT_BODY_CONTAINS;
    as.expected.type = VALUE_BYTES;
    as.expected.as.byte_value.data = expected;
    as.expected.as.byte_value.len = sizeof(expected);

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_BODY_CONTAINS, result.type);
    TEST_ASSERT_NOT_NULL(result.message);
}

void test_body_contains_should_fail_when_expected_type_is_not_bytes(void)
{
    const char body[] = "Example Domain";

    Response resp = {0};
    resp.body = (unsigned char*)body;
    resp.body_len = strlen(body);

    Assertion as = {0};
    as.type = ASSERT_BODY_CONTAINS;
    as.expected.type = VALUE_STRING;
    as.expected.as.string_value = "Example";

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_BODY_CONTAINS, result.type);
    TEST_ASSERT_NOT_NULL(result.message);
}

void test_body_contains_should_fail_when_body_is_empty(void)
{
    Assertion as = {0};
    as.type = ASSERT_BODY_CONTAINS;
    as.expected.type = VALUE_BYTES;
    as.expected.as.byte_value.data = (const unsigned char*)"Example";
    as.expected.as.byte_value.len = strlen("Example");

    Response resp = {0};
    resp.body = NULL;
    resp.body_len = 0;

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_BODY_CONTAINS, result.type);
    TEST_ASSERT_NOT_NULL(result.message);
}


static Response make_response_with_headers(const char *headers)
{
    Response resp = {0};
    resp.status_code = 200;
    resp.headers = (char *)headers;
    resp.headers_len = headers ? strlen(headers) : 0;
    return resp;
}

void test_header_assertion_passes_when_header_name_case_differs(void)
{
    const char *headers =
        "HTTP/2 200 \r\n"
        "date: Thu, 25 Jun 2026 22:36:46 GMT\r\n"
        "content-type: text/html\r\n"
        "server: cloudflare\r\n";

    Response resp = make_response_with_headers(headers);

    Assertion assertion = {0};
    assertion.type = ASSERT_HEADER_CONTAINS;
    assertion.expected.type = VALUE_HEADER;
    assertion.expected.as.header_value.name = "Content-Type";
    assertion.expected.as.header_value.value = "text/html";

    AssertionResult result = eval_assertion(&resp, &assertion);

    TEST_ASSERT_TRUE(result.passed);
}

void test_header_assertion_fails_when_value_differs(void)
{
    const char *headers =
        "HTTP/2 200 \r\n"
        "content-type: text/html\r\n"
        "server: cloudflare\r\n";

    Response resp = make_response_with_headers(headers);

    Assertion assertion = {0};
    assertion.type = ASSERT_HEADER_CONTAINS;
    assertion.expected.type = VALUE_HEADER;
    assertion.expected.as.header_value.name = "Content-Type";
    assertion.expected.as.header_value.value = "application/json";

    AssertionResult result = eval_assertion(&resp, &assertion);

    TEST_ASSERT_FALSE(result.passed);
}

void test_header_assertion_fails_when_header_missing(void)
{
    const char *headers =
        "HTTP/2 200 \r\n"
        "server: cloudflare\r\n";

    Response resp = make_response_with_headers(headers);

    Assertion assertion = {0};
    assertion.type = ASSERT_HEADER_CONTAINS;
    assertion.expected.type = VALUE_HEADER;
    assertion.expected.as.header_value.name = "Content-Type";
    assertion.expected.as.header_value.value = "text/html";

    AssertionResult result = eval_assertion(&resp, &assertion);

    TEST_ASSERT_FALSE(result.passed);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_status_assertion_should_pass_when_status_matches);
    RUN_TEST(test_status_assertion_should_fail_when_status_does_not_match);
    RUN_TEST(test_status_assertion_should_fail_when_expected_type_is_not_int);
    RUN_TEST(test_eval_assertion_should_fail_with_null_response);
    RUN_TEST(test_eval_assertion_should_fail_with_null_assertion);
    RUN_TEST(test_unknown_assertion_type_should_fail);

    RUN_TEST(test_body_contains_should_pass_when_text_exists);
    RUN_TEST(test_body_contains_should_fail_when_text_does_not_exist);
    RUN_TEST(test_body_contains_should_pass_with_binary_bytes);
    RUN_TEST(test_body_contains_should_fail_with_missing_binary_bytes);
    RUN_TEST(test_body_contains_should_fail_when_expected_type_is_not_bytes);
    RUN_TEST(test_body_contains_should_fail_when_body_is_empty);


    RUN_TEST(test_header_assertion_passes_when_header_name_case_differs);
    RUN_TEST(test_header_assertion_fails_when_value_differs);
    RUN_TEST(test_header_assertion_fails_when_header_missing);
    return UNITY_END();
}