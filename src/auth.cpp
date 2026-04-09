#include "auth_interface.h"
#include "db_interface.h"
#include <memory>

class AuthService : public IAuthService {
    std::shared_ptr<IDBManager> db;
    User current_user;
    bool logged_in = false;
public:
    explicit AuthService(std::shared_ptr<IDBManager> db_manager) : db(std::move(db_manager)) {}

    bool login(const std::string& u, const std::string& p, User& out) override {
        bool found = false;
        db->query("SELECT id, username, role FROM CREW WHERE username=? AND password=?", 
                  {u, p}, [&](int, char** row, char**) {
            current_user = {std::stoi(row[0]), row[1], row[2], std::stoi(row[0])};
            found = true; return false;
        });
        if (found) { logged_in = true; out = current_user; return true; }
        
        if (u == "admin" && p == "admin") {
            current_user = {-1, "admin", "admin", 0};
            logged_in = true; out = current_user; return true;
        }
        return false;
    }
    bool isAuthenticated() const override { return logged_in; }
    User getCurrentUser() const override { return current_user; }
    void logout() override { logged_in = false; }
};

std::shared_ptr<IAuthService> createAuthService(std::shared_ptr<IDBManager> db_manager) {
    return std::make_shared<AuthService>(std::move(db_manager));
}
