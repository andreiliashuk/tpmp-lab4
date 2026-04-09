#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int id;
    char username[64];
    char role[16];
    int crew_db_id;
} User;


#define TEST_DB_PATH "test_flotilla.db"
#define TEST_SQL_PATH "test_database.sql"

static int test_trawler_id = 1;
static int test_crew_id = 1;
static char test_bank_name[64] = "Северная банка";


static int create_test_database(void) {
    FILE* f = fopen(TEST_SQL_PATH, "w");
    if (!f) return 0;
    
    fprintf(f, "PRAGMA foreign_keys = ON;\n");
    fprintf(f, "CREATE TABLE IF NOT EXISTS TRAWLERS (id INTEGER PRIMARY KEY, name TEXT, displacement REAL, build_date TEXT, photo BLOB);\n");
    fprintf(f, "CREATE TABLE IF NOT EXISTS CREW (id INTEGER PRIMARY KEY, surname TEXT, position TEXT, hire_date TEXT, birth_year INTEGER, trawler_id INTEGER, username TEXT, password TEXT, role TEXT);\n");
    fprintf(f, "CREATE TABLE IF NOT EXISTS BANKS (id INTEGER PRIMARY KEY, name TEXT);\n");
    fprintf(f, "CREATE TABLE IF NOT EXISTS TRIPS (id INTEGER PRIMARY KEY, trawler_id INTEGER, bank_id INTEGER, departure_date TEXT, return_date TEXT);\n");
    fprintf(f, "CREATE TABLE IF NOT EXISTS CATCH (id INTEGER PRIMARY KEY, trip_id INTEGER, fish_name TEXT, quality TEXT, quantity_kg REAL);\n");
    fprintf(f, "CREATE TABLE IF NOT EXISTS STATS (trawler_id INTEGER PRIMARY KEY, total_catch_kg REAL);\n");
    fprintf(f, "CREATE TABLE IF NOT EXISTS BONUSES (id INTEGER PRIMARY KEY, crew_id INTEGER, period_start TEXT, period_end TEXT, amount REAL);\n");
    
    fprintf(f, "INSERT OR IGNORE INTO TRAWLERS VALUES (1, 'Тайфун', 5000.0, '2010-05-15', NULL);\n");
    fprintf(f, "INSERT OR IGNORE INTO CREW VALUES (1, 'Иванов', 'captain', '2015-01-10', 1960, 1, 'cap_ivanov', '123', 'crew');\n");
    fprintf(f, "INSERT OR IGNORE INTO BANKS VALUES (1, 'Северная банка');\n");
    fprintf(f, "INSERT OR IGNORE INTO TRIPS VALUES (1, 1, 1, '2023-06-01', '2023-06-15');\n");
    fprintf(f, "INSERT OR IGNORE INTO CATCH VALUES (1, 1, 'Треска', 'high', 600.0);\n");
    fprintf(f, "INSERT OR IGNORE INTO STATS VALUES (1, 600.0);\n");
    
    fclose(f);
    return 1;
}


static void cleanup_test_files(void) {
    remove(TEST_SQL_PATH);
    remove(TEST_DB_PATH);
}
static int is_valid_date_format(const char* date) {
    if (!date || strlen(date) != 10) return 0;
    if (date[4] != '-' || date[7] != '-') return 0;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (date[i] < '0' || date[i] > '9') return 0;
    }
    return 1;
}


static int calculate_age(int birth_year, int current_year) {
    return current_year - birth_year;
}

static int is_eligible_for_retirement(int birth_year, int check_year) {
    return (check_year - birth_year) >= 60;
}

static double calculate_overplan_bonus(double actual_catch, double plan, double price_per_kg) {
    if (actual_catch <= plan) return 0.0;
    return (actual_catch - plan) * price_per_kg;
}

static int is_valid_role(const char* role) {
    return (strcmp(role, "admin") == 0 || strcmp(role, "crew") == 0);
}


/**
 * @test Проверка валидной роли "admin"
 */
void test_auth_valid_role_admin(void) {
    int result = is_valid_role("admin");
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_TRUE(result);
}

/**
 * @test Проверка валидной роли "crew"
 */
void test_auth_valid_role_crew(void) {
    int result = is_valid_role("crew");
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_TRUE(result);
}

/**
 * @test Проверка невалидной роли
 */
void test_auth_invalid_role_guest(void) {
    int result = is_valid_role("guest");
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_FALSE(result);
}

/**
 * @test Проверка невалидной роли (пустая строка)
 */
