#pragma once
#include <string>
#include <vector>
#include <functional>
#include <sqlite3.h>

class IDBManager {
public:
    virtual ~IDBManager() = default;
    virtual bool init(const std::string& db_path) = 0;
    virtual bool exec(const std::string& sql) = 0;
    virtual bool query(const std::string& sql, 
                       const std::vector<std::string>& params,
                       std::function<bool(int, char**, char**)> callback) = 0;
    virtual bool insertBlob(const std::string& sql, const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> readBlob(const std::string& sql, int trawler_id) = 0;
};
