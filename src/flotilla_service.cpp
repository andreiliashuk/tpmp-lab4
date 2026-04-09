#include "flotilla_service.h"
#include "models.h"
#include <iostream>
#include <fstream>
#include <iomanip>

class FlotillaService : public IFlotillaService {
    std::shared_ptr<IDBManager> db;
    User user;
public:
    explicit FlotillaService(std::shared_ptr<IDBManager> db_m, const User& u) : db(std::move(db_m)), user(u) {}

    bool getTripsByTrawler(int tid, const std::string& s, const std::string& e) override {
        db->query("SELECT t.id, t.departure_date, t.return_date, b.name, SUM(c.quantity_kg) "
                  "FROM TRIPS t JOIN BANKS b ON t.bank_id=b.id JOIN CATCH c ON c.trip_id=t.id "
                  "WHERE t.trawler_id=? AND t.departure_date>=? AND t.return_date<=? GROUP BY t.id",
                  {std::to_string(tid), s, e}, printCallback);
        return true;
    }

    bool getCatchByBank(const std::string& bank) override {
        db->query("SELECT c.fish_name, SUM(c.quantity_kg) FROM BANKS b "
                  "JOIN TRIPS t ON t.bank_id=b.id JOIN CATCH c ON c.trip_id=t.id "
                  "WHERE b.name=? GROUP BY c.fish_name", {bank}, printCallback);
        return true;
    }

    bool getMaxLowQualityBank() override {
        db->query("WITH lq AS (SELECT b.name, t.id as trip_id, t.departure_date, t.return_date, tr.name as tr, "
                  "SUM(c.quantity_kg) as qty FROM BANKS b JOIN TRIPS t ON t.bank_id=b.id "
                  "JOIN CATCH c ON c.trip_id=t.id JOIN TRAWLERS tr ON tr.id=t.trawler_id "
                  "WHERE c.quality='low' GROUP BY b.name, t.id), mb AS (SELECT name, SUM(qty) as total FROM lq GROUP BY name ORDER BY total DESC LIMIT 1) "
                  "SELECT departure_date, return_date, tr, qty FROM lq JOIN mb ON lq.name=mb.name",
                  {}, printCallback);
        return true;
    }

    bool getTopTrawlerInfo() override {
        db->query("WITH tt AS (SELECT tr.id, SUM(c.quantity_kg) as tot FROM TRAWLERS tr "
                  "JOIN TRIPS t ON t.trawler_id=tr.id JOIN CATCH c ON c.trip_id=t.id GROUP BY tr.id), "
                  "top AS (SELECT id FROM tt ORDER BY tot DESC LIMIT 1) "
                  "SELECT cr.surname, b.name, t.departure_date, t.return_date FROM TRIPS t "
                  "JOIN BANKS b ON t.bank_id=b.id JOIN CREW cr ON cr.trawler_id=t.trawler_id "
                  "WHERE t.trawler_id=(SELECT id FROM top) AND cr.position='captain'",
                  {}, printCallback);
        return true;
    }

    bool getRetiringCrew(const std::string& date) override {
        int year = std::stoi(date.substr(0,4));
        db->query("SELECT surname, birth_year, position, hire_date FROM CREW WHERE (? - birth_year) >= 60",
                  {std::to_string(year)}, printCallback);
        return true;
    }

    bool calculateBonusOverplan(const std::string& s, const std::string& e, double plan, double price) override {
        db->query("SELECT cr.id, COALESCE(SUM(c.quantity_kg),0) as total FROM CREW cr "
                  "JOIN TRAWLERS tr ON tr.id=cr.trawler_id JOIN TRIPS t ON t.trawler_id=tr.id "
                  "JOIN CATCH c ON c.trip_id=t.id WHERE t.departure_date>=? AND t.return_date<=? "
                  "GROUP BY cr.id", {s, e}, [&](int, char** row, char**) {
            double total = std::stod(row[1]);
            if (total > plan) {
                double bonus = (total - plan) * price;
                db->exec("INSERT INTO BONUSES (crew_id, period_start, period_end, amount) VALUES (" +
                         std::string(row[0]) + ", '" + s + "', '" + e + "', " + std::to_string(bonus) + ")");
                std::cout << "Бонус начислен сотруднику ID " << row[0] << ": " << bonus << "\n";
            }
            return true;
        });
        return true;
    }

    bool calculateBonusForCrew(int cid, const std::string& s, const std::string& e) override {
        double total = 0;
        db->query("SELECT COALESCE(SUM(c.quantity_kg),0) FROM CREW cr JOIN TRAWLERS tr ON tr.id=cr.trawler_id "
                  "JOIN TRIPS t ON t.trawler_id=tr.id JOIN CATCH c ON c.trip_id=t.id WHERE cr.id=? "
                  "AND t.departure_date>=? AND t.return_date<=?", 
                  {std::to_string(cid), s, e}, [&](int, char** row, char**){ total = std::stod(row[0]); return false; });
        
        if (total > 0) {
            db->exec("INSERT INTO BONUSES VALUES (NULL, " + std::to_string(cid) + ", '" + s + "', '" + e + "', " + std::to_string(total * 0.5) + ")");
            std::cout << "Персональный бонус сотруднику ID " << cid << " начислен: " << (total * 0.5) << "\n";
        }
        return true;
    }

    bool saveTrawlerImage(int tid, const std::string& path) override {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return db->insertBlob("UPDATE TRAWLERS SET photo=? WHERE id=" + std::to_string(tid), data);
    }

private:
    bool printCallback(int cols, char** row, char** names) {
        for (int i = 0; i < cols; ++i) std::cout << names[i] << ": " << (row[i] ? row[i] : "NULL") << " | ";
        std::cout << "\n";
        return true;
    }
};
