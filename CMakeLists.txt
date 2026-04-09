cmake_minimum_required(VERSION 3.15)
project(FishingFlotilla CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(SQLite3 REQUIRED)

add_executable(flotilla_app
    src/main.cpp
    src/auth.cpp
    src/db_manager.cpp
    src/flotilla_service.cpp
)

target_include_directories(flotilla_app PRIVATE ${CMAKE_SOURCE_DIR}/includes)
target_link_libraries(flotilla_app PRIVATE SQLite::SQLite3)