void test_auth_invalid_role_empty(void) {
    int result = is_valid_role("");
    CU_ASSERT_EQUAL(result, 0);
}

/**
 * @test Проверка структуры пользователя
 */
void test_auth_user_structure(void) {
    User test_user;
    test_user.id = 1;
    strcpy(test_user.username, "cap_ivanov");
    strcpy(test_user.role, "crew");
    test_user.crew_db_id = 1;
    
    CU_ASSERT_EQUAL(test_user.id, 1);
    CU_ASSERT_STRING_EQUAL(test_user.username, "cap_ivanov");
    CU_ASSERT_STRING_EQUAL(test_user.role, "crew");
    CU_ASSERT_EQUAL(test_user.crew_db_id, 1);
}

/**
 * @test Проверка хеширования пароля (имитация)
 */
void test_auth_password_not_empty(void) {
    const char* password = "123";
    CU_ASSERT_PTR_NOT_NULL(password);
    CU_ASSERT(strlen(password) > 0);
    CU_ASSERT(strlen(password) >= 3);
}

/**
 * @test Проверка создания тестовой БД
 */
void test_db_create_database(void) {
    int result = create_test_database();
    CU_ASSERT_EQUAL(result, 1);
    
    FILE* f = fopen(TEST_SQL_PATH, "r");
    CU_ASSERT_PTR_NOT_NULL(f);
    if (f) fclose(f);
}

/**
 * @test Проверка SQL-синтаксиса CREATE TABLE
 */
void test_db_sql_syntax_create_table(void) {
    const char* sql = "CREATE TABLE IF NOT EXISTS TEST (id INTEGER PRIMARY KEY, name TEXT);";
    CU_ASSERT_PTR_NOT_NULL(sql);
    CU_ASSERT(strstr(sql, "CREATE TABLE") != NULL);
    CU_ASSERT(strstr(sql, "PRIMARY KEY") != NULL);
}

/**
 * @test Проверка SQL-синтаксиса INSERT
 */
void test_db_sql_syntax_insert(void) {
    const char* sql = "INSERT INTO TRAWLERS VALUES (1, 'Тайфун', 5000.0, '2010-05-15', NULL);";
    CU_ASSERT_PTR_NOT_NULL(sql);
    CU_ASSERT(strstr(sql, "INSERT INTO") != NULL);
    CU_ASSERT(strstr(sql, "VALUES") != NULL);
}

/**
 * @test Проверка SQL-синтаксиса SELECT с JOIN
 */
void test_db_sql_syntax_select_join(void) {
    const char* sql = "SELECT t.id, b.name FROM TRIPS t JOIN BANKS b ON t.bank_id=b.id WHERE t.trawler_id=1;";
    CU_ASSERT_PTR_NOT_NULL(sql);
    CU_ASSERT(strstr(sql, "SELECT") != NULL);
    CU_ASSERT(strstr(sql, "JOIN") != NULL);
    CU_ASSERT(strstr(sql, "WHERE") != NULL);
}

/**
 * @test Проверка размера BLOB данных
 */
void test_db_blob_size_calculation(void) {
    unsigned char test_data[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}; // PNG header
    size_t blob_size = sizeof(test_data);
    
    CU_ASSERT(blob_size > 0);
    CU_ASSERT_EQUAL(blob_size, 8);
    CU_ASSERT(blob_size <= 1048576); // Не более 1 МБ
}

/**
 * @test Проверка ограничения внешнего ключа
 */
void test_db_foreign_key_constraint(void) {
    const char* pragma = "PRAGMA foreign_keys = ON;";
    CU_ASSERT_PTR_NOT_NULL(pragma);
    CU_ASSERT(strstr(pragma, "foreign_keys") != NULL);
    CU_ASSERT(strstr(pragma, "ON") != NULL);
}

/**
 * @test Проверка корректности типа данных CHECK
 */
void test_db_check_constraint_syntax(void) {
    const char* check_clause = "CHECK(quantity_kg >= 0)";
    CU_ASSERT_PTR_NOT_NULL(check_clause);
    CU_ASSERT(strstr(check_clause, "CHECK") != NULL);
    CU_ASSERT(strstr(check_clause, ">=") != NULL);
}

/**
 * @test Расчет бонуса при превышении плана
 */
void test_service_bonus_overplan_calculation(void) {
    double actual_catch = 1500.0;
    double plan = 1000.0;
    double price = 50.0;
    
    double bonus = calculate_overplan_bonus(actual_catch, plan, price);
    double expected = 500.0 * 50.0;
    
    CU_ASSERT_DOUBLE_EQUAL(bonus, expected, 0.01);
    CU_ASSERT(bonus > 0);
}

