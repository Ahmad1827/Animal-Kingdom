#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

struct KingdomDataDef {
    std::string id;
    std::string displayName;
    sf::Color color = sf::Color(150, 140, 125);
    float labelRotation = 0.f;
};

struct CountyDataDef {
    int countyId = 0;
    std::string countyName;
    std::string settlementName;
    std::string modernName;
    std::string kingdomId;
    std::string deJureKingdomId;
    int initialOpinion = 20;
    int supplyLimit = 30;
    int fortTier = 0;
    std::vector<sf::Vector2f> points;
    sf::Vector2f center;
};

class WorldMapRepository {
public:
    static WorldMapRepository& getInstance() {
        static WorldMapRepository instance;
        return instance;
    }

    bool load(const std::string& filepath = "assets/data/world_map.json") {
        activeFilepath = filepath;
        if (std::ifstream(filepath).good()) {
            if (loadFromJsonFile(filepath)) {
                return true;
            }
        }
        initDefaults();
        saveToJsonFile(filepath);
        return true;
    }

    const std::string& getPlayerKingdomId() const {
        return playerKingdomId;
    }

    void setPlayerKingdomId(const std::string& id) {
        playerKingdomId = id;
    }

    const std::string& getPlayerCapital() const {
        return playerCapital;
    }

    const std::string& getDefaultAllyKingdom() const {
        return defaultAllyKingdom;
    }

    void setDefaultAllyKingdom(const std::string& id) {
        defaultAllyKingdom = id;
    }

    std::string getKingdomDisplayName(const std::string& id) const {
        auto it = kingdoms.find(id);
        if (it != kingdoms.end() && !it->second.displayName.empty()) {
            return it->second.displayName;
        }
        return "Kingdom of " + id;
    }

    sf::Color getKingdomColor(const std::string& id) const {
        auto it = kingdoms.find(id);
        if (it != kingdoms.end()) {
            return it->second.color;
        }
        for (const auto& pair : kingdoms) {
            if (!pair.first.empty() && id.find(pair.first) != std::string::npos) {
                return pair.second.color;
            }
        }
        return sf::Color(150, 140, 125);
    }

    const std::vector<CountyDataDef>& getCounties() const {
        return counties;
    }

    const std::unordered_map<std::string, KingdomDataDef>& getKingdoms() const {
        return kingdoms;
    }

private:
    WorldMapRepository() {
        initDefaults();
    }

    std::string activeFilepath;
    std::string playerKingdomId = "Wessex";
    std::string playerCapital = "Hampshire";
    std::string defaultAllyKingdom = "Cornwall";
    std::unordered_map<std::string, KingdomDataDef> kingdoms;
    std::vector<CountyDataDef> counties;

