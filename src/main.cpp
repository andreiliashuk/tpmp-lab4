#include "db_interface.h"
#include "auth_interface.h"
#include "flotilla_service.h"
#include "models.h"
#include <iostream>
#include <memory>
#include <fstream>

std::shared_ptr<IDBManager> createDB();
class AuthService; 
class FlotillaService;

void loadSQL(std::shared_ptr<IDBManager> db, const std::string& file) {
    std::ifstream f(file);
    std::string sql((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    db->exec(sql);
}

int main() {
    auto db = createDB();
    if (!db->init("flotilla.db")) { std::cerr << "DB init failed\n"; return 1; }
    
    loadSQL(db, "database.sql");

    std::string u, p;
    std::cout << "Fishing Flotilla System\nLogin: "; std::cin >> u;
    std::cout << "Password: "; std::cin >> p;

    auto auth = std::make_shared<AuthService>(db);
    User usr;
    if (!auth->login(u, p, usr)) { std::cout << "Auth failed\n"; return 1; }

    std::cout << "Welcome, " << usr.username << " (" << usr.role << ")\n";
    auto svc = std::make_shared<FlotillaService>(db, usr);

    int choice;
    do {
        std::cout << "\nMenu:\n1. Trips by Trawler\n2. Catch by Bank\n3. Max Low-Quality Bank\n"
                  << "4. Top Trawler Info\n5. Retiring Crew\n6. Bonus Overplan\n7. Bonus Crew(*)\n"
                  << "8. Save Trawler Image (BLOB)\n9. Logout\n0. Exit\nChoice: ";
        std::cin >> choice;
        std::string d1, d2;

        switch(choice) {
            case 1: { int id; std::cout << "Trawler ID: "; std::cin >> id; 
                      std::cout << "Start(YYYY-MM-DD): "; std::cin >> d1;
                      std::cout << "End: "; std::cin >> d2; svc->getTripsByTrawler(id, d1, d2); break; }
            case 2: { std::string bank; std::cout << "Bank Name: "; std::cin >> bank; svc->getCatchByBank(bank); break; }
            case 3: svc->getMaxLowQualityBank(); break;
            case 4: svc->getTopTrawlerInfo(); break;
            case 5: { std::cout << "Check date (YYYY): "; std::cin >> d1; svc->getRetiringCrew(d1); break; }
            case 6: { double plan, price; std::cout << "Start/End, Plan(kg), Price: "; 
                      std::cin >> d1 >> d2 >> plan >> price; svc->calculateBonusOverplan(d1, d2, plan, price); break; }
            case 7: { if (usr.role == "crew" && usr.crew_db_id == 0) { std::cout << "Admin only\n"; break; }
                      int cid = usr.role == "crew" ? usr.crew_db_id : 0;
                      if (usr.role == "admin") { std::cout << "Crew ID: "; std::cin >> cid; }
                      std::cout << "Start/End: "; std::cin >> d1 >> d2;
                      svc->calculateBonusForCrew(cid, d1, d2); break; }
            case 8: { int tid; std::string path; std::cout << "Trawler ID & Image Path: "; std::cin >> tid >> path;
                      svc->saveTrawlerImage(tid, path); break; }
            case 9: auth->logout(); std::cout << "Logged out.\n"; break;
            case 0: std::cout << "Bye.\n"; break;
        }
    } while (choice != 0 && auth->isAuthenticated());

    return 0;
}
