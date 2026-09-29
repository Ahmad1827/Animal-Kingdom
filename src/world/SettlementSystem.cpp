#include "world/SettlementSystem.h"
#include <cmath>
#include <algorithm>
#include <functional>

SettlementSystem* SettlementSystem::s_instance = nullptr;

SettlementSystem* SettlementSystem::getInstance() {
    return s_instance;
}

void SettlementSystem::startWar(const std::string& county, const std::string& attacker, const std::string& enemy, const std::string& cb) {
    if (!s_instance) return;
    s_instance->activeWar.active = true;
    s_instance->activeWar.targetCounty = county;
    s_instance->activeWar.attackerKingdom = attacker;
    s_instance->activeWar.enemyKingdom = enemy;
    s_instance->activeWar.casusBelli = cb;
    s_instance->activeWar.warScore = 20.f;
    s_instance->activeWar.warTimer = 0.f;
    s_instance->activeWar.aiThinkTimer = 0.f;
    s_instance->peaceModalOpen = false;

    std::string musterCounty = county;
    for (const auto& c : s_instance->counties) {
        if (c.kingdomName == enemy && c.countyName != county) {
            musterCounty = c.countyName;
            break;
        }
    }

    spawnArmy(musterCounty, enemy, 22);
}
void SettlementSystem::assignCouncilMission(sim::CouncilRole role, CouncilMissionType mission, const std::string& county) {
    if (!s_instance) return;
    for (auto& a : s_instance->councilAssignments) {
        if (a.role == role) {
            a.mission = mission;
            a.targetCounty = county;
            a.progress = 0.f;
            return;
        }
    }
    CouncilAssignment a;
    a.role = role;
    a.mission = mission;
    a.targetCounty = county;
    a.progress = 0.f;
    s_instance->councilAssignments.push_back(a);
}

const std::vector<CouncilAssignment>& SettlementSystem::getAllCouncilAssignments() {
    static const std::vector<CouncilAssignment> empty;
    if (!s_instance) return empty;
    return s_instance->councilAssignments;
}

const CouncilAssignment* SettlementSystem::getCouncilAssignment(sim::CouncilRole role) {
    if (!s_instance) return nullptr;
    for (const auto& a : s_instance->councilAssignments) {
        if (a.role == role) return &a;
    }
    return nullptr;
}

void SettlementSystem::spawnArmy(const std::string& county, const std::string& kingdom, int strength) {
    if (!s_instance) return;

    sf::Vector2f spawnPos(440.f, 525.f);
    for (const auto& c : s_instance->counties) {
        if (c.countyName == county) {
            spawnPos = c.center;
            break;
        }
    }

    static uint32_t nextArmyId = 1;
    MapArmy army;
    army.id = nextArmyId++;
    army.ownerKingdom = kingdom;
    army.strength = strength;
    army.pos = spawnPos;
    army.targetPos = spawnPos;
    army.currentCounty = county;
    army.targetCounty = county;
    army.isMoving = false;

    s_instance->mapArmies.push_back(army);
    s_instance->selectedArmyId = static_cast<int>(army.id);
}

void SettlementSystem::annexCounty(const std::string& county, const std::string& newKingdom) {
    if (!s_instance) return;

    for (auto& c : s_instance->counties) {
        if (c.countyName == county) {
            c.kingdomName = newKingdom;
            c.isOccupied = false;
            c.occupierKingdom.clear();
            c.siegeProgress = 0.f;
            break;
        }
    }

    std::string fullKName = (newKingdom == "Wessex") ? "Kingdom of Wessex" : ("Kingdom of " + newKingdom);
    for (auto& s : s_instance->realSettlements) {
        if (s.countyName == county) {
            s.kingdomName = fullKName;
            s.isAllied = (newKingdom == "Wessex");
            break;
        }
    }

    std::string enemyK = s_instance->activeWar.enemyKingdom;
    s_instance->mapArmies.erase(
        std::remove_if(s_instance->mapArmies.begin(), s_instance->mapArmies.end(),
                       [&enemyK](const MapArmy& a) { return a.ownerKingdom == enemyK; }),
        s_instance->mapArmies.end()
    );

    getPlayerClaims().erase(county);
    s_instance->kingdomTruces[enemyK] = 5;
    s_instance->activeWar.active = false;
    s_instance->peaceModalOpen = false;
}

bool SettlementSystem::isAtWarWith(const std::string& kingdom) {
    if (!s_instance || !s_instance->activeWar.active) return false;
    return (s_instance->activeWar.enemyKingdom == kingdom);
}

bool SettlementSystem::hasTruceWith(const std::string& kingdom) {
    if (!s_instance) return false;
    auto it = s_instance->kingdomTruces.find(kingdom);
    return (it != s_instance->kingdomTruces.end() && it->second > 0);
}

SettlementSystem::SettlementSystem() {
    s_instance = this;
    warBadgeBounds = sf::FloatRect(820.f, 580.f, 310.f, 44.f);

    fontLoaded = font.loadFromFile("assets/fonts/Cinzel-Bold.ttf") ||
                 font.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
                 font.loadFromFile("font.ttf") ||
                 font.loadFromFile("assets/fonts/font.ttf");

    buildAuthenticMapGeometry();
    buildOrganicCounties();
}

bool SettlementSystem::pointInPolygon(const std::vector<sf::Vector2f>& poly, sf::Vector2f pt) const {
    bool inside = false;
    size_t n = poly.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        if (((poly[i].y > pt.y) != (poly[j].y > pt.y)) &&
            (pt.x < (poly[j].x - poly[i].x) * (pt.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x)) {
            inside = !inside;
        }
    }
    return inside;
}

void SettlementSystem::buildOrganicCounties() {
    counties.clear();

    auto addCounty = [this](int id, const std::string& cName, const std::string& sName, const std::string& mName,
                            const std::string& deFactoKName, const std::string& deJureKName,
                            const std::vector<sf::Vector2f>& pts) {
        CountyDef c;
        c.countyId = id;
        c.countyName = cName;
        c.settlementName = sName;
        c.modernName = mName;
        c.kingdomName = deFactoKName;
        c.deJureKingdom = deJureKName;
        c.points = pts;

        c.shape.setPointCount(pts.size());
        float sumX = 0.f;
        float sumY = 0.f;
        for (size_t i = 0; i < pts.size(); ++i) {
            c.shape.setPoint(i, pts[i]);
            sumX += pts[i].x;
            sumY += pts[i].y;
        }
        c.center = sf::Vector2f(sumX / static_cast<float>(pts.size()), sumY / static_cast<float>(pts.size()));
        counties.push_back(c);
    };

    addCounty(1, "Cornwall", "Kernow", "Tintagel", "Cornwall", "Cornwall", {
        {260.f, 575.f}, {275.f, 585.f}, {310.f, 570.f}, {340.f, 555.f}, {330.f, 525.f}, {305.f, 532.f}
    });

    addCounty(2, "Hampshire", "Wintanceaster", "Winchester", "Wessex", "Wessex", {
        {330.f, 525.f}, {340.f, 555.f}, {395.f, 560.f}, {415.f, 515.f}, {370.f, 502.f}
    });

    addCounty(3, "Wight", "Hamwic", "Southampton", "Wessex", "Wessex", {
        {395.f, 560.f}, {440.f, 558.f}, {445.f, 520.f}, {415.f, 515.f}
    });

    addCounty(4, "Berkshire", "Readingas", "Reading", "Mercia", "Wessex", {
        {370.f, 502.f}, {415.f, 515.f}, {445.f, 520.f}, {480.f, 485.f}, {470.f, 455.f}, {440.f, 475.f}, {380.f, 480.f}
    });

    addCounty(5, "Middlesex", "Lundenburh", "London", "East Anglia", "Wessex", {
        {445.f, 520.f}, {440.f, 558.f}, {485.f, 555.f}, {535.f, 535.f}, {525.f, 505.f}, {480.f, 485.f}
    });

    addCounty(6, "Norfolk", "Theodford", "Thetford", "East Anglia", "East Anglia", {
        {480.f, 485.f}, {525.f, 505.f}, {575.f, 470.f}, {540.f, 435.f}, {505.f, 425.f}, {470.f, 455.f}
    });

    addCounty(7, "Chester", "Legaceaster", "Chester", "Mercia", "Mercia", {
        {340.f, 420.f}, {360.f, 445.f}, {330.f, 475.f}, {370.f, 500.f}, {380.f, 480.f}, {415.f, 445.f}, {395.f, 395.f}, {365.f, 390.f}
    });

    addCounty(8, "Warwick", "Tamworthig", "Tamworth", "Mercia", "Mercia", {
        {380.f, 480.f}, {440.f, 475.f}, {470.f, 455.f}, {460.f, 380.f}, {410.f, 395.f}, {415.f, 445.f}
    });

    addCounty(9, "Lincoln", "Lindcylene", "Lincoln", "Northumbria", "Mercia", {
        {470.f, 455.f}, {505.f, 425.f}, {525.f, 390.f}, {530.f, 355.f}, {470.f, 345.f}, {460.f, 380.f}
    });

    addCounty(10, "Yorkshire", "Jorvik", "York", "Northumbria", "Northumbria", {
        {410.f, 395.f}, {460.f, 380.f}, {470.f, 345.f}, {530.f, 355.f}, {505.f, 315.f}, {450.f, 305.f}, {395.f, 340.f}
    });

    addCounty(11, "Durham", "Dunholm", "Durham", "Northumbria", "Northumbria", {
        {395.f, 340.f}, {450.f, 305.f}, {505.f, 315.f}, {485.f, 260.f}, {435.f, 255.f}, {385.f, 275.f}
    });

    addCounty(12, "Bamburgh", "Bebbanburg", "Bamburgh", "Northumbria", "Northumbria", {
        {385.f, 275.f}, {435.f, 255.f}, {485.f, 260.f}, {465.f, 225.f}, {420.f, 215.f}, {380.f, 230.f}
    });

    addCounty(13, "Lothian", "Dun Eideann", "Edinburgh", "Northumbria", "Alba", {
        {380.f, 230.f}, {420.f, 215.f}, {465.f, 225.f}, {480.f, 195.f}, {440.f, 175.f}, {385.f, 185.f}
    });

    addCounty(14, "Gowrie", "Sgain", "Scone", "Alba", "Alba", {
        {385.f, 185.f}, {440.f, 175.f}, {480.f, 195.f}, {510.f, 155.f}, {455.f, 145.f}, {460.f, 100.f}, {415.f, 110.f}, {395.f, 150.f}, {375.f, 195.f}
    });

    addCounty(15, "Meath", "Dublin", "Dublin", "Ireland", "Ireland", {
        {230.f, 335.f}, {270.f, 325.f}, {285.f, 360.f}, {275.f, 415.f}, {250.f, 475.f}, {205.f, 485.f}, {180.f, 430.f}, {195.f, 360.f}
    });
}

