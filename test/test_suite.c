#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sqlite3 sqlite3;

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

struct User {
    int id;
    const char* username;
    const char* role;
    int crew_db_id;
};

std::shared_ptr<IDBManager> createDB();
std::shared_ptr<IAuthService> createAuthService(std::shared_ptr<IDBManager> db);
std::shared_ptr<IFlotillaService> createFlotillaService(std::shared_ptr<IDBManager> db, const User* u);

#ifdef __cplusplus
}
#endif

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

void test_create_db_manager(void) {
    auto db = createDB();
    CU_ASSERT_PTR_NOT_NULL(db.get());
}

void test_db_manager_init(void) {
    cleanup_test_db();
    auto db = createDB();
    CU_ASSERT_PTR_NOT_NULL(db.get());
    
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    
    bool result = (db.get()->*init)(TEST_DB_PATH);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_TRUE(file_exists(TEST_DB_PATH));
}

void test_db_manager_exec_create_table(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    const char* sql = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, name TEXT);";
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    bool result = (db.get()->*exec)(sql);
    CU_ASSERT_TRUE(result);
}

void test_db_manager_exec_insert(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, name TEXT);");
    bool result = (db.get()->*exec)("INSERT INTO test (name) VALUES ('test_value');");
    CU_ASSERT_TRUE(result);
}

void test_db_manager_query(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("INSERT INTO test (name) VALUES ('query_test');");
    
    int count = 0;
    auto callback = [&count](int, char**, char**) -> bool {
        count++;
        return true;
    };
    
    typedef bool (IDBManager::*QueryFunc)(const std::string&, const std::vector<std::string>&, std::function<bool(int, char**, char**)>);
    QueryFunc query = (QueryFunc)&IDBManager::query;
    bool result = (db.get()->*query)("SELECT * FROM test;", {}, callback);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_TRUE(count > 0);
}

void test_create_auth_service(void) {
    auto db = createDB();
    auto auth = createAuthService(db);
    CU_ASSERT_PTR_NOT_NULL(auth.get());
}

void test_auth_service_login_admin(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    auto auth = createAuthService(db);
    
    typedef bool (IAuthService::*LoginFunc)(const std::string&, const std::string&, User&);
    LoginFunc login = (LoginFunc)&IAuthService::login;
    
    User out_user;
    bool result = (auth.get()->*login)("admin", "admin", out_user);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_STRING_EQUAL(out_user.role, "admin");
}

void test_auth_service_is_authenticated(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    auto auth = createAuthService(db);
    
    typedef bool (IAuthService::*IsAuthFunc)() const;
    IsAuthFunc isAuth = (IsAuthFunc)&IAuthService::isAuthenticated;
    
    CU_ASSERT_FALSE((auth.get()->*isAuth)());
    
    typedef bool (IAuthService::*LoginFunc)(const std::string&, const std::string&, User&);
    LoginFunc login = (LoginFunc)&IAuthService::login;
    User u;
    (auth.get()->*login)("admin", "admin", u);
    
    CU_ASSERT_TRUE((auth.get()->*isAuth)());
}

void test_auth_service_logout(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    auto auth = createAuthService(db);
    
    typedef bool (IAuthService::*LoginFunc)(const std::string&, const std::string&, User&);
    LoginFunc login = (LoginFunc)&IAuthService::login;
    User u;
    (auth.get()->*login)("admin", "admin", u);
    
    typedef void (IAuthService::*LogoutFunc)();
    LogoutFunc logout = (LogoutFunc)&IAuthService::logout;
    (auth.get()->*logout)();
    
    typedef bool (IAuthService::*IsAuthFunc)() const;
    IsAuthFunc isAuth = (IsAuthFunc)&IAuthService::isAuthenticated;
    CU_ASSERT_FALSE((auth.get()->*isAuth)());
}

void test_create_flotilla_service(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, &u);
    CU_ASSERT_PTR_NOT_NULL(svc.get());
}