    void initDefaults() {
        kingdoms.clear();
        counties.clear();

        kingdoms["Wessex"] = { "Wessex", "Kingdom of Wessex", sf::Color(185, 55, 65), 0.f };
        kingdoms["Mercia"] = { "Mercia", "Kingdom of Mercia", sf::Color(205, 115, 65), -8.f };
        kingdoms["Northumbria"] = { "Northumbria", "Kingdom of Northumbria", sf::Color(155, 65, 95), -14.f };
        kingdoms["Alba"] = { "Alba", "Kingdom of Alba", sf::Color(65, 105, 145), 0.f };
        kingdoms["Cornwall"] = { "Cornwall", "Kingdom of Cornwall", sf::Color(155, 135, 75), -25.f };
        kingdoms["East Anglia"] = { "East Anglia", "Kingdom of East Anglia", sf::Color(210, 160, 60), 15.f };
        kingdoms["Ireland"] = { "Ireland", "Kingdom of Ireland", sf::Color(75, 135, 80), 60.f };
        kingdoms["Cadet Wessex"] = { "Cadet Wessex", "Cadet Wessex", sf::Color(165, 80, 120), 0.f };
        kingdoms["Rebels"] = { "Rebels", "Rebels", sf::Color(150, 25, 30), 0.f };

        auto addDef = [this](int id, const std::string& cName, const std::string& sName, const std::string& mName,
                             const std::string& kId, const std::string& djId, int op, int sup, int fort,
                             const std::vector<sf::Vector2f>& pts) {
            CountyDataDef c;
            c.countyId = id;
            c.countyName = cName;
            c.settlementName = sName;
            c.modernName = mName;
            c.kingdomId = kId;
            c.deJureKingdomId = djId;
            c.initialOpinion = op;
            c.supplyLimit = sup;
            c.fortTier = fort;
            c.points = pts;

            float sumX = 0.f;
            float sumY = 0.f;
            for (const auto& pt : pts) {
                sumX += pt.x;
                sumY += pt.y;
            }
            c.center = sf::Vector2f(sumX / static_cast<float>(pts.size()), sumY / static_cast<float>(pts.size()));
            counties.push_back(c);
        };

        addDef(1, "Cornwall", "Kernow", "Tintagel", "Cornwall", "Cornwall", 15, 22, 0,
               { {260.f, 575.f}, {275.f, 585.f}, {310.f, 570.f}, {340.f, 555.f}, {330.f, 525.f}, {305.f, 532.f} });

        addDef(2, "Hampshire", "Wintanceaster", "Winchester", "Wessex", "Wessex", 65, 45, 0,
               { {330.f, 525.f}, {340.f, 555.f}, {395.f, 560.f}, {415.f, 515.f}, {370.f, 502.f} });

        addDef(3, "Wight", "Hamwic", "Southampton", "Wessex", "Wessex", 40, 25, 0,
               { {395.f, 560.f}, {440.f, 558.f}, {445.f, 520.f}, {415.f, 515.f} });

        addDef(4, "Berkshire", "Readingas", "Reading", "Mercia", "Wessex", -18, 30, 0,
               { {370.f, 502.f}, {415.f, 515.f}, {445.f, 520.f}, {480.f, 485.f}, {470.f, 455.f}, {440.f, 475.f}, {380.f, 480.f} });

        addDef(5, "Middlesex", "Lundenburh", "London", "East Anglia", "Wessex", -25, 40, 0,
               { {445.f, 520.f}, {440.f, 558.f}, {485.f, 555.f}, {535.f, 535.f}, {525.f, 505.f}, {480.f, 485.f} });

        addDef(6, "Norfolk", "Theodford", "Thetford", "East Anglia", "East Anglia", 15, 35, 0,
               { {480.f, 485.f}, {525.f, 505.f}, {575.f, 470.f}, {540.f, 435.f}, {505.f, 425.f}, {470.f, 455.f} });

        addDef(7, "Chester", "Legaceaster", "Chester", "Mercia", "Mercia", 15, 24, 0,
               { {340.f, 420.f}, {360.f, 445.f}, {330.f, 475.f}, {370.f, 500.f}, {380.f, 480.f}, {415.f, 445.f}, {395.f, 395.f}, {365.f, 390.f} });

        addDef(8, "Warwick", "Tamworthig", "Tamworth", "Mercia", "Mercia", 15, 28, 0,
               { {380.f, 480.f}, {440.f, 475.f}, {470.f, 455.f}, {460.f, 380.f}, {410.f, 395.f}, {415.f, 445.f} });

        addDef(9, "Lincoln", "Lindcylene", "Lincoln", "Northumbria", "Mercia", 15, 28, 0,
               { {470.f, 455.f}, {505.f, 425.f}, {525.f, 390.f}, {530.f, 355.f}, {470.f, 345.f}, {460.f, 380.f} });

        addDef(10, "Yorkshire", "Jorvik", "York", "Northumbria", "Northumbria", 15, 38, 0,
               { {410.f, 395.f}, {460.f, 380.f}, {470.f, 345.f}, {530.f, 355.f}, {505.f, 315.f}, {450.f, 305.f}, {395.f, 340.f} });

        addDef(11, "Durham", "Dunholm", "Durham", "Northumbria", "Northumbria", 15, 28, 0,
               { {395.f, 340.f}, {450.f, 305.f}, {505.f, 315.f}, {485.f, 260.f}, {435.f, 255.f}, {385.f, 275.f} });

        addDef(12, "Bamburgh", "Bebbanburg", "Bamburgh", "Northumbria", "Northumbria", 15, 18, 0,
               { {385.f, 275.f}, {435.f, 255.f}, {485.f, 260.f}, {465.f, 225.f}, {420.f, 215.f}, {380.f, 230.f} });

        addDef(13, "Lothian", "Dun Eideann", "Edinburgh", "Northumbria", "Alba", 15, 24, 0,
               { {380.f, 230.f}, {420.f, 215.f}, {465.f, 225.f}, {480.f, 195.f}, {440.f, 175.f}, {385.f, 185.f} });

        addDef(14, "Gowrie", "Sgain", "Scone", "Alba", "Alba", 15, 20, 0,
               { {385.f, 185.f}, {440.f, 175.f}, {480.f, 195.f}, {510.f, 155.f}, {455.f, 145.f}, {460.f, 100.f}, {415.f, 110.f}, {395.f, 150.f}, {375.f, 195.f} });

        addDef(15, "Meath", "Dublin", "Dublin", "Ireland", "Ireland", 15, 28, 0,
               { {230.f, 335.f}, {270.f, 325.f}, {285.f, 360.f}, {275.f, 415.f}, {250.f, 475.f}, {205.f, 485.f}, {180.f, 430.f}, {195.f, 360.f} });
    }

