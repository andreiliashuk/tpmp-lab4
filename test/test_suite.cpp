#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <fstream>

#include "db_interface.h"
#include "auth_interface.h"
#include "flotilla_service.h"
#include "models.h"

#define TEST_DB_PATH "test_flotilla.db"

static void cleanup_test_db(void) {
    remove(TEST_DB_PATH);
}

static int file_exists(const char* path) {
    FILE* f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

void loadSQL(std::shared_ptr<IDBManager> db, const std::string& file) {
    std::ifstream f(file);
    if (!f.is_open()) return;
    std::string sql((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    db->exec(sql);
}

void test_create_db_manager(void) {
    auto db = createDB();
    CU_ASSERT_PTR_NOT_NULL(db.get());
}

void test_db_manager_init(void) {
    cleanup_test_db();
    auto db = createDB();
    CU_ASSERT_PTR_NOT_NULL(db.get());
    
    bool result = db->init(TEST_DB_PATH);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_TRUE(file_exists(TEST_DB_PATH));
}

void test_db_manager_exec_create_table(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    const std::string sql = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, name TEXT);";
    bool result = db->exec(sql);
    CU_ASSERT_TRUE(result);
}

void test_db_manager_exec_insert(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, name TEXT);");
    bool result = db->exec("INSERT INTO test (name) VALUES ('test_value');");
    CU_ASSERT_TRUE(result);
}

void test_db_manager_query(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("INSERT INTO test (name) VALUES ('query_test');");
    
    int count = 0;
    auto callback = [&count](int, char**, char**) -> bool {
        count++;
        return true;
    };
    
    bool result = db->query("SELECT * FROM test;", {}, callback);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_TRUE(count > 0);
}

void test_db_manager_blob(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS test_blob (id INTEGER PRIMARY KEY, data BLOB);");
    
    std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    bool result = db->insertBlob("INSERT INTO test_blob (data) VALUES (?)", test_data);
    CU_ASSERT_TRUE(result);
    
    std::vector<uint8_t> read_data = db->readBlob("SELECT data FROM test_blob WHERE id=1", 0);
    CU_ASSERT_EQUAL(read_data.size(), test_data.size());
}

void test_create_auth_service(void) {
    auto db = createDB();
    auto auth = createAuthService(db);
    CU_ASSERT_PTR_NOT_NULL(auth.get());
}

void test_auth_service_login_admin(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    auto auth = createAuthService(db);
    
    User out_user;
    bool result = auth->login("admin", "admin", out_user);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_STRING_EQUAL(out_user.role.c_str(), "admin");
}

void test_auth_service_is_authenticated(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    auto auth = createAuthService(db);
    
    CU_ASSERT_FALSE(auth->isAuthenticated());
    
    User u;
    auth->login("admin", "admin", u);
    
    CU_ASSERT_TRUE(auth->isAuthenticated());
}

void test_auth_service_logout(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    auto auth = createAuthService(db);
    
    User u;
    auth->login("admin", "admin", u);
    CU_ASSERT_TRUE(auth->isAuthenticated());
    
    auth->logout();
    CU_ASSERT_FALSE(auth->isAuthenticated());
}

void test_auth_service_get_current_user(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    auto auth = createAuthService(db);
    
    User u;
    auth->login("admin", "admin", u);
    
    User current = auth->getCurrentUser();
    CU_ASSERT_STRING_EQUAL(current.username.c_str(), "admin");
    CU_ASSERT_STRING_EQUAL(current.role.c_str(), "admin");
}

void test_create_flotilla_service(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    CU_ASSERT_PTR_NOT_NULL(svc.get());
}

void test_flotilla_service_get_trips(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);");
    db->exec("INSERT INTO TRAWLERS VALUES (1, 'Test');");
    db->exec("INSERT INTO BANKS VALUES (1, 'Test Bank');");
    db->exec("INSERT INTO TRIPS VALUES (1, 1, 1, '2024-01-01', '2024-01-10');");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    
    bool result = svc->getTripsByTrawler(1, "2024-01-01", "2024-12-31");
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_get_catch_by_bank(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);");
    db->exec("INSERT INTO TRAWLERS VALUES (1, 'Test');");
    db->exec("INSERT INTO BANKS VALUES (1, 'Test Bank');");
    db->exec("INSERT INTO TRIPS VALUES (1, 1, 1, '2024-01-01', '2024-01-10');");
    db->exec("INSERT INTO CATCH VALUES (1, 1, 'Cod', 'high', 100.0);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    
    bool result = svc->getCatchByBank("Test Bank");
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_max_low_quality(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    
    bool result = svc->getMaxLowQualityBank();
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_top_trawler(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CREW (id INTEGER PRIMARY KEY, surname TEXT, position TEXT, trawler_id INTEGER);");
    db->exec("CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    
    bool result = svc->getTopTrawlerInfo();
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_retiring_crew(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS CREW (id INTEGER PRIMARY KEY, surname TEXT, birth_year INTEGER);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    
    bool result = svc->getRetiringCrew("2025");
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_bonus_overplan(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CREW (id INTEGER PRIMARY KEY, trawler_id INTEGER);");
    db->exec("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, departure_date TEXT, return_date TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, quantity_kg REAL);");
    db->exec("CREATE TABLE IF NOT EXISTS BONUSES (id INTEGER PRIMARY KEY, crew_id INTEGER, period_start TEXT, period_end TEXT, amount REAL);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    
    bool result = svc->calculateBonusOverplan("2024-01-01", "2024-12-31", 1000.0, 50.0);
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_bonus_for_crew(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CREW (id INTEGER PRIMARY KEY, trawler_id INTEGER);");
    db->exec("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, departure_date TEXT, return_date TEXT);");
    db->exec("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, quantity_kg REAL);");
    db->exec("CREATE TABLE IF NOT EXISTS BONUSES (id INTEGER PRIMARY KEY, crew_id INTEGER, period_start TEXT, period_end TEXT, amount REAL);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    
    bool result = svc->calculateBonusForCrew(1, "2024-01-01", "2024-12-31");
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_save_image(void) {
    cleanup_test_db();
    auto db = createDB();
    db->init(TEST_DB_PATH);
    
    db->exec("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT, photo BLOB);");
    db->exec("INSERT INTO TRAWLERS VALUES (1, 'Test', NULL);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, u);
    
    std::ofstream img("test_img.txt");
    img << "test image data";
    img.close();
    
    bool result = svc->saveTrawlerImage(1, "test_img.txt");
    CU_ASSERT_TRUE(result);
    
    remove("test_img.txt");
}

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    cleanup_test_db();
    return 0;
}

int main(void) {
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();

    pSuite = CU_add_suite("DB_Manager_Tests", init_suite, clean_suite);
    if (pSuite) {
        CU_add_test(pSuite, "test_create_db_manager", test_create_db_manager);
        CU_add_test(pSuite, "test_db_manager_init", test_db_manager_init);
        CU_add_test(pSuite, "test_db_manager_exec_create_table", test_db_manager_exec_create_table);
        CU_add_test(pSuite, "test_db_manager_exec_insert", test_db_manager_exec_insert);
        CU_add_test(pSuite, "test_db_manager_query", test_db_manager_query);
        CU_add_test(pSuite, "test_db_manager_blob", test_db_manager_blob);
    }

    pSuite = CU_add_suite("Auth_Service_Tests", init_suite, clean_suite);
    if (pSuite) {
        CU_add_test(pSuite, "test_create_auth_service", test_create_auth_service);
        CU_add_test(pSuite, "test_auth_service_login_admin", test_auth_service_login_admin);
        CU_add_test(pSuite, "test_auth_service_is_authenticated", test_auth_service_is_authenticated);
        CU_add_test(pSuite, "test_auth_service_logout", test_auth_service_logout);
        CU_add_test(pSuite, "test_auth_service_get_current_user", test_auth_service_get_current_user);
    }

    pSuite = CU_add_suite("Flotilla_Service_Tests", init_suite, clean_suite);
    if (pSuite) {
        CU_add_test(pSuite, "test_create_flotilla_service", test_create_flotilla_service);
        CU_add_test(pSuite, "test_flotilla_service_get_trips", test_flotilla_service_get_trips);
        CU_add_test(pSuite, "test_flotilla_service_get_catch_by_bank", test_flotilla_service_get_catch_by_bank);
        CU_add_test(pSuite, "test_flotilla_service_max_low_quality", test_flotilla_service_max_low_quality);
        CU_add_test(pSuite, "test_flotilla_service_top_trawler", test_flotilla_service_top_trawler);
        CU_add_test(pSuite, "test_flotilla_service_retiring_crew", test_flotilla_service_retiring_crew);
        CU_add_test(pSuite, "test_flotilla_service_bonus_overplan", test_flotilla_service_bonus_overplan);
        CU_add_test(pSuite, "test_flotilla_service_bonus_for_crew", test_flotilla_service_bonus_for_crew);
        CU_add_test(pSuite, "test_flotilla_service_save_image", test_flotilla_service_save_image);
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    
    CU_pRunSummary summary = CU_get_run_summary();
    int failed = summary->nTestsFailed;
    
    CU_cleanup_registry();
    return (failed > 0) ? 1 : 0;
}
