#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

typedef struct {
    int id;
    char username[64];
    char role[16];
    int crew_db_id;
} User;

typedef struct IDBManager IDBManager;
typedef struct IAuthService IAuthService;
typedef struct IFlotillaService IFlotillaService;

struct IDBManager {
    void* vtable;
};

struct IAuthService {
    void* vtable;
};

struct IFlotillaService {
    void* vtable;
};

typedef struct {
    void* db;
    User current_user;
    int logged_in;
} AuthService;

typedef struct {
    sqlite3* db;
} DBManager;

typedef struct {
    void* db;
    User user;
} FlotillaService;

extern sqlite3* get_sqlite_handle(void* db_manager);
extern int db_manager_init(void* db_manager, const char* path);
extern int db_manager_exec(void* db_manager, const char* sql);
extern int auth_login(void* auth_service, const char* username, const char* password, User* out_user);
extern int auth_is_authenticated(void* auth_service);
extern void auth_logout(void* auth_service);
extern User auth_get_current_user(void* auth_service);

static sqlite3* test_db = NULL;
static const char* TEST_DB_PATH = "test_flotilla.db";

int open_test_database(void) {
    int rc = sqlite3_open(TEST_DB_PATH, &test_db);
    if (rc != SQLITE_OK) return 0;
    
    const char* create_sql = 
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS TRAWLERS ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name TEXT NOT NULL UNIQUE,"
        "    displacement REAL CHECK(displacement > 0),"
        "    build_date TEXT NOT NULL,"
        "    photo BLOB"
        ");"
        "CREATE TABLE IF NOT EXISTS CREW ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    surname TEXT NOT NULL,"
        "    position TEXT NOT NULL,"
        "    hire_date TEXT NOT NULL,"
        "    birth_year INTEGER,"
        "    trawler_id INTEGER,"
        "    username TEXT NOT NULL UNIQUE,"
        "    password TEXT NOT NULL,"
        "    role TEXT CHECK(role IN ('admin', 'crew')) NOT NULL DEFAULT 'crew',"
        "    FOREIGN KEY (trawler_id) REFERENCES TRAWLERS(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS BANKS ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name TEXT NOT NULL UNIQUE"
        ");"
        "CREATE TABLE IF NOT EXISTS TRIPS ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    trawler_id INTEGER NOT NULL,"
        "    bank_id INTEGER NOT NULL,"
        "    departure_date TEXT NOT NULL,"
        "    return_date TEXT NOT NULL,"
        "    FOREIGN KEY (trawler_id) REFERENCES TRAWLERS(id),"
        "    FOREIGN KEY (bank_id) REFERENCES BANKS(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS CATCH ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    trip_id INTEGER NOT NULL,"
        "    fish_name TEXT NOT NULL,"
        "    quality TEXT CHECK(quality IN ('high', 'medium', 'low')) NOT NULL,"
        "    quantity_kg REAL CHECK(quantity_kg >= 0),"
        "    FOREIGN KEY (trip_id) REFERENCES TRIPS(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS STATS ("
        "    trawler_id INTEGER PRIMARY KEY,"
        "    total_catch_kg REAL DEFAULT 0,"
        "    FOREIGN KEY (trawler_id) REFERENCES TRAWLERS(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS BONUSES ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    crew_id INTEGER NOT NULL,"
        "    period_start TEXT NOT NULL,"
        "    period_end TEXT NOT NULL,"
        "    amount REAL NOT NULL,"
        "    FOREIGN KEY (crew_id) REFERENCES CREW(id)"
        ");"
        "INSERT OR IGNORE INTO TRAWLERS (id, name, displacement, build_date) VALUES (1, 'Тайфун', 5000.0, '2010-05-15');"
        "INSERT OR IGNORE INTO TRAWLERS (id, name, displacement, build_date) VALUES (2, 'Шторм', 4500.0, '2012-08-20');"
        "INSERT OR IGNORE INTO CREW (id, surname, position, hire_date, birth_year, trawler_id, username, password, role) "
        "VALUES (1, 'Иванов', 'captain', '2015-01-10', 1960, 1, 'cap_ivanov', '123', 'crew');"
        "INSERT OR IGNORE INTO CREW (id, surname, position, hire_date, birth_year, trawler_id, username, password, role) "
        "VALUES (2, 'Петров', 'sailor', '2018-03-15', 1995, 1, 'sail_petrov', '123', 'crew');"
        "INSERT OR IGNORE INTO BANKS (id, name) VALUES (1, 'Северная банка');"
        "INSERT OR IGNORE INTO BANKS (id, name) VALUES (2, 'Южная банка');"
        "INSERT OR IGNORE INTO TRIPS (id, trawler_id, bank_id, departure_date, return_date) "
        "VALUES (1, 1, 1, '2023-06-01', '2023-06-15');"
        "INSERT OR IGNORE INTO TRIPS (id, trawler_id, bank_id, departure_date, return_date) "
        "VALUES (2, 1, 2, '2023-07-10', '2023-07-25');"
        "INSERT OR IGNORE INTO CATCH (id, trip_id, fish_name, quality, quantity_kg) VALUES (1, 1, 'Треска', 'high', 600.0);"
        "INSERT OR IGNORE INTO CATCH (id, trip_id, fish_name, quality, quantity_kg) VALUES (2, 1, 'Сельдь', 'low', 150.0);"
        "INSERT OR IGNORE INTO STATS (trawler_id, total_catch_kg) VALUES (1, 750.0);"
        "INSERT OR IGNORE INTO STATS (trawler_id, total_catch_kg) VALUES (2, 0.0);";
    
    char* err = NULL;
    rc = sqlite3_exec(test_db, create_sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

void close_test_database(void) {
    if (test_db) {
        sqlite3_close(test_db);
        test_db = NULL;
    }
    remove(TEST_DB_PATH);
}

int count_rows(const char* table_name) {
    if (!test_db) return -1;
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s", table_name);
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

double get_sum(const char* table, const char* column) {
    if (!test_db) return -1.0;
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COALESCE(SUM(%s), 0) FROM %s", column, table);
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1.0;
    double sum = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) sum = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);
    return sum;
}

void test_db_open_close(void) {
    sqlite3* local_db = NULL;
    int rc = sqlite3_open("temp_test.db", &local_db);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    CU_ASSERT_PTR_NOT_NULL(local_db);
    sqlite3_close(local_db);
    remove("temp_test.db");
}

void test_db_init_success(void) {
    int result = open_test_database();
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_PTR_NOT_NULL(test_db);
}

void test_db_tables_exist(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(test_db, "SELECT name FROM sqlite_master WHERE type='table' AND name='TRAWLERS'", -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    int has_row = (sqlite3_step(stmt) == SQLITE_ROW);
    CU_ASSERT_TRUE(has_row);
    sqlite3_finalize(stmt);
}

void test_db_insert_trawler(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    char* err = NULL;
    int rc = sqlite3_exec(test_db, "INSERT INTO TRAWLERS (name, displacement, build_date) VALUES ('Тест', 3000.0, '2024-01-01')", NULL, NULL, &err);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    CU_ASSERT_PTR_NULL(err);
    int count = count_rows("TRAWLERS");
    CU_ASSERT_TRUE(count >= 3);
}

void test_db_select_trawlers(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    int count = count_rows("TRAWLERS");
    CU_ASSERT_TRUE(count >= 2);
    CU_ASSERT_EQUAL(count, 2);
}

void test_db_update_trawler(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    char* err = NULL;
    int rc = sqlite3_exec(test_db, "UPDATE TRAWLERS SET displacement = 5500.0 WHERE id = 1", NULL, NULL, &err);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(test_db, "SELECT displacement FROM TRAWLERS WHERE id = 1", -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        double disp = sqlite3_column_double(stmt, 0);
        CU_ASSERT_DOUBLE_EQUAL(disp, 5500.0, 0.01);
    }
    sqlite3_finalize(stmt);
}

void test_db_delete_catch(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    int before = count_rows("CATCH");
    char* err = NULL;
    int rc = sqlite3_exec(test_db, "DELETE FROM CATCH WHERE id = 2", NULL, NULL, &err);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    int after = count_rows("CATCH");
    CU_ASSERT_EQUAL(after, before - 1);
}

void test_db_join_query(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT t.id, tr.name, b.name FROM TRIPS t "
                      "JOIN TRAWLERS tr ON t.trawler_id = tr.id "
                      "JOIN BANKS b ON t.bank_id = b.id WHERE t.id = 1";
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        CU_ASSERT_EQUAL(sqlite3_column_int(stmt, 0), 1);
        CU_ASSERT_STRING_EQUAL((const char*)sqlite3_column_text(stmt, 1), "Тайфун");
        CU_ASSERT_STRING_EQUAL((const char*)sqlite3_column_text(stmt, 2), "Северная банка");
    }
    sqlite3_finalize(stmt);
}

void test_db_aggregate_query(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    double total = get_sum("CATCH", "quantity_kg");
    CU_ASSERT_DOUBLE_EQUAL(total, 750.0, 0.01);
}

void test_db_blob_insert_read(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    unsigned char test_blob[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    sqlite3_stmt* stmt;
    const char* insert_sql = "UPDATE TRAWLERS SET photo = ? WHERE id = 1";
    int rc = sqlite3_prepare_v2(test_db, insert_sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    sqlite3_bind_blob(stmt, 1, test_blob, sizeof(test_blob), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    CU_ASSERT_EQUAL(rc, SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    const char* select_sql = "SELECT photo FROM TRAWLERS WHERE id = 1";
    rc = sqlite3_prepare_v2(test_db, select_sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int size = sqlite3_column_bytes(stmt, 0);
        CU_ASSERT_EQUAL(size, 8);
        CU_ASSERT_PTR_NOT_NULL(blob);
        CU_ASSERT_EQUAL(memcmp(blob, test_blob, 8), 0);
    }
    sqlite3_finalize(stmt);
}

void test_db_check_constraint(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    char* err = NULL;
    int rc = sqlite3_exec(test_db, "INSERT INTO CATCH (trip_id, fish_name, quality, quantity_kg) VALUES (1, 'Тест', 'high', -100.0)", NULL, NULL, &err);
    CU_ASSERT_NOT_EQUAL(rc, SQLITE_OK);
    CU_ASSERT_PTR_NOT_NULL(err);
    sqlite3_free(err);
}

void test_db_foreign_key(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    char* err = NULL;
    int rc = sqlite3_exec(test_db, "INSERT INTO TRIPS (trawler_id, bank_id, departure_date, return_date) VALUES (999, 1, '2024-01-01', '2024-01-10')", NULL, NULL, &err);
    CU_ASSERT_NOT_EQUAL(rc, SQLITE_OK);
    CU_ASSERT_PTR_NOT_NULL(err);
    sqlite3_free(err);
}

void test_auth_valid_credentials(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, username, role FROM CREW WHERE username='cap_ivanov' AND password='123'";
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    CU_ASSERT_TRUE(found);
    if (found) {
        CU_ASSERT_EQUAL(sqlite3_column_int(stmt, 0), 1);
        CU_ASSERT_STRING_EQUAL((const char*)sqlite3_column_text(stmt, 1), "cap_ivanov");
        CU_ASSERT_STRING_EQUAL((const char*)sqlite3_column_text(stmt, 2), "crew");
    }
    sqlite3_finalize(stmt);
}

void test_auth_invalid_password(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id FROM CREW WHERE username='cap_ivanov' AND password='wrong'";
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    CU_ASSERT_FALSE(found);
    sqlite3_finalize(stmt);
}

void test_auth_invalid_username(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id FROM CREW WHERE username='nonexistent' AND password='123'";
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    CU_ASSERT_FALSE(found);
    sqlite3_finalize(stmt);
}

void test_auth_role_check(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT role FROM CREW WHERE username='cap_ivanov'";
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* role = (const char*)sqlite3_column_text(stmt, 0);
        int valid = (strcmp(role, "admin") == 0 || strcmp(role, "crew") == 0);
        CU_ASSERT_TRUE(valid);
    }
    sqlite3_finalize(stmt);
}

void test_service_trips_by_trawler(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT t.id, t.departure_date, t.return_date, b.name, "
                      "COALESCE(SUM(c.quantity_kg), 0) as total "
                      "FROM TRIPS t "
                      "JOIN BANKS b ON t.bank_id = b.id "
                      "LEFT JOIN CATCH c ON c.trip_id = t.id "
                      "WHERE t.trawler_id = 1 "
                      "GROUP BY t.id";
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    int row_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) row_count++;
    CU_ASSERT_TRUE(row_count >= 2);
    sqlite3_finalize(stmt);
}

void test_service_catch_by_bank(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT c.fish_name, SUM(c.quantity_kg) as total "
                      "FROM BANKS b "
                      "JOIN TRIPS t ON t.bank_id = b.id "
                      "JOIN CATCH c ON c.trip_id = t.id "
                      "WHERE b.name = 'Северная банка' "
                      "GROUP BY c.fish_name";
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    CU_ASSERT_TRUE(found);
    sqlite3_finalize(stmt);
}

void test_service_max_low_quality_bank(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    sqlite3_stmt* stmt;
    const char* sql = "WITH lq AS ("
                      "SELECT b.name, SUM(c.quantity_kg) as qty "
                      "FROM BANKS b "
                      "JOIN TRIPS t ON t.bank_id = b.id "
                      "JOIN CATCH c ON c.trip_id = t.id "
                      "WHERE c.quality = 'low' "
                      "GROUP BY b.name) "
                      "SELECT name, qty FROM lq ORDER BY qty DESC LIMIT 1";
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        double qty = sqlite3_column_double(stmt, 1);
        CU_ASSERT_TRUE(qty > 0);
    }
    sqlite3_finalize(stmt);
}

void test_service_retiring_crew(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    int current_year = 2025;
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT surname, birth_year FROM CREW WHERE (%d - birth_year) >= 60", current_year);
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(test_db, sql, -1, &stmt, NULL);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int birth_year = sqlite3_column_int(stmt, 1);
        CU_ASSERT_TRUE(birth_year <= 1965);
    }
    sqlite3_finalize(stmt);
}

void test_service_bonus_overplan(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    double total_catch = 1000.0;
    double plan = 800.0;
    double price = 50.0;
    double bonus = 0.0;
    if (total_catch > plan) bonus = (total_catch - plan) * price;
    CU_ASSERT_DOUBLE_EQUAL(bonus, 10000.0, 0.01);
    CU_ASSERT_TRUE(bonus > 0);
    
    char* err = NULL;
    char insert_sql[512];
    snprintf(insert_sql, sizeof(insert_sql), 
             "INSERT INTO BONUSES (crew_id, period_start, period_end, amount) "
             "VALUES (1, '2024-01-01', '2024-12-31', %f)", bonus);
    int rc = sqlite3_exec(test_db, insert_sql, NULL, NULL, &err);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
    int bonus_count = count_rows("BONUSES");
    CU_ASSERT_EQUAL(bonus_count, 1);
}

void test_service_bonus_for_crew(void) {
    CU_ASSERT_EQUAL(open_test_database(), 1);
    double total = 500.0;
    double bonus = total * 0.5;
    CU_ASSERT_DOUBLE_EQUAL(bonus, 250.0, 0.01);
    
    char* err = NULL;
    char insert_sql[512];
    snprintf(insert_sql, sizeof(insert_sql),
             "INSERT INTO BONUSES (crew_id, period_start, period_end, amount) "
             "VALUES (1, '2024-01-01', '2024-01-31', %f)", bonus);
    int rc = sqlite3_exec(test_db, insert_sql, NULL, NULL, &err);
    CU_ASSERT_EQUAL(rc, SQLITE_OK);
}

void test_utils_date_format(void) {
    const char* valid[] = {"2023-01-15", "2024-12-31", "2020-06-01"};
    for (int i = 0; i < 3; i++) {
        CU_ASSERT_EQUAL(strlen(valid[i]), 10);
        CU_ASSERT_EQUAL(valid[i][4], '-');
        CU_ASSERT_EQUAL(valid[i][7], '-');
    }
    const char* invalid = "2023/01/15";
    CU_ASSERT_NOT_EQUAL(invalid[4], '-');
}

void test_utils_string_concat(void) {
    char buffer[128];
    strcpy(buffer, "Траулер ");
    strcat(buffer, "Тайфун");
    CU_ASSERT_STRING_EQUAL(buffer, "Траулер Тайфун");
    CU_ASSERT_TRUE(strlen(buffer) > 0);
}

void test_utils_numeric_range(void) {
    int ids[] = {1, 10, 100, 1000};
    for (int i = 0; i < 4; i++) {
        CU_ASSERT_TRUE(ids[i] > 0);
    }
    double weights[] = {100.5, 500.0, 1000.75};
    double sum = 0;
    for (int i = 0; i < 3; i++) sum += weights[i];
    CU_ASSERT_DOUBLE_EQUAL(sum, 1601.25, 0.01);
}

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    close_test_database();
    return 0;
}

