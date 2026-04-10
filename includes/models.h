#pragma once
#include <string>
#include <vector>

struct User {
    int id = 0;
    std::string username;
    std::string role;
    int crew_db_id = 0;
    
    User() = default;
    User(int i, const std::string& u, const std::string& r, int c) 
        : id(i), username(u), role(r), crew_db_id(c) {}
};

struct TripReport {
    int trip_id = 0;
    std::string departure;
    std::string return_date;
    std::string bank_name;
    double total_kg = 0.0;
};
