#include "db_interface.h"
#include <iostream>
#include <fstream>

class DBManager : public IDBManager {
    sqlite3* db = nullptr;
public:
    ~DBManager() override { if(db) sqlite3_close(db); }

    bool init(const std::string& path) override {
        return sqlite3_open(path.c_str(), &db) == SQLITE_OK;
    }

    bool exec(const std::string& sql) override {
        char* err = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
        if (rc != SQLITE_OK) { std::cerr << "SQL Error: " << err << "\n"; sqlite3_free(err); return false; }
        return true;
    }

    bool query(const std::string& sql, const std::vector<std::string>& params,
               std::function<bool(int, char**, char**)> cb) override {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

        for (size_t i = 0; i < params.size(); ++i)
            sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int cols = sqlite3_column_count(stmt);
            char** row = new char*[cols];
            char** names = new char*[cols];
            for (int i = 0; i < cols; ++i) {
                row[i] = const_cast<char*>(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)));
                names[i] = const_cast<char*>(sqlite3_column_name(stmt, i));
            }
            if (!cb(cols, row, names)) { delete[] row; delete[] names; break; }
            delete[] row; delete[] names;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    bool insertBlob(const std::string& sql, const std::vector<uint8_t>& data) override {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_blob(stmt, 1, data.data(), data.size(), SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }

    std::vector<uint8_t> readBlob(const std::string& sql, int trawler_id) override {
        sqlite3_stmt* stmt = nullptr;
        std::vector<uint8_t> res;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return res;
        sqlite3_bind_int(stmt, 1, trawler_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const void* blob = sqlite3_column_blob(stmt, 0);
            int size = sqlite3_column_bytes(stmt, 0);
            if (blob && size > 0) res.assign(static_cast<const uint8_t*>(blob), static_cast<const uint8_t*>(blob) + size);
        }
        sqlite3_finalize(stmt);
        return res;
    }
};

std::shared_ptr<IDBManager> createDB() { return std::make_shared<DBManager>(); }