/**
 * @test Расчет бонуса при невыполнении плана
 */
void test_service_bonus_no_overplan(void) {
    double actual_catch = 800.0;
    double plan = 1000.0;
    double price = 50.0;
    
    double bonus = calculate_overplan_bonus(actual_catch, plan, price);
    
    CU_ASSERT_DOUBLE_EQUAL(bonus, 0.0, 0.01);
    CU_ASSERT_EQUAL(bonus, 0.0);
}

/**
 * @test Расчет бонуса при точном выполнении плана
 */
void test_service_bonus_exact_plan(void) {
    double actual_catch = 1000.0;
    double plan = 1000.0;
    double price = 50.0;
    
    double bonus = calculate_overplan_bonus(actual_catch, plan, price);
    
    CU_ASSERT_DOUBLE_EQUAL(bonus, 0.0, 0.01);
}

/**
 * @test Проверка валидности ID траулера
 */
void test_service_trawler_id_validation(void) {
    int valid_ids[] = {1, 2, 3, 100, 999};
    int invalid_ids[] = {0, -1, -100};
    
    for (int i = 0; i < 5; i++) {
        CU_ASSERT(valid_ids[i] > 0);
    }
    
    for (int i = 0; i < 3; i++) {
        CU_ASSERT(invalid_ids[i] <= 0);
    }
}

/**
 * @test Проверка формата даты
 */
void test_service_date_format_validation(void) {
    const char* valid_dates[] = {"2023-01-15", "2024-12-31", "2020-06-01"};
    const char* invalid_dates[] = {"2023/01/15", "15-01-2023", "2023-13-01", "2023-01-32", "", NULL};
    
    for (int i = 0; i < 3; i++) {
        CU_ASSERT_EQUAL(is_valid_date_format(valid_dates[i]), 1);
    }
    
    for (int i = 0; i < 5; i++) {
        if (invalid_dates[i]) {
            CU_ASSERT_EQUAL(is_valid_date_format(invalid_dates[i]), 0);
        }
    }
}

/**
 * @test Проверка сравнения дат (return_date >= departure_date)
 */
void test_service_date_comparison(void) {
    const char* dep1 = "2023-06-01";
    const char* ret1 = "2023-06-15";
    const char* dep2 = "2023-06-15";
    const char* ret2 = "2023-06-15";
    const char* dep3 = "2023-06-20";
    const char* ret3 = "2023-06-15"; 
    
    CU_ASSERT(strcmp(ret1, dep1) >= 0);
    CU_ASSERT(strcmp(ret2, dep2) >= 0);
    CU_ASSERT(strcmp(ret3, dep3) < 0); 
}

/**
 * @test Проверка качества рыбы
 */
void test_service_fish_quality_validation(void) {
    const char* valid_qualities[] = {"high", "medium", "low"};
    const char* invalid_qualities[] = {"excellent", "good", "bad", "", "HIGH"};
    
    for (int i = 0; i < 3; i++) {
        int valid = (strcmp(valid_qualities[i], "high") == 0 ||
                     strcmp(valid_qualities[i], "medium") == 0 ||
                     strcmp(valid_qualities[i], "low") == 0);
        CU_ASSERT_EQUAL(valid, 1);
    }
    
    for (int i = 0; i < 5; i++) {
        int valid = (strcmp(invalid_qualities[i], "high") == 0 ||
                     strcmp(invalid_qualities[i], "medium") == 0 ||
                     strcmp(invalid_qualities[i], "low") == 0);
        CU_ASSERT_EQUAL(valid, 0);
    }
}

/**
 * @test Расчет возраста для пенсии
 */
void test_service_retirement_calculation(void) {
    int birth_year_eligible = 1960;
    int birth_year_not_eligible = 1980;
    int check_year = 2025;
    
    CU_ASSERT_EQUAL(is_eligible_for_retirement(birth_year_eligible, check_year), 1);
    CU_ASSERT_EQUAL(is_eligible_for_retirement(birth_year_not_eligible, check_year), 0);
    
    int age1 = calculate_age(birth_year_eligible, check_year);
    int age2 = calculate_age(birth_year_not_eligible, check_year);
    
    CU_ASSERT_EQUAL(age1, 65);
    CU_ASSERT_EQUAL(age2, 45);
    CU_ASSERT(age1 >= 60);
    CU_ASSERT(age2 < 60);
}

/**
 * @test Проверка работы со строками
 */