void SettlementSystem::buildAuthenticMapGeometry() {
    mapOuterVellum.setFillColor(sf::Color(214, 196, 161));
    mapOuterVellum.setOutlineColor(sf::Color(44, 28, 16));
    mapOuterVellum.setOutlineThickness(4.f);

    mapInnerVellum.setFillColor(sf::Color(226, 210, 178));
    mapInnerVellum.setOutlineColor(sf::Color(135, 95, 45));
    mapInnerVellum.setOutlineThickness(2.f);

    mapInnerBorder.setFillColor(sf::Color(186, 204, 208, 170));
    mapInnerBorder.setOutlineColor(sf::Color(90, 65, 35));
    mapInnerBorder.setOutlineThickness(1.5f);

    std::vector<sf::Vector2f> fraPts = {
        {150.f, 642.f}, {320.f, 630.f}, {540.f, 622.f}, {740.f, 630.f},
        {890.f, 646.f}, {890.f, 660.f}, {150.f, 660.f}
    };
    frankiaCoast.setPointCount(fraPts.size());
    for (size_t i = 0; i < fraPts.size(); ++i) frankiaCoast.setPoint(i, fraPts[i]);
    frankiaCoast.setFillColor(sf::Color(175, 155, 130, 180));
    frankiaCoast.setOutlineColor(sf::Color(65, 45, 25));
    frankiaCoast.setOutlineThickness(1.5f);

    std::vector<sf::Vector2f> scaPts = {
        {720.f, 115.f}, {800.f, 105.f}, {890.f, 115.f}, {940.f, 160.f},
        {920.f, 225.f}, {845.f, 230.f}, {780.f, 195.f}, {710.f, 150.f}
    };
    scandiCoast.setPointCount(scaPts.size());
    for (size_t i = 0; i < scaPts.size(); ++i) scandiCoast.setPoint(i, scaPts[i]);
    scandiCoast.setFillColor(sf::Color(170, 150, 130, 180));
    scandiCoast.setOutlineColor(sf::Color(65, 45, 25));
    scandiCoast.setOutlineThickness(1.5f);

    rhumbLines.clear();
    sf::Vector2f roseA(760.f, 380.f);
    sf::Vector2f roseB(320.f, 250.f);
    for (int i = 0; i < 16; ++i) {
        float a = i * (3.14159265f / 8.f);
        sf::Vector2f d(std::cos(a), std::sin(a));
        rhumbLines.push_back(sf::Vertex(roseA, sf::Color(140, 95, 55, 35)));
        rhumbLines.push_back(sf::Vertex(roseA + d * 450.f, sf::Color(140, 95, 55, 0)));
        rhumbLines.push_back(sf::Vertex(roseB, sf::Color(70, 105, 130, 25)));
        rhumbLines.push_back(sf::Vertex(roseB + d * 350.f, sf::Color(70, 105, 130, 0)));
    }

    seaWaves.clear();
    auto addWave = [&](float x, float y) {
        sf::Color wCol(115, 145, 155, 95);
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x, y), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 10.f, y - 3.f), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 10.f, y - 3.f), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 20.f, y), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 20.f, y), wCol));
        seaWaves.push_back(sf::Vertex(sf::Vector2f(x + 30.f, y - 3.f), wCol));
    };
    addWave(630.f, 280.f);
    addWave(680.f, 350.f);
    addWave(610.f, 430.f);
    addWave(720.f, 500.f);
    addWave(250.f, 290.f);
    addWave(310.f, 360.f);

    miniFrameOuter.setSize(sf::Vector2f(276.f, 200.f));
    miniFrameOuter.setFillColor(sf::Color(220, 204, 172));
    miniFrameOuter.setOutlineColor(sf::Color(44, 28, 16));
    miniFrameOuter.setOutlineThickness(2.5f);

    miniFrameInner.setSize(sf::Vector2f(268.f, 192.f));
    miniFrameInner.setFillColor(sf::Color::Transparent);
    miniFrameInner.setOutlineColor(sf::Color(135, 95, 45));
    miniFrameInner.setOutlineThickness(1.f);

    miniSea.setSize(sf::Vector2f(260.f, 142.f));
    miniSea.setFillColor(sf::Color(186, 204, 208, 240));
}

void SettlementSystem::setMapMode(int mode) {
    targetMapMode = mode;
}

void SettlementSystem::syncDynamicVillages(sim::SimulationRegistry& registry) {
    realSettlements.clear();

    auto allVillages = registry.getAllVillages();
    if (allVillages.empty()) return;

    std::vector<sim::VillageData*> sorted;
    for (auto& pair : allVillages) {
        sorted.push_back(&pair.second);
    }
    std::sort(sorted.begin(), sorted.end(), [](const sim::VillageData* a, const sim::VillageData* b) {
        return a->centerX < b->centerX;
    });

    sim::ApeData* controlled = registry.getApe(registry.getControlledApe());

    westCoastX = sorted.front()->borderMinX - 800.f;
    eastCoastX = sorted.back()->borderMaxX + 800.f;

    for (sim::VillageData* v : sorted) {
        RealSettlement rs;
        rs.villageId = v->id;
        rs.kingdomId = v->kingdomId;
        rs.centerX = v->centerX;
        rs.borderLeftX = v->borderMinX;
        rs.borderRightX = v->borderMaxX;
        rs.historicalName = v->name;

        sim::KingdomData* kd = registry.getKingdom(v->kingdomId);
        rs.kingdomName = kd ? ("Kingdom of " + kd->name) : "Wilderness";

        for (auto& c : counties) {
            if (v->name.find(c.settlementName) != std::string::npos || c.settlementName.find(v->name) != std::string::npos) {
                c.villageId = v->id;
                c.kingdomId = v->kingdomId;
                rs.mapCoord = c.center;
                rs.countyName = c.countyName;
                rs.modernName = c.modernName;
                rs.deJureKingdom = c.deJureKingdom;
                rs.hasPlayerClaim = (getPlayerClaims().count(c.countyName) > 0);
                break;
            }
        }
        if (rs.mapCoord.x == 0.f && rs.mapCoord.y == 0.f) {
            rs.mapCoord = {450.f, 350.f};
            rs.countyName = "Province";
            rs.modernName = v->name;
        }

        bool allied = false;
        if (rs.kingdomName.find("Wessex") != std::string::npos || rs.kingdomName.find("Cornwall") != std::string::npos) {
            allied = true;
        } else if (controlled) {
            if (v->id == controlled->villageId || (controlled->currentKingdom != 0 && v->kingdomId == controlled->currentKingdom)) {
                allied = true;
            } else if (v->personalOpinions.count(controlled->id) && v->personalOpinions[controlled->id] >= 30) {
                allied = true;
            }
        }
        rs.isAllied = allied;

        realSettlements.push_back(rs);
    }

    std::sort(realSettlements.begin(), realSettlements.end(), [](const RealSettlement& a, const RealSettlement& b) {
        return a.centerX < b.centerX;
    });

    isInitialized = true;
}

void SettlementSystem::syncWithWorld(sim::SimulationRegistry& registry) {
    syncDynamicVillages(registry);
}