    std::string extractString(const std::string& src, const std::string& key) {
        std::string pattern = "\"" + key + "\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return "";
        size_t colon = src.find(':', p + pattern.size());
        if (colon == std::string::npos) return "";
        size_t q1 = src.find('"', colon + 1);
        if (q1 == std::string::npos) return "";
        size_t q2 = src.find('"', q1 + 1);
        if (q2 == std::string::npos) return "";
        return src.substr(q1 + 1, q2 - q1 - 1);
    }

    int extractInt(const std::string& src, const std::string& key, int defaultVal = 0) {
        std::string pattern = "\"" + key + "\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return defaultVal;
        size_t colon = src.find(':', p + pattern.size());
        if (colon == std::string::npos) return defaultVal;
        size_t start = src.find_first_of("-0123456789", colon + 1);
        if (start == std::string::npos) return defaultVal;
        size_t end = src.find_first_not_of("-0123456789", start);
        return std::stoi(src.substr(start, end - start));
    }

    float extractFloat(const std::string& src, const std::string& key, float defaultVal = 0.f) {
        std::string pattern = "\"" + key + "\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return defaultVal;
        size_t colon = src.find(':', p + pattern.size());
        if (colon == std::string::npos) return defaultVal;
        size_t start = src.find_first_of("-0123456789.", colon + 1);
        if (start == std::string::npos) return defaultVal;
        size_t end = src.find_first_not_of("-0123456789.", start);
        return std::stof(src.substr(start, end - start));
    }

    sf::Color extractColor(const std::string& src) {
        std::string pattern = "\"color\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return sf::Color(150, 140, 125);
        size_t bracket = src.find('[', p);
        if (bracket == std::string::npos) return sf::Color(150, 140, 125);
        size_t endBracket = src.find(']', bracket);
        if (endBracket == std::string::npos) return sf::Color(150, 140, 125);
        std::string sub = src.substr(bracket + 1, endBracket - bracket - 1);
        std::stringstream ss(sub);
        int r = 150, g = 140, b = 125;
        char c1, c2;
        if (ss >> r >> c1 >> g >> c2 >> b) {
            return sf::Color(r, g, b);
        }
        return sf::Color(150, 140, 125);
    }

    std::vector<sf::Vector2f> extractPoints(const std::string& src) {
        std::vector<sf::Vector2f> pts;
        std::string pattern = "\"points\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return pts;
        size_t outerStart = src.find('[', p);
        if (outerStart == std::string::npos) return pts;
        size_t outerEnd = src.find(']', outerStart);
        if (outerEnd == std::string::npos) return pts;
        size_t cur = outerStart + 1;
        while (cur < outerEnd) {
            size_t inStart = src.find('[', cur);
            if (inStart == std::string::npos || inStart > outerEnd) break;
            size_t inEnd = src.find(']', inStart);
            if (inEnd == std::string::npos || inEnd > outerEnd) break;
            std::string pStr = src.substr(inStart + 1, inEnd - inStart - 1);
            std::stringstream ss(pStr);
            float px = 0.f, py = 0.f;
            char comma;
            if (ss >> px >> comma >> py) {
                pts.push_back(sf::Vector2f(px, py));
            }
            cur = inEnd + 1;
        }
        return pts;
    }

    bool loadFromJsonFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        if (content.empty()) return false;

        std::string pk = extractString(content, "playerKingdom");
        if (!pk.empty()) playerKingdomId = pk;
        std::string cap = extractString(content, "playerCapital");
        if (!cap.empty()) playerCapital = cap;
        std::string dak = extractString(content, "defaultAllyKingdom");
        if (!dak.empty()) defaultAllyKingdom = dak;

        size_t kStart = content.find("\"kingdoms\"");
        if (kStart != std::string::npos) {
            size_t kArrStart = content.find('[', kStart);
            size_t kArrEnd = content.find(']', kArrStart);
            if (kArrStart != std::string::npos && kArrEnd != std::string::npos) {
                kingdoms.clear();
                size_t pos = kArrStart;
                while (pos < kArrEnd) {
                    size_t o1 = content.find('{', pos);
                    if (o1 == std::string::npos || o1 > kArrEnd) break;
                    size_t o2 = content.find('}', o1);
                    if (o2 == std::string::npos || o2 > kArrEnd) break;
                    std::string block = content.substr(o1, o2 - o1 + 1);
                    KingdomDataDef kd;
                    kd.id = extractString(block, "id");
                    kd.displayName = extractString(block, "displayName");
                    kd.color = extractColor(block);
                    kd.labelRotation = extractFloat(block, "rotation", 0.f);
                    if (!kd.id.empty()) {
                        kingdoms[kd.id] = kd;
                    }
                    pos = o2 + 1;
                }
            }
        }

