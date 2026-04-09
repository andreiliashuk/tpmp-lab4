#pragma once
#include "db_interface.h"
#include "auth_interface.h"
#include <string>

class IFlotillaService {
public:
    virtual ~IFlotillaService() = default;
    virtual bool getTripsByTrawler(int trawler_id, const std::string& start, const std::string& end) = 0;
    virtual bool getCatchByBank(const std::string& bank_name) = 0;
    virtual bool getMaxLowQualityBank() = 0;
    virtual bool getTopTrawlerInfo() = 0;
    virtual bool getRetiringCrew(const std::string& check_date) = 0;
    virtual bool calculateBonusOverplan(const std::string& start, const std::string& end, 
                                        double plan_kg, double price_per_kg) = 0;
    virtual bool calculateBonusForCrew(int crew_id, const std::string& start, const std::string& end) = 0;
    virtual bool saveTrawlerImage(int trawler_id, const std::string& file_path) = 0;
};