void SettlementSystem::update(float dt, float playerX, sim::SimulationRegistry& registry) {
    if (!isInitialized || realSettlements.size() != registry.getAllVillages().size()) {
        syncDynamicVillages(registry);
    }

    float targetMini = (targetMapMode != 0) ? 1.0f : 0.0f;
    float targetExpand = (targetMapMode == 2) ? 1.0f : 0.0f;
    float animSpeed = 9.0f;

    miniAnimT += (targetMini - miniAnimT) * std::min(1.0f, dt * animSpeed);
    expandAnimT += (targetExpand - expandAnimT) * std::min(1.0f, dt * animSpeed);

    if (std::abs(targetMini - miniAnimT) < 0.002f) miniAnimT = targetMini;
    if (std::abs(targetExpand - expandAnimT) < 0.002f) expandAnimT = targetExpand;

    if (!hasExplored) {
        minExploredX = playerX - 500.f;
        maxExploredX = playerX + 500.f;
        hasExplored = true;
    } else {
        minExploredX = std::min(minExploredX, playerX);
        maxExploredX = std::max(maxExploredX, playerX);
    }

    pulseTime += dt;

    for (auto& a : mapArmies) {
        if (a.isMoving) {
            sf::Vector2f diff = a.targetPos - a.pos;
            float dist = std::hypot(diff.x, diff.y);
            float step = dt * 45.f;
            if (dist <= step || dist < 1.0f) {
                a.pos = a.targetPos;
                a.isMoving = false;
                a.currentCounty = a.targetCounty;
            } else {
                a.pos += (diff / dist) * step;
            }
        }
    }

    if (activeWar.active) {
        activeWar.aiThinkTimer += dt;
        if (activeWar.aiThinkTimer >= 1.5f) {
            activeWar.aiThinkTimer = 0.f;

            sf::Vector2f targetCountyCenter(440.f, 525.f);
            for (const auto& c : counties) {
                if (c.countyName == activeWar.targetCounty) {
                    targetCountyCenter = c.center;
                    break;
                }
            }

            for (auto& a : mapArmies) {
                if (a.ownerKingdom == activeWar.enemyKingdom && !a.inCombat && !a.isMoving) {
                    if (a.currentCounty != activeWar.targetCounty) {
                        a.targetPos = targetCountyCenter;
                        a.targetCounty = activeWar.targetCounty;
                        a.isMoving = true;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < mapArmies.size(); ++i) {
        mapArmies[i].inCombat = false;
    }

    for (size_t i = 0; i < mapArmies.size(); ++i) {
        for (size_t j = i + 1; j < mapArmies.size(); ++j) {
            if (mapArmies[i].ownerKingdom != mapArmies[j].ownerKingdom) {
                if (!mapArmies[i].isMoving && !mapArmies[j].isMoving &&
                    mapArmies[i].currentCounty == mapArmies[j].currentCounty &&
                    !mapArmies[i].currentCounty.empty()) {
                    mapArmies[i].inCombat = true;
                    mapArmies[j].inCombat = true;

                    float dmgToJ = dt * (static_cast<float>(mapArmies[i].strength) * 0.15f + 1.5f);
                    float dmgToI = dt * (static_cast<float>(mapArmies[j].strength) * 0.15f + 1.5f);

                    mapArmies[i].combatTimer += dmgToI;
                    mapArmies[j].combatTimer += dmgToJ;

                    if (mapArmies[i].combatTimer >= 1.0f) {
                        int loss = static_cast<int>(mapArmies[i].combatTimer);
                        mapArmies[i].strength = std::max(0, mapArmies[i].strength - loss);
                        mapArmies[i].combatTimer -= static_cast<float>(loss);
                    }
                    if (mapArmies[j].combatTimer >= 1.0f) {
                        int loss = static_cast<int>(mapArmies[j].combatTimer);
                        mapArmies[j].strength = std::max(0, mapArmies[j].strength - loss);
                        mapArmies[j].combatTimer -= static_cast<float>(loss);
                    }
                }
            }
        }
    }

    for (auto it = mapArmies.begin(); it != mapArmies.end();) {
        if (it->strength <= 0) {
            if (activeWar.active) {
                if (it->ownerKingdom == activeWar.attackerKingdom) {
                    activeWar.warScore = std::max(-100.f, activeWar.warScore - 30.f);
                } else if (it->ownerKingdom == activeWar.enemyKingdom) {
                    activeWar.warScore = std::min(100.f, activeWar.warScore + 30.f);
                }
            }
            if (selectedArmyId == static_cast<int>(it->id)) {
                selectedArmyId = -1;
            }
            it = mapArmies.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& c : counties) {
        bool armyPresent = false;
        bool battleInCounty = false;
        std::string sieger;

        for (const auto& a : mapArmies) {
            if (!a.isMoving && a.currentCounty == c.countyName) {
                armyPresent = true;
                sieger = a.ownerKingdom;
                if (a.inCombat) battleInCounty = true;
            }
        }

        if (activeWar.active && armyPresent && !battleInCounty && !c.isOccupied) {
            if (sieger == activeWar.attackerKingdom && (c.kingdomName == activeWar.enemyKingdom || c.countyName == activeWar.targetCounty)) {
                c.siegeProgress = std::min(100.f, c.siegeProgress + dt * 8.f);
                if (c.siegeProgress >= 100.f) {
                    c.isOccupied = true;
                    c.occupierKingdom = sieger;
                    c.siegeProgress = 0.f;
                    if (c.countyName == activeWar.targetCounty) {
                        activeWar.warScore = 100.f;
                    } else {
                        activeWar.warScore = std::min(100.f, activeWar.warScore + 35.f);
                    }
                }
            }
        } else if ((!armyPresent || battleInCounty) && c.siegeProgress > 0.f && !c.isOccupied) {
            c.siegeProgress = std::max(0.f, c.siegeProgress - dt * 4.f);
        }
    }

    int currentIdx = -1;
    for (size_t i = 0; i < realSettlements.size(); ++i) {
        if (playerX >= realSettlements[i].borderLeftX && playerX <= realSettlements[i].borderRightX) {
            currentIdx = static_cast<int>(i);
            break;
        }
    }

    if (currentIdx != activeSettlementIdx) {
        if (currentIdx != -1) {
            bannerOldName = realSettlements[currentIdx].historicalName;
            bannerModernName = realSettlements[currentIdx].modernName;
            bannerKingdom = realSettlements[currentIdx].kingdomName;
            bannerAllied = realSettlements[currentIdx].isAllied;
            isExiting = false;
            bannerTimer = 0.f;
            showBanner = true;
        } else if (activeSettlementIdx != -1 && activeSettlementIdx < static_cast<int>(realSettlements.size())) {
            bannerOldName = realSettlements[activeSettlementIdx].historicalName;
            bannerModernName = realSettlements[activeSettlementIdx].modernName;
            bannerKingdom = realSettlements[activeSettlementIdx].kingdomName;
            bannerAllied = realSettlements[activeSettlementIdx].isAllied;
            isExiting = true;
            bannerTimer = 0.f;
            showBanner = true;
        }
        activeSettlementIdx = currentIdx;
    }

    if (showBanner) {
        bannerTimer += dt;
        if (bannerTimer >= 5.2f) {
            showBanner = false;
        }
    }
}

void SettlementSystem::draw(sf::RenderWindow& window, const sf::View& letterboxView) {
    if (!fontLoaded || !showBanner || expandAnimT > 0.35f) return;

    window.setView(letterboxView);

    float alpha = 0.f;
    float morphT = 0.f;

    if (bannerTimer < 0.5f) {
        alpha = bannerTimer / 0.5f;
    } else if (bannerTimer < 1.9f) {
        alpha = 1.0f;
    } else if (bannerTimer < 3.1f) {
        alpha = 1.0f;
        morphT = (bannerTimer - 1.9f) / 1.2f;
    } else if (bannerTimer < 4.2f) {
        alpha = 1.0f;
        morphT = 1.0f;
    } else {
        alpha = 1.0f - ((bannerTimer - 4.2f) / 1.0f);
        morphT = 1.0f;
    }

    alpha = std::clamp(alpha, 0.0f, 1.0f);
    morphT = std::clamp(morphT, 0.0f, 1.0f);
    sf::Uint8 byteAlpha = static_cast<sf::Uint8>(alpha * 255);

    float centerY = 88.f;
    std::string prefix = isExiting ? "Departing Boundary of " : "Entering Realm of ";
    sf::Color realmColor = bannerAllied ? sf::Color(140, 215, 125, byteAlpha) : sf::Color(225, 150, 90, byteAlpha);

    sf::Text kingdomText(prefix + bannerKingdom, font, 13);
    kingdomText.setStyle(sf::Text::Italic);
    kingdomText.setFillColor(realmColor);
    kingdomText.setOutlineColor(sf::Color(0, 0, 0, byteAlpha));
    kingdomText.setOutlineThickness(1.5f);
    sf::FloatRect kb = kingdomText.getLocalBounds();
    kingdomText.setOrigin(kb.left + kb.width / 2.f, kb.top + kb.height / 2.f);
    kingdomText.setPosition(640.f, centerY - 24.f);
    window.draw(kingdomText);

    if (morphT < 1.0f) {
        sf::Uint8 oldAlpha = static_cast<sf::Uint8>((1.0f - morphT) * 255 * alpha);
        sf::Text oldText(bannerOldName, font, 24);
        oldText.setStyle(sf::Text::Bold);
        oldText.setFillColor(sf::Color(255, 225, 140, oldAlpha));
        oldText.setOutlineColor(sf::Color(0, 0, 0, oldAlpha));
        oldText.setOutlineThickness(2.0f);
        sf::FloatRect ob = oldText.getLocalBounds();
        oldText.setOrigin(ob.left + ob.width / 2.f, ob.top + ob.height / 2.f);
        oldText.setPosition(640.f, centerY + 8.f);
        window.draw(oldText);
    }

    if (morphT > 0.0f) {
        sf::Uint8 modAlpha = static_cast<sf::Uint8>(morphT * 255 * alpha);
        sf::Text modernText(bannerModernName, font, 24);
        modernText.setStyle(sf::Text::Bold);
        modernText.setFillColor(sf::Color(245, 245, 250, modAlpha));
        modernText.setOutlineColor(sf::Color(0, 0, 0, modAlpha));
        modernText.setOutlineThickness(2.0f);
        sf::FloatRect mb = modernText.getLocalBounds();
        modernText.setOrigin(mb.left + mb.width / 2.f, mb.top + mb.height / 2.f);
        modernText.setPosition(640.f, centerY + 8.f);
        window.draw(modernText);
    }

    float lineHalfW = 150.f * alpha;
    sf::Color lineClr = bannerAllied ? sf::Color(120, 200, 110, byteAlpha) : sf::Color(210, 140, 80, byteAlpha);
    sf::Color edgeClr = bannerAllied ? sf::Color(120, 200, 110, 0) : sf::Color(210, 140, 80, 0);

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(640.f - lineHalfW, centerY + 32.f), edgeClr),
        sf::Vertex(sf::Vector2f(640.f, centerY + 32.f), lineClr),
        sf::Vertex(sf::Vector2f(640.f + lineHalfW, centerY + 32.f), edgeClr)
    };
    window.draw(line, 3, sf::LinesStrip);
}

void SettlementSystem::drawCoast(sf::RenderTarget& rt, const sf::FloatRect& viewBounds, float groundY, float timeOfDay, const sf::Texture* skyTex, const sf::View* cameraView) {
    (void)viewBounds;
    (void)groundY;
    (void)timeOfDay;
    (void)skyTex;
    (void)cameraView;
    (void)rt;
}

sf::Vector2f SettlementSystem::getPlayerMapCoord(float playerX) const {
    if (realSettlements.empty()) return sf::Vector2f(408.f, 525.f);
    if (playerX <= realSettlements.front().centerX) return realSettlements.front().mapCoord;
    if (playerX >= realSettlements.back().centerX) return realSettlements.back().mapCoord;

    for (size_t i = 0; i + 1 < realSettlements.size(); ++i) {
        if (playerX >= realSettlements[i].centerX && playerX <= realSettlements[i + 1].centerX) {
            float dist = realSettlements[i + 1].centerX - realSettlements[i].centerX;
            float t = (dist > 0.001f) ? (playerX - realSettlements[i].centerX) / dist : 0.f;
            return sf::Vector2f(
                realSettlements[i].mapCoord.x + t * (realSettlements[i + 1].mapCoord.x - realSettlements[i].mapCoord.x),
                realSettlements[i].mapCoord.y + t * (realSettlements[i + 1].mapCoord.y - realSettlements[i].mapCoord.y)
            );
        }
    }
    return realSettlements.back().mapCoord;
}

void SettlementSystem::drawMinimap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry) {
    drawMap(window, letterboxView, playerX, registry);
}

void SettlementSystem::drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX) {
    sim::SimulationRegistry dummy;
    drawMap(window, letterboxView, playerX, dummy);
}

void SettlementSystem::drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry) {
    drawMap(window, letterboxView, playerX, registry);
}

