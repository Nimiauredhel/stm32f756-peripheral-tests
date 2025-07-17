/**
 * @file db.c
 * @brief Source file for the test client module's database functions.
 */

#include "sqlite3.h"
#include "db.h"

#define TESTS_DB_PATH "tests.db"

static sqlite3_stmt *stmt_append_request = NULL;
static sqlite3_stmt *stmt_append_result = NULL;

static sqlite3 *open_tests_db(void)
{
    sqlite3 *tests_db = NULL;
    int ret = sqlite3_open(TESTS_DB_PATH, &tests_db);

    if (ret != SQLITE_OK)
    {
        printf("Failed to open tests DB: %s\n", sqlite3_errmsg(tests_db));
        return NULL;
    }

    return tests_db;
}

void db_init(void)
{
    static const char db_str_create_requests_table[] =
    {
        "CREATE TABLE IF NOT EXISTS requests ("
        "test_id INTEGER NOT NULL, "
        "time_sent TEXT NOT NULL, "
        "test_string TEXT, "
        "test_iterations INTEGER NOT NULL, "
        "tests_selected INTEGER NOT NULL );"
    };

    static const char db_str_create_results_table[] =
    {
        "CREATE TABLE IF NOT EXISTS results ("
        "test_id INTEGER NOT NULL, "
        "time_received TEXT NOT NULL, "
        "test_string TEXT, "
        "test_iterations INTEGER NOT NULL, "
        "tests_selected INTEGER NOT NULL, "
        "tests_passed INTEGER NOT NULL, "
        "duration_seconds REAL NOT NULL );"
    };

    static const char db_str_append_request[] =
    {
        "INSERT INTO requests VALUES(?, ?, ?, ?, ?)"
    };

    static const char db_str_append_result[] =
    {
        "INSERT INTO results VALUES(?, ?, ?, ?, ?, ?, ?)"
    };

    sqlite3 *tests_db = open_tests_db();

    if (tests_db == NULL)
    {
        goto open_failure;
    }

    char *sqlite_error_msg;

    if (SQLITE_OK != sqlite3_exec(tests_db, db_str_create_requests_table, NULL, NULL, &sqlite_error_msg))
    {
        printf("Error creating requests table: %s\n", sqlite_error_msg);
        goto exec_failure;
    }

    if (SQLITE_OK != sqlite3_exec(tests_db, db_str_create_results_table, NULL, NULL, &sqlite_error_msg))
    {
        printf("Error creating results table: %s\n", sqlite_error_msg);
        goto exec_failure;
    }

    int ret;
    ret = sqlite3_prepare_v2(tests_db, db_str_append_request, strlen(db_str_append_request), &stmt_append_request, NULL);

    if(ret != SQLITE_OK)
    {
        printf("Error preparing append request statement: %s\n", sqlite3_errstr(ret));
        goto prepare_failure;
    }

    ret = sqlite3_prepare_v2(tests_db, db_str_append_result, strlen(db_str_append_result), &stmt_append_result, NULL);

    if(ret != SQLITE_OK)
    {
        printf("Error preparing append result statement: %s\n", sqlite3_errstr(ret));
        goto prepare_failure;
    }

    sqlite3_close(tests_db);
    return;

prepare_failure:
    if (stmt_append_request != NULL) sqlite3_finalize(stmt_append_request);
exec_failure:
    sqlite3_free(sqlite_error_msg);
    sqlite3_close(tests_db);
open_failure:
    why_terminate = TERMR_ERROR;
    should_terminate = true;
    return;
}

void db_deinit(void)
{
    if (stmt_append_request != NULL) sqlite3_finalize(stmt_append_request);
    if (stmt_append_result != NULL) sqlite3_finalize(stmt_append_result);
}

void db_append_request(uint8_t *request_buffer)
{
    char datetime[64] = {0};
    datetime_str_nonalloc(datetime, sizeof(datetime));

    printf("Recording request to DB at date-time: %s\n", datetime);

    sqlite3_bind_int(stmt_append_request, 1, *(uint32_t *)(request_buffer+TEST_PACKET_ID_BYTE_OFFSET));
    sqlite3_bind_text(stmt_append_request, 2, datetime, strlen(datetime), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_append_request, 3, (char *)request_buffer+TEST_PACKET_STRING_HEAD_OFFSET, request_buffer[TEST_PACKET_STRING_LEN_OFFSET], SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_append_request, 4, request_buffer[TEST_PACKET_ITERATIONS_BYTE_OFFSET]);
    sqlite3_bind_int(stmt_append_request, 5, request_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET]);

    int ret = sqlite3_step(stmt_append_request);

    if (ret != SQLITE_DONE)
    {
        printf ("Statement step error: %s\n", sqlite3_errstr(ret));
    }

    sqlite3_reset(stmt_append_request);
}

void db_append_results(uint8_t *results_buffer, uint8_t *request_buffer, float duration_secs)
{
    char datetime[64] = {0};
    datetime_str_nonalloc(datetime, sizeof(datetime));

    printf("Recording result to DB at date-time: %s\n", datetime);

    sqlite3_bind_int(stmt_append_result, 1, *(uint32_t *)(results_buffer+TEST_PACKET_ID_BYTE_OFFSET));
    sqlite3_bind_text(stmt_append_result, 2, datetime, strlen(datetime), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_append_result, 3, (char *)request_buffer+TEST_PACKET_STRING_HEAD_OFFSET, request_buffer[TEST_PACKET_STRING_LEN_OFFSET], SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_append_result, 4, request_buffer[TEST_PACKET_ITERATIONS_BYTE_OFFSET]);
    sqlite3_bind_int(stmt_append_result, 5, request_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET]);
    sqlite3_bind_int(stmt_append_result, 6, results_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET]);
    sqlite3_bind_double(stmt_append_result, 7, duration_secs);

    int ret = sqlite3_step(stmt_append_result);

    if (ret != SQLITE_DONE)
    {
        printf ("Statement step error: %s\n", sqlite3_errstr(ret));
    }

    sqlite3_reset(stmt_append_result);
}
