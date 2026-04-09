cmake_minimum_required(VERSION 3.10)
project(FlotillaProject VERSION 1.0 LANGUAGES C CXX)

# Стандарты языка
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опция для включения покрытия кода
option(ENABLE_COVERAGE "Enable code coverage instrumentation" ON)

# Поиск SQLite3
find_package(SQLite3 REQUIRED)

# -----------------------------------------------------------------------------
# Флаги покрытия (общие для всего проекта)
# -----------------------------------------------------------------------------
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -O0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# -----------------------------------------------------------------------------
# Основной исполняемый файл
# -----------------------------------------------------------------------------
add_executable(flotilla
    src/main.cpp
    src/auth.cpp
    src/db_manager.cpp
    src/flotilla_service.cpp
)

# Подключение заголовочных файлов
target_include_directories(flotilla PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)

# Линковка SQLite3
target_link_libraries(flotilla PRIVATE SQLite::SQLite3)

# -----------------------------------------------------------------------------
# Модульные тесты (CUnit)
# -----------------------------------------------------------------------------

# Поиск CUnit
find_library(CUNIT_LIBRARY NAMES cunit libcunit)
find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h)

if(CUNIT_LIBRARY AND CUNIT_INCLUDE_DIR)
    message(STATUS "CUnit found: ${CUNIT_LIBRARY}")
    
    # Исполняемый файл тестов
    add_executable(test_runner
        test/test_suite.c
        src/auth.cpp
        src/db_manager.cpp
        src/flotilla_service.cpp
    )
    
    target_include_directories(test_runner PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
        ${CUNIT_INCLUDE_DIR}
    )
    
    target_link_libraries(test_runner PRIVATE 
        SQLite::SQLite3
        ${CUNIT_LIBRARY}
        stdc++
    )
    
    # Добавление теста в CTest
    enable_testing()
    add_test(NAME FlotillaTests COMMAND test_runner)
    
else()
    message(WARNING "CUnit not found. Tests will not be built.")
endif()

# -----------------------------------------------------------------------------
# Копирование database.sql в build директорию
# -----------------------------------------------------------------------------
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/database.sql 
               ${CMAKE_CURRENT_BINARY_DIR}/database.sql COPYONLY)