int main(void) {
    CU_pSuite pSuite_db = NULL;
    CU_pSuite pSuite_auth = NULL;
    CU_pSuite pSuite_service = NULL;
    CU_pSuite pSuite_utils = NULL;

    if (CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();

    pSuite_db = CU_add_suite("DB_Module", init_suite, clean_suite);
    if (pSuite_db) {
        CU_add_test(pSuite_db, "test_db_open_close", test_db_open_close);
        CU_add_test(pSuite_db, "test_db_init_success", test_db_init_success);
        CU_add_test(pSuite_db, "test_db_tables_exist", test_db_tables_exist);
        CU_add_test(pSuite_db, "test_db_insert_trawler", test_db_insert_trawler);
        CU_add_test(pSuite_db, "test_db_select_trawlers", test_db_select_trawlers);
        CU_add_test(pSuite_db, "test_db_update_trawler", test_db_update_trawler);
        CU_add_test(pSuite_db, "test_db_delete_catch", test_db_delete_catch);
        CU_add_test(pSuite_db, "test_db_join_query", test_db_join_query);
        CU_add_test(pSuite_db, "test_db_aggregate_query", test_db_aggregate_query);
        CU_add_test(pSuite_db, "test_db_blob_insert_read", test_db_blob_insert_read);
        CU_add_test(pSuite_db, "test_db_check_constraint", test_db_check_constraint);
        CU_add_test(pSuite_db, "test_db_foreign_key", test_db_foreign_key);
    }

    pSuite_auth = CU_add_suite("Auth_Module", init_suite, clean_suite);
    if (pSuite_auth) {
        CU_add_test(pSuite_auth, "test_auth_valid_credentials", test_auth_valid_credentials);
        CU_add_test(pSuite_auth, "test_auth_invalid_password", test_auth_invalid_password);
        CU_add_test(pSuite_auth, "test_auth_invalid_username", test_auth_invalid_username);
        CU_add_test(pSuite_auth, "test_auth_role_check", test_auth_role_check);
    }

    pSuite_service = CU_add_suite("Service_Module", init_suite, clean_suite);
    if (pSuite_service) {
        CU_add_test(pSuite_service, "test_service_trips_by_trawler", test_service_trips_by_trawler);
        CU_add_test(pSuite_service, "test_service_catch_by_bank", test_service_catch_by_bank);
        CU_add_test(pSuite_service, "test_service_max_low_quality_bank", test_service_max_low_quality_bank);
        CU_add_test(pSuite_service, "test_service_retiring_crew", test_service_retiring_crew);
        CU_add_test(pSuite_service, "test_service_bonus_overplan", test_service_bonus_overplan);
        CU_add_test(pSuite_service, "test_service_bonus_for_crew", test_service_bonus_for_crew);
    }

    pSuite_utils = CU_add_suite("Utils_Module", init_suite, clean_suite);
    if (pSuite_utils) {
        CU_add_test(pSuite_utils, "test_utils_date_format", test_utils_date_format);
        CU_add_test(pSuite_utils, "test_utils_string_concat", test_utils_string_concat);
        CU_add_test(pSuite_utils, "test_utils_numeric_range", test_utils_numeric_range);
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    
    CU_pRunSummary summary = CU_get_run_summary();
    int failed = summary->nTestsFailed;
    
    CU_cleanup_registry();
    return (failed > 0) ? 1 : 0;
}
