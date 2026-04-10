cmake_minimum_required(VERSION 3.10)
project(FlotillaProject VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_COVERAGE "Enable code coverage instrumentation" ON)

find_package(SQLite3 REQUIRED)

if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -O0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

add_executable(flotilla
    src/main.cpp
    src/auth.cpp
    src/db_manager.cpp
    src/flotilla_service.cpp
)

target_include_directories(flotilla PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(flotilla PRIVATE SQLite::SQLite3)

find_library(CUNIT_LIBRARY NAMES cunit libcunit)
find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h)

if(CUNIT_LIBRARY AND CUNIT_INCLUDE_DIR)
    message(STATUS "CUnit found: ${CUNIT_LIBRARY}")
    
    add_executable(test_runner
        test/test_suite.cpp
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
    
    enable_testing()
    add_test(NAME FlotillaTests COMMAND test_runner)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/database.sql 
               ${CMAKE_CURRENT_BINARY_DIR}/database.sql COPYONLY)