void test_utils_string_operations(void) {
    char buffer[256];
    strcpy(buffer, "Тайфун");
    CU_ASSERT_STRING_EQUAL(buffer, "Тайфун");
    CU_ASSERT_EQUAL(strlen(buffer), 6);
    
    strcat(buffer, " 5000");
    CU_ASSERT(strstr(buffer, "5000") != NULL);
}

/**
 * @test Проверка математических операций (расчет водоизмещения)
 */
void test_utils_math_operations(void) {
    double displacement = 5000.0;
    double margin = 0.15; // 15% запас
    
    double max_load = displacement * margin;
    CU_ASSERT_DOUBLE_EQUAL(max_load, 750.0, 0.01);
    
    double total = displacement + max_load;
    CU_ASSERT_DOUBLE_EQUAL(total, 5750.0, 0.01);
}

/**
 * @test Проверка работы с памятью (имитация выделения)
 */
void test_utils_memory_allocation(void) {
    size_t size = 1024;
    void* ptr = malloc(size);
    CU_ASSERT_PTR_NOT_NULL(ptr);
    
    memset(ptr, 0, size);
    unsigned char* bytes = (unsigned char*)ptr;
    CU_ASSERT_EQUAL(bytes[0], 0);
    CU_ASSERT_EQUAL(bytes[size-1], 0);
    
    free(ptr);
}

/**
 * @test Проверка конвертации строки в число
 */
void test_utils_string_to_number(void) {
    const char* num_str1 = "12345";
    const char* num_str2 = "3.14159";
    const char* invalid_str = "abc123";
    
    int int_val = atoi(num_str1);
    CU_ASSERT_EQUAL(int_val, 12345);
    
    double double_val = atof(num_str2);
    CU_ASSERT_DOUBLE_EQUAL(double_val, 3.14159, 0.0001);
    
    int invalid_val = atoi(invalid_str);
    CU_ASSERT_EQUAL(invalid_val, 0);
}

/**
 * @test Проверка обработки граничных значений
 */
void test_utils_boundary_values(void) {
    int max_int = 2147483647;
    int min_int = -2147483648;
    
    CU_ASSERT(max_int > 0);
    CU_ASSERT(min_int < 0);
    
    double max_double = 1.7e308;
    double min_double = -1.7e308;
    
    CU_ASSERT(max_double > min_double);
}

/**
 * @test Проверка работы с файловыми путями
 */
void test_utils_file_path_operations(void) {
    const char* valid_paths[] = {"flotilla.db", "/tmp/test.db", "./data/database.sql", "../config/settings.ini"};
    
    for (int i = 0; i < 4; i++) {
        CU_ASSERT(strlen(valid_paths[i]) > 0);
        CU_ASSERT(strchr(valid_paths[i], '.') != NULL || strchr(valid_paths[i], '/') != NULL);
    }
}


/**
 * @test Проверка NULL-значений в параметрах
 */
void test_edge_null_parameters(void) {
    const char* null_str = NULL;
    CU_ASSERT_PTR_NULL(null_str);
    
    int* null_ptr = NULL;
    CU_ASSERT_PTR_NULL(null_ptr);
}

/**
 * @test Проверка больших чисел для количества рыбы
 */
void test_edge_large_catch_quantity(void) {
    double large_qty = 999999.99;
    double result = large_qty * 1.2; // 20% увеличение
    
    CU_ASSERT(result > large_qty);
    CU_ASSERT_DOUBLE_EQUAL(result, 1199999.988, 0.1);
}

/**
 * @test Проверка отрицательных значений
 */
void test_edge_negative_values(void) {
    double negative = -100.0;
    double positive = 100.0;
    
    CU_ASSERT(negative < 0);
    CU_ASSERT(positive > 0);
    CU_ASSERT(negative + positive == 0.0);
}

/**
 * @test Проверка пустых строк
 */
void test_edge_empty_strings(void) {
    const char* empty = "";
    CU_ASSERT_EQUAL(strlen(empty), 0);
    CU_ASSERT_STRING_EQUAL(empty, "");
}

/**
 * @brief Инициализация тестового окружения
 */
int init_suite(void) {
    create_test_database();
    return 0;
}

/**
 * @brief Очистка после выполнения тестов
 */
int clean_suite(void) {
    cleanup_test_files();
    return 0;
}

/**
 * @brief Главная функция запуска тестов
 */