void test_flotilla_service_get_trips(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);");
    (db.get()->*exec)("INSERT INTO TRAWLERS VALUES (1, 'Test');");
    (db.get()->*exec)("INSERT INTO BANKS VALUES (1, 'Test Bank');");
    (db.get()->*exec)("INSERT INTO TRIPS VALUES (1, 1, 1, '2024-01-01', '2024-01-10');");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, &u);
    
    typedef bool (IFlotillaService::*GetTripsFunc)(int, const std::string&, const std::string&);
    GetTripsFunc getTrips = (GetTripsFunc)&IFlotillaService::getTripsByTrawler;
    bool result = (svc.get()->*getTrips)(1, "2024-01-01", "2024-12-31");
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_get_catch_by_bank(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);");
    (db.get()->*exec)("INSERT INTO TRAWLERS VALUES (1, 'Test');");
    (db.get()->*exec)("INSERT INTO BANKS VALUES (1, 'Test Bank');");
    (db.get()->*exec)("INSERT INTO TRIPS VALUES (1, 1, 1, '2024-01-01', '2024-01-10');");
    (db.get()->*exec)("INSERT INTO CATCH VALUES (1, 1, 'Cod', 'high', 100.0);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, &u);
    
    typedef bool (IFlotillaService::*GetCatchFunc)(const std::string&);
    GetCatchFunc getCatch = (GetCatchFunc)&IFlotillaService::getCatchByBank;
    bool result = (svc.get()->*getCatch)("Test Bank");
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_max_low_quality(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, &u);
    
    typedef bool (IFlotillaService::*GetMaxFunc)();
    GetMaxFunc getMax = (GetMaxFunc)&IFlotillaService::getMaxLowQualityBank;
    bool result = (svc.get()->*getMax)();
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_top_trawler(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS CREW (id INTEGER PRIMARY KEY, surname TEXT, position TEXT, trawler_id INTEGER);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, &u);
    
    typedef bool (IFlotillaService::*GetTopFunc)();
    GetTopFunc getTop = (GetTopFunc)&IFlotillaService::getTopTrawlerInfo;
    bool result = (svc.get()->*getTop)();
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_retiring_crew(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS CREW (id INTEGER PRIMARY KEY, surname TEXT, birth_year INTEGER);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, &u);
    
    typedef bool (IFlotillaService::*GetRetiringFunc)(const std::string&);
    GetRetiringFunc getRetiring = (GetRetiringFunc)&IFlotillaService::getRetiringCrew;
    bool result = (svc.get()->*getRetiring)("2025");
    CU_ASSERT_TRUE(result);
}

void test_flotilla_service_bonus_overplan(void) {
    cleanup_test_db();
    auto db = createDB();
    typedef bool (IDBManager::*InitFunc)(const std::string&);
    InitFunc init = (InitFunc)&IDBManager::init;
    (db.get()->*init)(TEST_DB_PATH);
    
    typedef bool (IDBManager::*ExecFunc)(const std::string&);
    ExecFunc exec = (ExecFunc)&IDBManager::exec;
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS CREW (id INTEGER PRIMARY KEY, trawler_id INTEGER);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, departure_date TEXT, return_date TEXT);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, quantity_kg REAL);");
    (db.get()->*exec)("CREATE TABLE IF NOT EXISTS BONUSES (id INTEGER PRIMARY KEY, crew_id INTEGER, period_start TEXT, period_end TEXT, amount REAL);");
    
    User u;
    u.id = 1;
    u.username = "test";
    u.role = "admin";
    u.crew_db_id = 0;
    
    auto svc = createFlotillaService(db, &u);
    
    typedef bool (IFlotillaService::*BonusFunc)(const std::string&, const std::string&, double, double);
    BonusFunc bonus = (BonusFunc)&IFlotillaService::calculateBonusOverplan;
    bool result = (svc.get()->*bonus)("2024-01-01", "2024-12-31", 1000.0, 50.0);
    CU_ASSERT_TRUE(result);
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
    }

    pSuite = CU_add_suite("Auth_Service_Tests", init_suite, clean_suite);
    if (pSuite) {
        CU_add_test(pSuite, "test_create_auth_service", test_create_auth_service);
        CU_add_test(pSuite, "test_auth_service_login_admin", test_auth_service_login_admin);
        CU_add_test(pSuite, "test_auth_service_is_authenticated", test_auth_service_is_authenticated);
        CU_add_test(pSuite, "test_auth_service_logout", test_auth_service_logout);
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
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    
    CU_pRunSummary summary = CU_get_run_summary();
    int failed = summary->nTestsFailed;
    
    CU_cleanup_registry();
    return (failed > 0) ? 1 : 0;
}