bool SettlementSystem::handleMapLensInput(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView) {
    if (expandAnimT < 0.70f) return false;

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Q) { currentLens = MapLens::DeFacto; return true; }
        if (event.key.code == sf::Keyboard::W) { currentLens = MapLens::DeJure; return true; }
        if (event.key.code == sf::Keyboard::E) { currentLens = MapLens::Vassals; return true; }
        if (event.key.code == sf::Keyboard::R) { currentLens = MapLens::Diplomacy; return true; }
        if (event.key.code == sf::Keyboard::T) { currentLens = MapLens::Economy; return true; }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i clickPixel(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f uiCoords = window.mapPixelToCoords(clickPixel, letterboxView);
        for (int i = 0; i < 5; ++i) {
            if (lensTabBounds[i].contains(uiCoords)) {
                currentLens = static_cast<MapLens>(i);
                return true;
            }
        }
    }
    return false;
}

bool SettlementSystem::handleWorldMapInput(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView,
                                           const std::function<void(const RealSettlement&, bool isKingdomLevel)>& onSettlementClicked,
                                           const std::function<void(const RealSettlement&, sf::Vector2f, bool isKingdomLevel)>& onSettlementRightClicked) {
    if (expandAnimT < 0.80f) return false;

    sf::FloatRect canvasRect(134.f, 108.f, 1012.f, 524.f);
    bool isKingdomLevel = (mapZoom > 0.85f);

    if (event.type == sf::Event::MouseWheelScrolled) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseWheelScroll.x, event.mouseWheelScroll.y), letterboxView);
        if (canvasRect.contains(mPos)) {
            mapZoom = std::clamp(mapZoom - event.mouseWheelScroll.delta * 0.12f, 0.45f, 1.85f);
            return true;
        }
    }

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), letterboxView);
        if (canvasRect.contains(mPos)) {
            float relX = mPos.x - canvasRect.left;
            float relY = mPos.y - canvasRect.top;
            sf::Vector2f worldM = mapCenter + sf::Vector2f(relX - canvasRect.width * 0.5f, relY - canvasRect.height * 0.5f) * mapZoom;

            hoveredCountyIdx = -1;
            hoveredKingdomName = "";
            for (size_t i = 0; i < counties.size(); ++i) {
                if (pointInPolygon(counties[i].points, worldM)) {
                    hoveredCountyIdx = static_cast<int>(i);
                    hoveredKingdomName = counties[i].kingdomName;
                    break;
                }
            }
        } else {
            hoveredCountyIdx = -1;
            hoveredKingdomName = "";
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && (event.mouseButton.button == sf::Mouse::Left || event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right)) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);
        if (canvasRect.contains(mPos)) {
            isDraggingMap = (event.mouseButton.button != sf::Mouse::Right);
            lastDragMouse = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
            dragStartMouse = lastDragMouse;
            return (event.mouseButton.button != sf::Mouse::Right);
        }
    }

    if (event.type == sf::Event::MouseMoved && isDraggingMap) {
        sf::Vector2i curMouse(event.mouseMove.x, event.mouseMove.y);
        sf::Vector2f curWorld = window.mapPixelToCoords(curMouse, letterboxView);
        sf::Vector2f lastWorld = window.mapPixelToCoords(lastDragMouse, letterboxView);
        sf::Vector2f delta = (curWorld - lastWorld) * mapZoom;

        mapCenter -= delta;
        mapCenter.x = std::clamp(mapCenter.x, 180.f, 780.f);
        mapCenter.y = std::clamp(mapCenter.y, 80.f, 650.f);

        lastDragMouse = curMouse;
        return true;
    }

    if (event.type == sf::Event::MouseButtonReleased) {
        if (isDraggingMap) isDraggingMap = false;

        int dx = event.mouseButton.x - dragStartMouse.x;
        int dy = event.mouseButton.y - dragStartMouse.y;
        bool isClick = (dx * dx + dy * dy < 36);

        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);

        if (isClick && activeWar.active && warBadgeBounds.contains(mPos)) {
            peaceModalOpen = !peaceModalOpen;
            return true;
        }

        if (isClick && peaceModalOpen) {
            if (closePeaceModalBounds.contains(mPos)) {
                peaceModalOpen = false;
                return true;
            }

            if (enforceBtnBounds.contains(mPos) && activeWar.warScore >= 75.f) {
                annexCounty(activeWar.targetCounty, activeWar.attackerKingdom);
                return true;
            }

            if (whitePeaceBtnBounds.contains(mPos)) {
                for (auto& c : counties) {
                    c.isOccupied = false;
                    c.occupierKingdom.clear();
                    c.siegeProgress = 0.f;
                }
                std::string enemyK = activeWar.enemyKingdom;
                mapArmies.erase(
                    std::remove_if(mapArmies.begin(), mapArmies.end(),
                                   [&enemyK](const MapArmy& a) { return a.ownerKingdom == enemyK; }),
                    mapArmies.end()
                );
                kingdomTruces[enemyK] = 3;
                activeWar.active = false;
                peaceModalOpen = false;
                return true;
            }

            if (surrenderBtnBounds.contains(mPos)) {
                for (auto& c : counties) {
                    c.isOccupied = false;
                    c.occupierKingdom.clear();
                    c.siegeProgress = 0.f;
                }
                std::string enemyK = activeWar.enemyKingdom;
                mapArmies.erase(
                    std::remove_if(mapArmies.begin(), mapArmies.end(),
                                   [&enemyK](const MapArmy& a) { return a.ownerKingdom == enemyK; }),
                    mapArmies.end()
                );
                kingdomTruces[enemyK] = 5;
                activeWar.active = false;
                peaceModalOpen = false;
                return true;
            }

            sf::FloatRect modalRect(390.f, 175.f, 500.f, 335.f);
            if (modalRect.contains(mPos)) {
                return true;
            }
        }

        if (isClick && canvasRect.contains(mPos)) {
            float relX = mPos.x - canvasRect.left;
            float relY = mPos.y - canvasRect.top;
            sf::Vector2f worldClick = mapCenter + sf::Vector2f(relX - canvasRect.width * 0.5f, relY - canvasRect.height * 0.5f) * mapZoom;

            if (event.mouseButton.button == sf::Mouse::Left) {
                for (const auto& a : mapArmies) {
                    float dist = std::hypot(a.pos.x - worldClick.x, a.pos.y - worldClick.y);
                    if (dist <= 18.f) {
                        selectedArmyId = static_cast<int>(a.id);
                        return true;
                    }
                }
            }

            if (event.mouseButton.button == sf::Mouse::Right && selectedArmyId != -1) {
                for (size_t i = 0; i < counties.size(); ++i) {
                    if (pointInPolygon(counties[i].points, worldClick)) {
                        for (auto& a : mapArmies) {
                            if (static_cast<int>(a.id) == selectedArmyId) {
                                a.targetPos = counties[i].center;
                                a.targetCounty = counties[i].countyName;
                                a.isMoving = true;
                                break;
                            }
                        }
                        selectedArmyId = -1;
                        return true;
                    }
                }
            }

            for (size_t i = 0; i < counties.size(); ++i) {
                if (pointInPolygon(counties[i].points, worldClick)) {
                    RealSettlement dummyRs;
                    dummyRs.villageId = counties[i].villageId;
                    dummyRs.kingdomId = counties[i].kingdomId;
                    dummyRs.historicalName = counties[i].settlementName;
                    dummyRs.modernName = counties[i].modernName;
                    dummyRs.kingdomName = counties[i].kingdomName;
                    dummyRs.deJureKingdom = counties[i].deJureKingdom;
                    dummyRs.countyName = counties[i].countyName;
                    dummyRs.mapCoord = counties[i].center;
                    dummyRs.hasPlayerClaim = (getPlayerClaims().count(counties[i].countyName) > 0);

                    for (const auto& s : realSettlements) {
                        if (counties[i].villageId != 0 && s.villageId == counties[i].villageId) {
                            dummyRs.villageId = s.villageId;
                            dummyRs.centerX = s.centerX;
                            break;
                        } else if (!counties[i].settlementName.empty() && s.historicalName == counties[i].settlementName) {
                            dummyRs.villageId = s.villageId;
                            dummyRs.centerX = s.centerX;
                            break;
                        }
                    }
                    dummyRs.kingdomName = counties[i].kingdomName;
                    dummyRs.deJureKingdom = counties[i].deJureKingdom;
                    dummyRs.countyName = counties[i].countyName;

                    if (event.mouseButton.button == sf::Mouse::Left && onSettlementClicked) {
                        onSettlementClicked(dummyRs, isKingdomLevel);
                        return true;
                    } else if (event.mouseButton.button == sf::Mouse::Right && onSettlementRightClicked) {
                        onSettlementRightClicked(dummyRs, mPos, isKingdomLevel);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void SettlementSystem::drawMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry) {
    (void)registry;
    if (!fontLoaded) return;
    if (miniAnimT < 0.005f && expandAnimT < 0.005f) return;

    window.setView(letterboxView);

    auto getKingdomBaseColor = [&](const std::string& kName) -> sf::Color {
        if (kName.find("Wessex") != std::string::npos) return sf::Color(185, 55, 65);
        if (kName.find("Mercia") != std::string::npos) return sf::Color(205, 115, 65);
        if (kName.find("Northumbria") != std::string::npos) return sf::Color(155, 65, 95);
        if (kName.find("Alba") != std::string::npos) return sf::Color(65, 105, 145);
        if (kName.find("Cornwall") != std::string::npos) return sf::Color(155, 135, 75);
        if (kName.find("East Anglia") != std::string::npos) return sf::Color(210, 160, 60);
        if (kName.find("Ireland") != std::string::npos) return sf::Color(75, 135, 80);
        return sf::Color(150, 140, 125);
    };

    if (expandAnimT <= 0.005f) {
        float ySlide = (1.0f - miniAnimT) * -220.f;
        sf::Vector2f originPos(984.f, 16.f + ySlide);

        miniFrameOuter.setPosition(originPos);
        miniFrameInner.setPosition(originPos.x + 4.f, originPos.y + 4.f);
        miniSea.setPosition(originPos.x + 8.f, originPos.y + 26.f);

        window.draw(miniFrameOuter);
        window.draw(miniFrameInner);
        window.draw(miniSea);

        sf::Text miniTitle("MAP OF REALMS", font, 11);
        miniTitle.setStyle(sf::Text::Bold);
        miniTitle.setFillColor(sf::Color(65, 42, 20));
        miniTitle.setPosition(originPos.x + 12.f, originPos.y + 8.f);
        window.draw(miniTitle);

        sf::Text tabPrompt("[TAB] Enlarge", font, 9);
        tabPrompt.setFillColor(sf::Color(115, 85, 50));
        tabPrompt.setPosition(originPos.x + 190.f, originPos.y + 10.f);
        window.draw(tabPrompt);

        sf::Vector2f miniCenter(originPos.x + 138.f, originPos.y + 98.f);
        sf::Vector2f mapCenterRef(377.5f, 342.5f);
        float miniScale = 0.20f;

        for (const auto& c : counties) {
            sf::ConvexShape miniC(c.points.size());
            for (size_t p = 0; p < c.points.size(); ++p) {
                sf::Vector2f pt = miniCenter + (c.points[p] - mapCenterRef) * miniScale;
                miniC.setPoint(p, pt);
            }
            sf::Color col = getKingdomBaseColor(c.kingdomName);
            miniC.setFillColor(col);
            miniC.setOutlineColor(sf::Color(45, 28, 16, 170));
            miniC.setOutlineThickness(0.7f);
            window.draw(miniC);
        }

        for (const auto& rs : realSettlements) {
            sf::Vector2f mDot = miniCenter + (rs.mapCoord - mapCenterRef) * miniScale;
            sf::CircleShape dot(1.8f);
            dot.setOrigin(1.8f, 1.8f);
            dot.setPosition(mDot);
            dot.setFillColor(sf::Color(250, 240, 210));
            dot.setOutlineColor(sf::Color(30, 18, 10));
            dot.setOutlineThickness(0.6f);
            window.draw(dot);
        }

        sf::Vector2f pMap = getPlayerMapCoord(playerX);
        sf::Vector2f pMini = miniCenter + (pMap - mapCenterRef) * miniScale;

        float pulse = 1.0f + 0.35f * std::sin(pulseTime * 6.f);
        sf::CircleShape aura(4.5f * pulse);
        aura.setOrigin(aura.getRadius(), aura.getRadius());
        aura.setPosition(pMini);
        aura.setFillColor(sf::Color(230, 160, 30, 100));
        window.draw(aura);

        sf::CircleShape pin(2.8f);
        pin.setOrigin(2.8f, 2.8f);
        pin.setPosition(pMini);
        pin.setFillColor(sf::Color(255, 220, 50));
        pin.setOutlineColor(sf::Color(30, 15, 5));
        pin.setOutlineThickness(1.f);
        window.draw(pin);

        return;
    }

    const unsigned int canvasW = 1012;
    const unsigned int canvasH = 524;

    if (!mapCanvasReady || mapCanvas.getSize().x != canvasW || mapCanvas.getSize().y != canvasH) {
        mapCanvas.create(canvasW, canvasH);
        mapCanvas.setSmooth(true);
        mapCanvasReady = true;
    }

    sf::View mapView;
    mapView.setCenter(mapCenter);
    mapView.setSize(static_cast<float>(canvasW) * mapZoom, static_cast<float>(canvasH) * mapZoom);

    mapCanvas.clear(sf::Color(176, 196, 204));
    mapCanvas.setView(mapView);

    if (!rhumbLines.empty()) mapCanvas.draw(rhumbLines.data(), rhumbLines.size(), sf::Lines);
    if (!seaWaves.empty()) mapCanvas.draw(seaWaves.data(), seaWaves.size(), sf::Lines);

    mapCanvas.draw(frankiaCoast);
    mapCanvas.draw(scandiCoast);

    bool isZoomedOut = (mapZoom > 0.85f);

    for (size_t i = 0; i < counties.size(); ++i) {
        auto& c = counties[i];
        sf::Color fillCol = getKingdomBaseColor(c.kingdomName);
        bool isContested = (c.kingdomName != c.deJureKingdom);

        if (currentLens == MapLens::DeFacto) {
            fillCol = getKingdomBaseColor(c.kingdomName);
        } else if (currentLens == MapLens::DeJure) {
            fillCol = getKingdomBaseColor(c.deJureKingdom);
        } else if (currentLens == MapLens::Vassals) {
            int pseudoOpinion = ((static_cast<int>(c.countyId) * 37) % 70) - 20;
            if (pseudoOpinion >= 25) fillCol = sf::Color(70, 165, 70);
            else if (pseudoOpinion >= 0) fillCol = sf::Color(200, 170, 55);
            else fillCol = sf::Color(190, 55, 45);
        } else if (currentLens == MapLens::Diplomacy) {
            if (c.kingdomName.find("Wessex") != std::string::npos) fillCol = sf::Color(55, 125, 215);
            else if (c.kingdomName.find("Cornwall") != std::string::npos) fillCol = sf::Color(45, 185, 220);
            else if (c.kingdomName.find("Northumbria") != std::string::npos) fillCol = sf::Color(215, 40, 40);
            else fillCol = sf::Color(110, 105, 100);
        } else if (currentLens == MapLens::Economy) {
            if (c.countyName == "Middlesex" || c.countyName == "Hampshire" || c.countyName == "Yorkshire") fillCol = sf::Color(235, 195, 50);
            else if (c.countyName == "Warwick" || c.countyName == "Chester" || c.countyName == "Norfolk") fillCol = sf::Color(115, 175, 90);
            else fillCol = sf::Color(165, 115, 75);
        }

        if (!isZoomedOut) {
            int shade = (static_cast<int>(c.countyId) % 3) * 6 - 6;
            fillCol.r = static_cast<sf::Uint8>(std::clamp(static_cast<int>(fillCol.r) + shade, 0, 255));
            fillCol.g = static_cast<sf::Uint8>(std::clamp(static_cast<int>(fillCol.g) + shade, 0, 255));
            fillCol.b = static_cast<sf::Uint8>(std::clamp(static_cast<int>(fillCol.b) + shade, 0, 255));
        }

        bool isHovered = false;
        if (isZoomedOut) {
            if (!hoveredKingdomName.empty() && c.kingdomName == hoveredKingdomName) {
                isHovered = true;
            }
        } else {
            if (static_cast<int>(i) == hoveredCountyIdx) {
                isHovered = true;
            }
        }

        if (isHovered) {
            fillCol.r = static_cast<sf::Uint8>(std::min(255, fillCol.r + 40));
            fillCol.g = static_cast<sf::Uint8>(std::min(255, fillCol.g + 40));
            fillCol.b = static_cast<sf::Uint8>(std::min(255, fillCol.b + 40));
            c.shape.setOutlineColor(sf::Color(255, 235, 140, 255));
            c.shape.setOutlineThickness(isZoomedOut ? 2.5f : 1.8f);
        } else {
            if (isZoomedOut) {
                c.shape.setOutlineColor(fillCol);
                c.shape.setOutlineThickness(0.5f);
            } else {
                c.shape.setOutlineColor(sf::Color(35, 22, 14, 180));
                c.shape.setOutlineThickness(1.2f);
            }
        }

        c.shape.setFillColor(fillCol);
        mapCanvas.draw(c.shape);

        if (c.isOccupied) {
            sf::Color occColor = getKingdomBaseColor(c.occupierKingdom);
            occColor.a = 150;
            const auto& pts = c.points;
            size_t numPts = pts.size();

            float dMin = 999999.f, dMax = -999999.f;
            for (const auto& p : pts) {
                float val = p.x - p.y;
                if (val < dMin) dMin = val;
                if (val > dMax) dMax = val;
            }

            for (float d = dMin + 7.f; d < dMax; d += 14.f) {
                std::vector<sf::Vector2f> hits;
                for (size_t k = 0; k < numPts; ++k) {
                    sf::Vector2f p1 = pts[k];
                    sf::Vector2f p2 = pts[(k + 1) % numPts];
                    float denom = (p2.x - p1.x) - (p2.y - p1.y);
                    if (std::abs(denom) > 0.0001f) {
                        float t = (d - (p1.x - p1.y)) / denom;
                        if (t >= 0.0f && t <= 1.0f) hits.push_back(p1 + t * (p2 - p1));
                    }
                }

                if (hits.size() >= 2) {
                    std::sort(hits.begin(), hits.end(), [](const sf::Vector2f& a, const sf::Vector2f& b) { return a.x < b.x; });
                    for (size_t h = 0; h + 1 < hits.size(); h += 2) {
                        sf::Vector2f mid = (hits[h] + hits[h + 1]) * 0.5f;
                        if (pointInPolygon(pts, mid)) {
                            sf::Vertex stripe[] = { sf::Vertex(hits[h], occColor), sf::Vertex(hits[h + 1], occColor) };
                            mapCanvas.draw(stripe, 2, sf::Lines);
                        }
                    }
                }
            }

            sf::RectangleShape occBadge(sf::Vector2f(74.f, 14.f));
            occBadge.setOrigin(37.f, 7.f);
            occBadge.setPosition(c.center.x, c.center.y - 18.f);
            occBadge.setFillColor(sf::Color(18, 12, 10, 230));
            occBadge.setOutlineColor(occColor);
            occBadge.setOutlineThickness(1.f);
            mapCanvas.draw(occBadge);

            sf::Text oTxt("OCCUPIED", font, 8);
            oTxt.setStyle(sf::Text::Bold);
            oTxt.setFillColor(sf::Color(255, 235, 175));
            sf::FloatRect tb = oTxt.getLocalBounds();
            oTxt.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
            oTxt.setPosition(occBadge.getPosition());
            mapCanvas.draw(oTxt);
        }

        if (c.siegeProgress > 0.f && !c.isOccupied) {
            sf::CircleShape ringBack(14.f);
            ringBack.setOrigin(14.f, 14.f);
            ringBack.setPosition(c.center);
            ringBack.setFillColor(sf::Color(20, 14, 10, 220));
            ringBack.setOutlineColor(sf::Color(185, 140, 60));
            ringBack.setOutlineThickness(1.2f);
            mapCanvas.draw(ringBack);

            int segs = static_cast<int>((c.siegeProgress / 100.f) * 16.f);
            for (int s = 0; s < segs; ++s) {
                float a1 = s * (6.28318f / 16.f) - 1.57079f;
                float a2 = (s + 1) * (6.28318f / 16.f) - 1.57079f;
                sf::ConvexShape wedge(3);
                wedge.setPoint(0, c.center);
                wedge.setPoint(1, c.center + sf::Vector2f(std::cos(a1) * 13.f, std::sin(a1) * 13.f));
                wedge.setPoint(2, c.center + sf::Vector2f(std::cos(a2) * 13.f, std::sin(a2) * 13.f));
                wedge.setFillColor(sf::Color(235, 60, 45, 220));
                mapCanvas.draw(wedge);
            }

            sf::Text sPct(std::to_string(static_cast<int>(c.siegeProgress)) + "%", font, 8);
            sPct.setStyle(sf::Text::Bold);
            sPct.setFillColor(sf::Color::White);
            sf::FloatRect pb = sPct.getLocalBounds();
            sPct.setOrigin(pb.left + pb.width * 0.5f, pb.top + pb.height * 0.5f);
            sPct.setPosition(c.center.x, c.center.y + 20.f);
            mapCanvas.draw(sPct);
        }

        if (currentLens == MapLens::DeJure && isContested) {
            sf::Color stripeCol(245, 60, 50, 130);
            const auto& pts = c.points;
            size_t numPts = pts.size();

            float dMin = 999999.f;
            float dMax = -999999.f;
            for (const auto& p : pts) {
                float val = p.x - p.y;
                if (val < dMin) dMin = val;
                if (val > dMax) dMax = val;
            }

            for (float d = dMin + 7.f; d < dMax; d += 14.f) {
                std::vector<sf::Vector2f> hits;
                for (size_t k = 0; k < numPts; ++k) {
                    sf::Vector2f p1 = pts[k];
                    sf::Vector2f p2 = pts[(k + 1) % numPts];

                    float denom = (p2.x - p1.x) - (p2.y - p1.y);
                    if (std::abs(denom) > 0.0001f) {
                        float t = (d - (p1.x - p1.y)) / denom;
                        if (t >= 0.0f && t <= 1.0f) {
                            hits.push_back(p1 + t * (p2 - p1));
                        }
                    }
                }

                if (hits.size() >= 2) {
                    std::sort(hits.begin(), hits.end(), [](const sf::Vector2f& a, const sf::Vector2f& b) {
                        return a.x < b.x;
                    });

                    for (size_t h = 0; h + 1 < hits.size(); h += 2) {
                        sf::Vector2f mid = (hits[h] + hits[h + 1]) * 0.5f;
                        if (pointInPolygon(pts, mid)) {
                            sf::Vertex stripe[] = {
                                sf::Vertex(hits[h], stripeCol),
                                sf::Vertex(hits[h + 1], stripeCol)
                            };
                            mapCanvas.draw(stripe, 2, sf::Lines);
                        }
                    }
                }
            }

            sf::RectangleShape marker(sf::Vector2f(72.f, 14.f));
            marker.setOrigin(36.f, 7.f);
            marker.setPosition(c.center.x, c.center.y + 14.f);
            marker.setFillColor(sf::Color(140, 20, 15, 230));
            marker.setOutlineColor(sf::Color(255, 220, 100));
            marker.setOutlineThickness(1.f);
            mapCanvas.draw(marker);

            sf::Text cTag("USURPED", font, 8);
            cTag.setStyle(sf::Text::Bold);
            cTag.setFillColor(sf::Color(255, 240, 200));
            sf::FloatRect tb = cTag.getLocalBounds();
            cTag.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
            cTag.setPosition(marker.getPosition().x, marker.getPosition().y - 1.f);
            mapCanvas.draw(cTag);
        }
    }

    if (isZoomedOut) {
        for (size_t i = 0; i < counties.size(); ++i) {
            const auto& cA = counties[i];
            for (size_t j = i + 1; j < counties.size(); ++j) {
                const auto& cB = counties[j];
                if (cA.kingdomName == cB.kingdomName) continue;

                for (size_t pA = 0; pA < cA.points.size(); ++pA) {
                    sf::Vector2f ptA1 = cA.points[pA];
                    sf::Vector2f ptA2 = cA.points[(pA + 1) % cA.points.size()];

                    for (size_t pB = 0; pB < cB.points.size(); ++pB) {
                        sf::Vector2f ptB1 = cB.points[pB];
                        sf::Vector2f ptB2 = cB.points[(pB + 1) % cB.points.size()];

                        float d1 = std::hypot(ptA1.x - ptB2.x, ptA1.y - ptB2.y);
                        float d2 = std::hypot(ptA2.x - ptB1.x, ptA2.y - ptB1.y);

                        if (d1 < 1.0f && d2 < 1.0f) {
                            sf::Vertex kBorder[] = {
                                sf::Vertex(ptA1, sf::Color(235, 195, 60, 230)),
                                sf::Vertex(ptA2, sf::Color(235, 195, 60, 230))
                            };
                            mapCanvas.draw(kBorder, 2, sf::Lines);
                        }
                    }
                }
            }
        }

        struct RealmLabel {
            std::string text;
            sf::Vector2f pos;
            float rotation;
            unsigned int size;
        };

        std::vector<RealmLabel> rLabels = {
            {"W E S S E X",           {440.f, 525.f},   0.f, 18},
            {"M E R C I A",           {440.f, 415.f},  -8.f, 18},
            {"N O R T H U M B R I A", {465.f, 285.f}, -15.f, 16},
            {"A L B A",               {435.f, 160.f},   0.f, 18},
            {"C O R N W A L L",       {295.f, 555.f}, -25.f, 13},
            {"E A S T  A N G L I A",  {535.f, 445.f},  15.f, 12},
            {"I R E L A N D",         {235.f, 400.f},  65.f, 16}
        };

        for (const auto& rl : rLabels) {
            sf::Text rTxt(rl.text, font, rl.size);
            rTxt.setStyle(sf::Text::Bold);
            rTxt.setFillColor(sf::Color(25, 18, 12, 240));
            rTxt.setOutlineColor(sf::Color(245, 235, 205, 210));
            rTxt.setOutlineThickness(1.5f);
            rTxt.setRotation(rl.rotation);

            sf::FloatRect b = rTxt.getLocalBounds();
            rTxt.setOrigin(b.left + b.width * 0.5f, b.top + b.height * 0.5f);
            rTxt.setPosition(rl.pos);
            mapCanvas.draw(rTxt);
        }
    } else {
        for (const auto& c : counties) {
            sf::Text cName(c.countyName, font, 11);
            cName.setStyle(sf::Text::Bold);
            cName.setFillColor(sf::Color(25, 16, 10, 240));
            cName.setOutlineColor(sf::Color(250, 242, 225, 220));
            cName.setOutlineThickness(1.6f);

            sf::FloatRect nb = cName.getLocalBounds();
            cName.setOrigin(nb.left + nb.width * 0.5f, nb.top + nb.height * 0.5f);
            cName.setPosition(c.center.x, c.center.y - 6.f);
            mapCanvas.draw(cName);

            sf::Text sName(c.settlementName, font, 9);
            sName.setStyle(sf::Text::Italic);
            sName.setFillColor(sf::Color(65, 45, 25, 200));
            sName.setOutlineColor(sf::Color(250, 242, 225, 200));
            sName.setOutlineThickness(1.2f);

            sf::FloatRect sb = sName.getLocalBounds();
            sName.setOrigin(sb.left + sb.width * 0.5f, sb.top + sb.height * 0.5f);
            sName.setPosition(c.center.x, c.center.y + 7.f);
            mapCanvas.draw(sName);
        }
    }

    for (const auto& a : mapArmies) {
        if (a.isMoving) {
            sf::Vertex path[] = {
                sf::Vertex(a.pos, sf::Color(255, 220, 100, 180)),
                sf::Vertex(a.targetPos, sf::Color(255, 220, 100, 60))
            };
            mapCanvas.draw(path, 2, sf::Lines);
        }

        bool isSel = (static_cast<int>(a.id) == selectedArmyId);

        sf::CircleShape banner(12.f);
        banner.setOrigin(12.f, 12.f);
        banner.setPosition(a.pos);
        banner.setFillColor(getKingdomBaseColor(a.ownerKingdom));
        banner.setOutlineColor(isSel ? sf::Color(255, 230, 80) : sf::Color(30, 20, 14));
        banner.setOutlineThickness(isSel ? 2.5f : 1.5f);
        mapCanvas.draw(banner);

        sf::Text aStrength(std::to_string(a.strength), font, 9);
        aStrength.setStyle(sf::Text::Bold);
        aStrength.setFillColor(sf::Color::White);
        sf::FloatRect sb = aStrength.getLocalBounds();
        aStrength.setOrigin(sb.left + sb.width * 0.5f, sb.top + sb.height * 0.5f);
        aStrength.setPosition(a.pos.x, a.pos.y - 1.f);
        mapCanvas.draw(aStrength);

        if (isSel) {
            sf::Text selPrompt("SELECTED", font, 7);
            selPrompt.setStyle(sf::Text::Bold);
            selPrompt.setFillColor(sf::Color(255, 225, 100));
            sf::FloatRect spb = selPrompt.getLocalBounds();
            selPrompt.setOrigin(spb.left + spb.width * 0.5f, spb.top + spb.height * 0.5f);
            selPrompt.setPosition(a.pos.x, a.pos.y - 16.f);
            mapCanvas.draw(selPrompt);
        }

        if (a.inCombat) {
            float p = 0.5f + 0.5f * std::sin(pulseTime * 14.f);
            sf::Color swCol(255, static_cast<sf::Uint8>(60 + p * 150), 40);

            sf::Vertex sw1[] = {
                sf::Vertex(sf::Vector2f(a.pos.x - 9.f, a.pos.y - 20.f), swCol),
                sf::Vertex(sf::Vector2f(a.pos.x + 9.f, a.pos.y - 36.f), swCol)
            };
            sf::Vertex sw2[] = {
                sf::Vertex(sf::Vector2f(a.pos.x + 9.f, a.pos.y - 20.f), swCol),
                sf::Vertex(sf::Vector2f(a.pos.x - 9.f, a.pos.y - 36.f), swCol)
            };
            mapCanvas.draw(sw1, 2, sf::Lines);
            mapCanvas.draw(sw2, 2, sf::Lines);

            sf::Text clTxt("BATTLE", font, 8);
            clTxt.setStyle(sf::Text::Bold);
            clTxt.setFillColor(sf::Color(255, 235, 120));
            sf::FloatRect cb = clTxt.getLocalBounds();
            clTxt.setOrigin(cb.left + cb.width * 0.5f, cb.top + cb.height * 0.5f);
            clTxt.setPosition(a.pos.x, a.pos.y - 42.f);
            mapCanvas.draw(clTxt);
        }
    }

    for (const auto& ca : councilAssignments) {
        if (ca.mission == CouncilMissionType::None) continue;
        sf::Vector2f cPos(0.f, 0.f);
        for (const auto& c : counties) {
            if (c.countyName == ca.targetCounty) {
                cPos = c.center + sf::Vector2f(-22.f, -14.f);
                break;
            }
        }
        if (cPos.x == 0.f && cPos.y == 0.f) continue;

        sf::Color pinCol = sf::Color(160, 110, 220);
        std::string badgeLetter = "S";
        if (ca.role == sim::CouncilRole::WarChief) {
            pinCol = sf::Color(220, 65, 55);
            badgeLetter = "W";
        } else if (ca.role == sim::CouncilRole::ChiefBuilder) {
            pinCol = sf::Color(70, 175, 95);
            badgeLetter = "B";
        }

        sf::CircleShape pin(9.f);
        pin.setOrigin(9.f, 9.f);
        pin.setPosition(cPos);
        pin.setFillColor(pinCol);
        pin.setOutlineColor(sf::Color(255, 235, 170));
        pin.setOutlineThickness(1.2f);
        mapCanvas.draw(pin);

        sf::Text bTxt(badgeLetter, font, 9);
        bTxt.setStyle(sf::Text::Bold);
        bTxt.setFillColor(sf::Color::White);
        sf::FloatRect btb = bTxt.getLocalBounds();
        bTxt.setOrigin(btb.left + btb.width * 0.5f, btb.top + btb.height * 0.5f);
        bTxt.setPosition(cPos.x, cPos.y - 1.f);
        mapCanvas.draw(bTxt);

        if (ca.mission == CouncilMissionType::FabricateClaim) {
            float pW = 28.f;
            float pH = 4.f;
            sf::RectangleShape pBg(sf::Vector2f(pW, pH));
            pBg.setPosition(cPos.x - pW * 0.5f, cPos.y + 11.f);
            pBg.setFillColor(sf::Color(16, 12, 10, 220));
            pBg.setOutlineColor(sf::Color(65, 45, 35));
            pBg.setOutlineThickness(0.8f);
            mapCanvas.draw(pBg);

            float fillW = pW * (ca.progress / ca.maxProgress);
            if (fillW > 0.f) {
                sf::RectangleShape pFill(sf::Vector2f(fillW, pH));
                pFill.setPosition(cPos.x - pW * 0.5f, cPos.y + 11.f);
                pFill.setFillColor(pinCol);
                mapCanvas.draw(pFill);
            }
        }
    }

    sf::Vector2f playerCoord = getPlayerMapCoord(playerX);

    float pulse = 1.0f + 0.35f * std::sin(pulseTime * 5.0f);
    sf::CircleShape halo(9.f * pulse);
    halo.setOrigin(halo.getRadius(), halo.getRadius());
    halo.setPosition(playerCoord);
    halo.setFillColor(sf::Color(235, 195, 45, 110));
    mapCanvas.draw(halo);

    sf::CircleShape playerPin(5.5f);
    playerPin.setOrigin(5.5f, 5.5f);
    playerPin.setPosition(playerCoord);
    playerPin.setFillColor(sf::Color(255, 220, 50));
    playerPin.setOutlineColor(sf::Color(35, 18, 5));
    playerPin.setOutlineThickness(2.f);
    mapCanvas.draw(playerPin);

    sf::Text youTxt("Alpha (You)", font, 9);
    youTxt.setStyle(sf::Text::Bold);
    youTxt.setFillColor(sf::Color(45, 20, 10));
    youTxt.setOutlineColor(sf::Color(255, 245, 205));
    youTxt.setOutlineThickness(1.6f);
    sf::FloatRect yb = youTxt.getLocalBounds();
    youTxt.setOrigin(yb.left + yb.width * 0.5f, yb.top + yb.height);
    youTxt.setPosition(playerCoord.x, playerCoord.y - 8.f);
    mapCanvas.draw(youTxt);

    mapCanvas.display();

    float easeT = expandAnimT * expandAnimT * (3.0f - 2.0f * expandAnimT);

    float miniX = 984.f, miniY = 16.f, miniW = 276.f, miniH = 200.f;
    float fullX = 110.f, fullY = 35.f, fullW = 1060.f, fullH = 650.f;

    float curX = miniX + (fullX - miniX) * easeT;
    float curY = miniY + (fullY - miniY) * easeT;
    float curW = miniW + (fullW - miniW) * easeT;
    float curH = miniH + (fullH - miniH) * easeT;

    sf::RectangleShape backdrop(sf::Vector2f(1280.f, 720.f));
    backdrop.setFillColor(sf::Color(12, 10, 8, static_cast<sf::Uint8>(easeT * 225.f)));
    window.draw(backdrop);

    mapOuterVellum.setSize(sf::Vector2f(curW, curH));
    mapOuterVellum.setOrigin(curW * 0.5f, curH * 0.5f);
    mapOuterVellum.setPosition(curX + curW * 0.5f, curY + curH * 0.5f);
    window.draw(mapOuterVellum);

    mapInnerVellum.setSize(sf::Vector2f(curW - 24.f, curH - 24.f));
    mapInnerVellum.setOrigin((curW - 24.f) * 0.5f, (curH - 24.f) * 0.5f);
    mapInnerVellum.setPosition(curX + curW * 0.5f, curY + curH * 0.5f);
    window.draw(mapInnerVellum);

    mapInnerBorder.setSize(sf::Vector2f(curW - 44.f, curH - 44.f));
    mapInnerBorder.setOrigin((curW - 44.f) * 0.5f, (curH - 44.f) * 0.5f);
    mapInnerBorder.setPosition(curX + curW * 0.5f, curY + curH * 0.5f);
    window.draw(mapInnerBorder);

    float cX = curX + 24.f * easeT + 8.f;
    float cY = curY + 73.f * easeT + 26.f * (1.f - easeT);
    float cW = curW - (48.f * easeT + 16.f);
    float cH = curH - (126.f * easeT + 36.f * (1.f - easeT));

    sf::Sprite canvasSprite(mapCanvas.getTexture());
    canvasSprite.setPosition(cX, cY);
    canvasSprite.setScale(cW / static_cast<float>(canvasW), cH / static_cast<float>(canvasH));
    window.draw(canvasSprite);

    if (easeT > 0.65f) {
        sf::Uint8 uiAlpha = static_cast<sf::Uint8>((easeT - 0.65f) / 0.35f * 255.f);

        sf::RectangleShape headerRibbon(sf::Vector2f(1012.f, 40.f));
        headerRibbon.setPosition(134.f, 68.f);
        headerRibbon.setFillColor(sf::Color(38, 26, 16, uiAlpha));
        headerRibbon.setOutlineColor(sf::Color(120, 85, 45, uiAlpha));
        headerRibbon.setOutlineThickness(1.5f);
        window.draw(headerRibbon);

        sf::Text headerText("BRITANNIA ET PARTES SEPTENTRIONALES", font, 13);
        headerText.setStyle(sf::Text::Bold);
        headerText.setFillColor(sf::Color(245, 220, 140, uiAlpha));
        headerText.setPosition(148.f, 72.f);
        window.draw(headerText);

        sf::Text subText("AD 878 | REGNA ANGLO-SAXONUM ET DANELAGH", font, 9);
        subText.setStyle(sf::Text::Italic);
        subText.setFillColor(sf::Color(175, 145, 105, uiAlpha));
        subText.setPosition(148.f, 89.f);
        window.draw(subText);

        float tabStartX = 538.f;
        float tabY = 74.f;
        float tabW = 96.f;
        float tabH = 26.f;
        float tabGap = 5.f;

        const std::string tabLabels[5] = {
            "[Q] De Facto",
            "[W] De Jure",
            "[E] Vassals",
            "[R] Diplomacy",
            "[T] Economy"
        };

        for (int i = 0; i < 5; ++i) {
            float tx = tabStartX + i * (tabW + tabGap);
            lensTabBounds[i] = sf::FloatRect(tx, tabY, tabW, tabH);
            bool isActive = (currentLens == static_cast<MapLens>(i));

            sf::RectangleShape tabBox(sf::Vector2f(tabW, tabH));
            tabBox.setPosition(tx, tabY);

            if (isActive) {
                tabBox.setFillColor(sf::Color(115, 75, 28, uiAlpha));
                tabBox.setOutlineColor(sf::Color(245, 215, 95, uiAlpha));
                tabBox.setOutlineThickness(1.5f);
            } else {
                tabBox.setFillColor(sf::Color(22, 16, 12, static_cast<sf::Uint8>(uiAlpha * 0.85f)));
                tabBox.setOutlineColor(sf::Color(78, 54, 30, uiAlpha));
                tabBox.setOutlineThickness(1.f);
            }
            window.draw(tabBox);

            sf::Text tabTxt(tabLabels[i], font, 10);
            tabTxt.setStyle(isActive ? sf::Text::Bold : sf::Text::Regular);
            tabTxt.setFillColor(isActive ? sf::Color(255, 245, 205, uiAlpha) : sf::Color(180, 150, 110, uiAlpha));
            sf::FloatRect tb = tabTxt.getLocalBounds();
            tabTxt.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
            tabTxt.setPosition(tx + tabW * 0.5f, tabY + tabH * 0.5f);
            window.draw(tabTxt);
        }

        sf::RectangleShape footerRibbon(sf::Vector2f(1012.f, 26.f));
        footerRibbon.setPosition(134.f, 634.f);
        footerRibbon.setFillColor(sf::Color(32, 22, 14, uiAlpha));
        footerRibbon.setOutlineColor(sf::Color(115, 82, 42, uiAlpha));
        footerRibbon.setOutlineThickness(1.5f);
        window.draw(footerRibbon);

        std::string legendStr = "";
        if (currentLens == MapLens::DeFacto) {
            legendStr = isZoomedOut ? "LENS: De Facto Realms | Physical realm control | Left-Click: Inspect | Right-Click: Actions"
                                    : "LENS: De Facto Counties | Local administration | Left-Click: Inspect | Right-Click: Actions";
        } else if (currentLens == MapLens::DeJure) {
            legendStr = "LENS: De Jure Kingdoms | Historical legitimate borders | Red tint: Usurped / Contested Title";
        } else if (currentLens == MapLens::Vassals) {
            legendStr = "LENS: Vassal Loyalty | Green: Loyal Alpha (+25) | Yellow: Neutral (0 to +25) | Red: Disloyal (<0)";
        } else if (currentLens == MapLens::Diplomacy) {
            legendStr = "LENS: Diplomacy | Blue: Your Realm & Allies | Red: War Opponents | Grey: Neutral Realms";
        } else if (currentLens == MapLens::Economy) {
            legendStr = "LENS: Economy | Gold: Wealthy (>1.2k Stockpile) | Green: Developed (>400) | Brown: Subsistence";
        }

        sf::Text legend(legendStr, font, 10);
        legend.setFillColor(sf::Color(240, 218, 160, uiAlpha));
        sf::FloatRect legB = legend.getLocalBounds();
        legend.setOrigin(legB.left + legB.width / 2.f, legB.top + legB.height / 2.f);
        legend.setPosition(640.f, 647.f);
        window.draw(legend);

        if (activeWar.active) {
            sf::RectangleShape badge(sf::Vector2f(warBadgeBounds.width, warBadgeBounds.height));
            badge.setPosition(warBadgeBounds.left, warBadgeBounds.top);
            badge.setFillColor(sf::Color(20, 14, 10, 245));
            badge.setOutlineColor(activeWar.warScore >= 75.f ? sf::Color(100, 230, 90) : sf::Color(225, 60, 50));
            badge.setOutlineThickness(1.5f);
            window.draw(badge);

            float pulse = 0.5f + 0.5f * std::sin(pulseTime * 6.f);
            sf::CircleShape pip(5.f);
            pip.setOrigin(5.f, 5.f);
            pip.setPosition(warBadgeBounds.left + 15.f, warBadgeBounds.top + 22.f);
            pip.setFillColor(sf::Color(225, 45, 35, static_cast<sf::Uint8>(180 + pulse * 75)));
            window.draw(pip);

            sf::Text wLabel("WAR FOR " + activeWar.targetCounty + " (" + activeWar.enemyKingdom + ")", font, 9);
            wLabel.setStyle(sf::Text::Bold);
            wLabel.setFillColor(sf::Color(245, 220, 160));
            wLabel.setPosition(warBadgeBounds.left + 26.f, warBadgeBounds.top + 6.f);
            window.draw(wLabel);

            std::string scoreStr = "+" + std::to_string(static_cast<int>(std::round(activeWar.warScore))) + "%";
            sf::Text scoreTxt(scoreStr, font, 13);
            scoreTxt.setStyle(sf::Text::Bold);
            scoreTxt.setFillColor(activeWar.warScore >= 75.f ? sf::Color(90, 235, 90) : sf::Color(245, 185, 65));
            scoreTxt.setPosition(warBadgeBounds.left + 26.f, warBadgeBounds.top + 20.f);
            window.draw(scoreTxt);

            sf::Text treatyPrompt("[Negotiate Peace ->]", font, 9);
            treatyPrompt.setStyle(sf::Text::Bold);
            treatyPrompt.setFillColor(sf::Color(255, 230, 120));
            treatyPrompt.setPosition(warBadgeBounds.left + 140.f, warBadgeBounds.top + 22.f);
            window.draw(treatyPrompt);
        }

        if (activeWar.active && peaceModalOpen) {
            sf::FloatRect mR(390.f, 175.f, 500.f, 335.f);

            sf::RectangleShape shadow(sf::Vector2f(mR.width + 6.f, mR.height + 6.f));
            shadow.setPosition(mR.left + 3.f, mR.top + 3.f);
            shadow.setFillColor(sf::Color(0, 0, 0, 210));
            window.draw(shadow);

            sf::RectangleShape modal(sf::Vector2f(mR.width, mR.height));
            modal.setPosition(mR.left, mR.top);
            modal.setFillColor(sf::Color(22, 16, 12, 252));
            modal.setOutlineColor(sf::Color(195, 150, 70));
            modal.setOutlineThickness(1.8f);
            window.draw(modal);

            sf::RectangleShape header(sf::Vector2f(mR.width - 6.f, 32.f));
            header.setPosition(mR.left + 3.f, mR.top + 3.f);
            header.setFillColor(sf::Color(46, 30, 20));
            window.draw(header);

            sf::Text title("PEACE TREATY: WAR FOR " + activeWar.targetCounty, font, 12);
            title.setStyle(sf::Text::Bold);
            title.setFillColor(sf::Color(255, 230, 140));
            title.setPosition(mR.left + 14.f, mR.top + 8.f);
            window.draw(title);

            closePeaceModalBounds = sf::FloatRect(mR.left + mR.width - 26.f, mR.top + 6.f, 20.f, 20.f);
            sf::RectangleShape closeBtn(sf::Vector2f(20.f, 20.f));
            closeBtn.setPosition(closePeaceModalBounds.left, closePeaceModalBounds.top);
            closeBtn.setFillColor(sf::Color(140, 25, 20));
            closeBtn.setOutlineColor(sf::Color(240, 200, 75));
            closeBtn.setOutlineThickness(1.f);
            window.draw(closeBtn);

            sf::Text xT("x", font, 12);
            xT.setStyle(sf::Text::Bold);
            xT.setFillColor(sf::Color::White);
            xT.setPosition(closePeaceModalBounds.left + 6.f, closePeaceModalBounds.top + 1.f);
            window.draw(xT);

            float curY = mR.top + 42.f;
            sf::Text sub("Casus Belli: " + activeWar.casusBelli, font, 10);
            sub.setFillColor(sf::Color(195, 180, 150));
            sub.setPosition(mR.left + 16.f, curY);
            window.draw(sub);
            curY += 20.f;

            sf::RectangleShape breakdownBox(sf::Vector2f(mR.width - 32.f, 44.f));
            breakdownBox.setPosition(mR.left + 16.f, curY);
            breakdownBox.setFillColor(sf::Color(14, 10, 8, 235));
            breakdownBox.setOutlineColor(sf::Color(90, 65, 40));
            breakdownBox.setOutlineThickness(1.f);
            window.draw(breakdownBox);

            std::string scStr = "+" + std::to_string(static_cast<int>(std::round(activeWar.warScore))) + "%";
            sf::Text scH("Current War Score: " + scStr, font, 12);
            scH.setStyle(sf::Text::Bold);
            scH.setFillColor(activeWar.warScore >= 75.f ? sf::Color(105, 240, 105) : sf::Color(245, 195, 65));
            scH.setPosition(breakdownBox.getPosition().x + 10.f, breakdownBox.getPosition().y + 6.f);
            window.draw(scH);

            sf::Text dtTxt(activeWar.warScore >= 75.f ? "Superiority established. Demands can be fully enforced!"
                                                      : "War score progressing toward requirement (+75%)...", font, 9);
            dtTxt.setFillColor(sf::Color(185, 170, 140));
            dtTxt.setPosition(breakdownBox.getPosition().x + 10.f, breakdownBox.getPosition().y + 24.f);
            window.draw(dtTxt);
            curY += 56.f;

            auto drawPeaceOption = [&](sf::FloatRect& bounds, const std::string& optTitle, const std::string& desc, bool enabled, sf::Color col) {
                bounds = sf::FloatRect(mR.left + 16.f, curY, mR.width - 32.f, 50.f);

                sf::RectangleShape opt(sf::Vector2f(bounds.width, bounds.height));
                opt.setPosition(bounds.left, bounds.top);
                opt.setFillColor(enabled ? sf::Color(32, 22, 16, 240) : sf::Color(18, 14, 12, 170));
                opt.setOutlineColor(enabled ? col : sf::Color(65, 50, 40));
                opt.setOutlineThickness(1.2f);
                window.draw(opt);

                sf::RectangleShape bar(sf::Vector2f(4.f, bounds.height));
                bar.setPosition(bounds.left, bounds.top);
                bar.setFillColor(enabled ? col : sf::Color(75, 60, 50));
                window.draw(bar);

                sf::Text oT(optTitle, font, 11);
                oT.setStyle(sf::Text::Bold);
                oT.setFillColor(enabled ? sf::Color::White : sf::Color(130, 120, 110));
                oT.setPosition(bounds.left + 12.f, bounds.top + 6.f);
                window.draw(oT);

                sf::Text oD(desc, font, 9);
                oD.setStyle(sf::Text::Italic);
                oD.setFillColor(enabled ? sf::Color(185, 170, 145) : sf::Color(100, 95, 90));
                oD.setPosition(bounds.left + 12.f, bounds.top + 24.f);
                window.draw(oD);

                curY += 56.f;
            };

            bool canEnforce = (activeWar.warScore >= 75.f);
            drawPeaceOption(enforceBtnBounds, "1. Enforce Demands",
                            canEnforce ? ("Annex " + activeWar.targetCounty + " into " + activeWar.attackerKingdom + " and sign a 5-year truce.")
                                       : "Requires at least +75% War Score.",
                            canEnforce, sf::Color(90, 225, 90));

            drawPeaceOption(whitePeaceBtnBounds, "2. White Peace",
                            "Status quo ante bellum. Borders remain unchanged. 3-year truce.",
                            true, sf::Color(225, 195, 75));

            drawPeaceOption(surrenderBtnBounds, "3. Surrender",
                            "Concede defeat, renounce claim, and pay reparations.",
                            true, sf::Color(235, 65, 65));
        }
    }
}

const RealSettlement* SettlementSystem::getActiveSettlement() const {
    if (activeSettlementIdx >= 0 && activeSettlementIdx < static_cast<int>(realSettlements.size())) {
        return &realSettlements[activeSettlementIdx];
    }
    return nullptr;
}

const RealSettlement* SettlementSystem::getSettlementAt(float x) const {
    for (const auto& s : realSettlements) {
        if (x >= s.borderLeftX && x <= s.borderRightX) {
            return &s;
        }
    }
    return nullptr;
}

const RealSettlement* SettlementSystem::getSettlementByVillageId(sim::VillageID id) const {
    for (const auto& s : realSettlements) {
        if (s.villageId == id) {
            return &s;
        }
    }
    return nullptr;
}

bool SettlementSystem::canFreelyPass(float x) const {
    const RealSettlement* s = getSettlementAt(x);
    if (s) {
        if (s->isAllied || s->kingdomName.find("Wessex") != std::string::npos || s->kingdomName.find("Cornwall") != std::string::npos) {
            return true;
        }
    }
    if (!realSettlements.empty() && x <= realSettlements[0].centerX) return true;
    return false;
}