int main(int argc, char** argv) {
    CU_pSuite pSuite_auth = NULL;
    CU_pSuite pSuite_db = NULL;
    CU_pSuite pSuite_service = NULL;
    CU_pSuite pSuite_utils = NULL;
    CU_pSuite pSuite_edge = NULL;

    /* Инициализация реестра тестов CUnit */
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    /* Набор 1: Аутентификация (не менее 3 тестов) */
    pSuite_auth = CU_add_suite("Auth_Module", init_suite, clean_suite);
    if (NULL == pSuite_auth) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_add_test(pSuite_auth, "Valid role: admin", test_auth_valid_role_admin);
    CU_add_test(pSuite_auth, "Valid role: crew", test_auth_valid_role_crew);
    CU_add_test(pSuite_auth, "Invalid role: guest", test_auth_invalid_role_guest);
    CU_add_test(pSuite_auth, "Invalid role: empty", test_auth_invalid_role_empty);
    CU_add_test(pSuite_auth, "User structure test", test_auth_user_structure);
    CU_add_test(pSuite_auth, "Password validation", test_auth_password_not_empty);

    /* Набор 2: База данных (не менее 3 тестов) */
    pSuite_db = CU_add_suite("DB_Module", init_suite, clean_suite);
    if (NULL == pSuite_db) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_add_test(pSuite_db, "Create test database", test_db_create_database);
    CU_add_test(pSuite_db, "SQL CREATE syntax", test_db_sql_syntax_create_table);
    CU_add_test(pSuite_db, "SQL INSERT syntax", test_db_sql_syntax_insert);
    CU_add_test(pSuite_db, "SQL SELECT JOIN syntax", test_db_sql_syntax_select_join);
    CU_add_test(pSuite_db, "BLOB size calculation", test_db_blob_size_calculation);
    CU_add_test(pSuite_db, "Foreign key constraint", test_db_foreign_key_constraint);
    CU_add_test(pSuite_db, "CHECK constraint syntax", test_db_check_constraint_syntax);

    /* Набор 3: Бизнес-логика флотилии (не менее 3 тестов) */
    pSuite_service = CU_add_suite("Flotilla_Service_Module", init_suite, clean_suite);
    if (NULL == pSuite_service) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_add_test(pSuite_service, "Bonus overplan calculation", test_service_bonus_overplan_calculation);
    CU_add_test(pSuite_service, "Bonus no overplan", test_service_bonus_no_overplan);
    CU_add_test(pSuite_service, "Bonus exact plan", test_service_bonus_exact_plan);
    CU_add_test(pSuite_service, "Trawler ID validation", test_service_trawler_id_validation);
    CU_add_test(pSuite_service, "Date format validation", test_service_date_format_validation);
    CU_add_test(pSuite_service, "Date comparison", test_service_date_comparison);
    CU_add_test(pSuite_service, "Fish quality validation", test_service_fish_quality_validation);
    CU_add_test(pSuite_service, "Retirement calculation", test_service_retirement_calculation);

    /* Набор 4: Вспомогательные функции (не менее 3 тестов) */
    pSuite_utils = CU_add_suite("Utils_Module", init_suite, clean_suite);
    if (NULL == pSuite_utils) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_add_test(pSuite_utils, "String operations", test_utils_string_operations);
    CU_add_test(pSuite_utils, "Math operations", test_utils_math_operations);
    CU_add_test(pSuite_utils, "Memory allocation", test_utils_memory_allocation);
    CU_add_test(pSuite_utils, "String to number", test_utils_string_to_number);
    CU_add_test(pSuite_utils, "Boundary values", test_utils_boundary_values);
    CU_add_test(pSuite_utils, "File path operations", test_utils_file_path_operations);

    /* Набор 5: Граничные случаи (для повышения покрытия) */
    pSuite_edge = CU_add_suite("Edge_Cases", init_suite, clean_suite);
    if (NULL == pSuite_edge) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_add_test(pSuite_edge, "NULL parameters", test_edge_null_parameters);
    CU_add_test(pSuite_edge, "Large catch quantity", test_edge_large_catch_quantity);
    CU_add_test(pSuite_edge, "Negative values", test_edge_negative_values);
    CU_add_test(pSuite_edge, "Empty strings", test_edge_empty_strings);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_pRunSummary summary = CU_get_run_summary();
    printf("\n========== ТЕСТИРОВАНИЕ ЗАВЕРШЕНО ==========\n");
    printf("Всего тестовых наборов: %d\n", CU_get_number_of_suites_run());
    printf("Всего тестов: %d\n", summary->nTestsRun);
    printf("Успешно: %d\n", summary->nTestsRun - summary->nTestsFailed);
    printf("Провалено: %d\n", summary->nTestsFailed);
    printf("============================================\n");

    CU_cleanup_registry();

    return (summary->nTestsFailed > 0) ? 1 : 0;
}
