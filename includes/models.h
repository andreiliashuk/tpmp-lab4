#pragma once
#include <string>
#include <vector>

struct User {
    int id;
    std::string username;
    std::string role;
    int crew_db_id = 0;
};

struct TripReport {
    int trip_id;
    std::string departure, return_date, bank_name;
    double total_kg;
};