        size_t cStart = content.find("\"counties\"");
        if (cStart != std::string::npos) {
            size_t cArrStart = content.find('[', cStart);
            size_t cArrEnd = content.rfind(']');
            if (cArrStart != std::string::npos && cArrEnd != std::string::npos) {
                counties.clear();
                size_t pos = cArrStart;
                while (pos < cArrEnd) {
                    size_t o1 = content.find('{', pos);
                    if (o1 == std::string::npos || o1 > cArrEnd) break;
                    size_t o2 = content.find('}', o1);
                    if (o2 == std::string::npos || o2 > cArrEnd) break;
                    std::string block = content.substr(o1, o2 - o1 + 1);
                    CountyDataDef cd;
                    cd.countyId = extractInt(block, "id", 0);
                    cd.countyName = extractString(block, "countyName");
                    cd.settlementName = extractString(block, "settlementName");
                    cd.modernName = extractString(block, "modernName");
                    cd.kingdomId = extractString(block, "kingdom");
                    cd.deJureKingdomId = extractString(block, "deJureKingdom");
                    cd.initialOpinion = extractInt(block, "opinion", 20);
                    cd.supplyLimit = extractInt(block, "supplyLimit", 30);
                    cd.fortTier = extractInt(block, "fortTier", 0);
                    cd.points = extractPoints(block);

                    if (!cd.points.empty()) {
                        float sumX = 0.f, sumY = 0.f;
                        for (const auto& pt : cd.points) {
                            sumX += pt.x;
                            sumY += pt.y;
                        }
                        cd.center = sf::Vector2f(sumX / static_cast<float>(cd.points.size()),
                                                 sumY / static_cast<float>(cd.points.size()));
                        counties.push_back(cd);
                    }
                    pos = o2 + 1;
                }
            }
        }
        return (!counties.empty() && !kingdoms.empty());
    }

    void saveToJsonFile(const std::string& path) {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        std::ofstream out(path);
        if (!out.is_open()) return;

        out << "{\n";
        out << "  \"playerKingdom\": \"" << playerKingdomId << "\",\n";
        out << "  \"playerCapital\": \"" << playerCapital << "\",\n";
        out << "  \"defaultAllyKingdom\": \"" << defaultAllyKingdom << "\",\n";
        out << "  \"kingdoms\": [\n";

        size_t kIdx = 0;
        for (const auto& pair : kingdoms) {
            const auto& kd = pair.second;
            out << "    { \"id\": \"" << kd.id << "\", \"displayName\": \"" << kd.displayName << "\", "
                << "\"color\": [" << static_cast<int>(kd.color.r) << ", " << static_cast<int>(kd.color.g) << ", " << static_cast<int>(kd.color.b) << "], "
                << "\"rotation\": " << kd.labelRotation << " }"
                << (kIdx + 1 < kingdoms.size() ? "," : "") << "\n";
            kIdx++;
        }

        out << "  ],\n";
        out << "  \"counties\": [\n";

        for (size_t i = 0; i < counties.size(); ++i) {
            const auto& cd = counties[i];
            out << "    {\n";
            out << "      \"id\": " << cd.countyId << ",\n";
            out << "      \"countyName\": \"" << cd.countyName << "\",\n";
            out << "      \"settlementName\": \"" << cd.settlementName << "\",\n";
            out << "      \"modernName\": \"" << cd.modernName << "\",\n";
            out << "      \"kingdom\": \"" << cd.kingdomId << "\",\n";
            out << "      \"deJureKingdom\": \"" << cd.deJureKingdomId << "\",\n";
            out << "      \"opinion\": " << cd.initialOpinion << ",\n";
            out << "      \"supplyLimit\": " << cd.supplyLimit << ",\n";
            out << "      \"fortTier\": " << cd.fortTier << ",\n";
            out << "      \"points\": [";
            for (size_t j = 0; j < cd.points.size(); ++j) {
                out << "[" << static_cast<int>(cd.points[j].x) << ", " << static_cast<int>(cd.points[j].y) << "]"
                    << (j + 1 < cd.points.size() ? ", " : "");
            }
            out << "]\n";
            out << "    }" << (i + 1 < counties.size() ? "," : "") << "\n";
        }

        out << "  ]\n";
        out << "}\n";
    }
};