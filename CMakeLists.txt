cmake_minimum_required(VERSION 3.10)
project(FlotillaProject VERSION 1.0 LANGUAGES C CXX)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(ENABLE_COVERAGE "Enable code coverage instrumentation" ON)


find_package(SQLite3 REQUIRED)


add_executable(flotilla
    src/main.cpp
    src/auth.cpp
    src/db_manager.cpp
    src/flotilla_service.cpp
)


target_include_directories(flotilla PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)


target_link_libraries(flotilla PRIVATE SQLite::SQLite3)


if(ENABLE_COVERAGE)
    target_compile_options(flotilla PRIVATE -fprofile-arcs -ftest-coverage -O0)
    target_link_options(flotilla PRIVATE --coverage)
endif()


find_library(CUNIT_LIBRARY NAMES cunit libcunit)
find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h)

if(CUNIT_LIBRARY AND CUNIT_INCLUDE_DIR)
    message(STATUS "CUnit found: ${CUNIT_LIBRARY}")
    
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
    
    if(ENABLE_COVERAGE)
        target_compile_options(test_runner PRIVATE -fprofile-arcs -ftest-coverage -O0)
        target_link_options(test_runner PRIVATE --coverage)
    endif()
    

    enable_testing()
    add_test(NAME FlotillaTests COMMAND test_runner)
    
else()
    message(WARNING "CUnit not found. Tests will not be built.")
endif()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/database.sql 
               ${CMAKE_CURRENT_BINARY_DIR}/database.sql COPYONLY)


add_custom_target(coverage
    COMMAND lcov --directory . --capture --output-file coverage.info
    COMMAND lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage_filtered.info
    COMMAND lcov --list coverage_filtered.info
    COMMAND echo "Coverage report generated in coverage_filtered.info"
    DEPENDS test_runner
)
