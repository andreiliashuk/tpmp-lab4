#pragma once
#include "models.h"
#include <memory>
#include <string>

class IAuthService {
public:
    virtual ~IAuthService() = default;
    virtual bool login(const std::string& username, const std::string& password, User& out_user) = 0;
    virtual bool isAuthenticated() const = 0;
    virtual User getCurrentUser() const = 0;
    virtual void logout() = 0;
};

class IDBManager;
std::shared_ptr<IAuthService> createAuthService(std::shared_ptr<IDBManager> db);
