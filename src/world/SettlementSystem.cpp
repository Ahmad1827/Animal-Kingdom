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
    s_instance->activeWarAllies.clear();
    s_instance->callAllyStatusTimer = 0.f;
    s_instance->callAllyStatusMsg.clear();

    std::string musterCounty = county;
    for (const auto& c : s_instance->counties) {
        if (c.kingdomName == enemy && c.countyName != county) {
            musterCounty = c.countyName;
            break;
        }
    }

    spawnArmy(musterCounty, enemy, 22);
}

void SettlementSystem::callAllyToWar(const std::string& allyKingdom) {
    if (!s_instance || !s_instance->activeWar.active) return;
    for (const auto& al : s_instance->activeWarAllies) {
        if (al == allyKingdom) return;
    }

    s_instance->activeWarAllies.push_back(allyKingdom);

    std::string allyCounty = "";
    for (const auto& c : s_instance->counties) {
        if (c.kingdomName == allyKingdom) {
            allyCounty = c.countyName;
            break;
        }
    }
    if (allyCounty.empty() && !s_instance->counties.empty()) {
        allyCounty = s_instance->counties.front().countyName;
    }

    spawnArmy(allyCounty, allyKingdom, 20);

    sf::Vector2f targetPos(440.f, 525.f);
    for (const auto& c : s_instance->counties) {
        if (c.countyName == s_instance->activeWar.targetCounty) {
            targetPos = c.center + sf::Vector2f(20.f, -12.f);
            break;
        }
    }

    for (auto& a : s_instance->mapArmies) {
        if (a.ownerKingdom == allyKingdom) {
            a.targetPos = targetPos;
            a.targetCounty = s_instance->activeWar.targetCounty;
            a.isMoving = true;
            break;
        }
    }

    s_instance->callAllyStatusMsg = allyKingdom + " honors the pact! 20 allied warriors marching to front.";
    s_instance->callAllyStatusTimer = 3.8f;
}

bool SettlementSystem::isAllyInWar(const std::string& allyKingdom) {
    if (!s_instance) return false;
    for (const auto& al : s_instance->activeWarAllies) {
        if (al == allyKingdom) return true;
    }
    return false;
}

int SettlementSystem::getCrownAuthority() {
    return s_instance ? s_instance->crownAuthority : 2;
}

void SettlementSystem::setCrownAuthority(int level) {
    if (!s_instance) return;
    s_instance->crownAuthority = std::clamp(level, 1, 4);
    s_instance->selectedAuthorityTier = s_instance->crownAuthority;
}

float SettlementSystem::getAuthorityTaxMultiplier() {
    int auth = getCrownAuthority();
    if (auth == 1) return 1.0f;
    if (auth == 2) return 1.15f;
    if (auth == 3) return 1.30f;
    return 1.50f;
}

int SettlementSystem::getAuthorityLevyPerCounty() {
    int auth = getCrownAuthority();
    if (auth == 1) return 15;
    if (auth == 2) return 22;
    if (auth == 3) return 30;
    return 40;
}

int SettlementSystem::getAuthorityVassalOpinionMod() {
    int auth = getCrownAuthority();
    if (auth == 1) return 10;
    if (auth == 2) return 0;
    if (auth == 3) return -12;
    return -25;
}

bool SettlementSystem::isCountyLevyRaised(const std::string& county) {
    if (!s_instance) return false;
    for (const auto& c : s_instance->counties) {
        if (c.countyName == county) return c.leviesRaised;
    }
    return false;
}

bool SettlementSystem::canMusterCountyLevies(const std::string& county, const std::string& kingdom) {
    if (!s_instance) return false;
    for (const auto& c : s_instance->counties) {
        if (c.countyName == county) {
            bool match = (c.kingdomName == kingdom ||
                          kingdom.find(c.kingdomName) != std::string::npos ||
                          c.kingdomName.find(kingdom) != std::string::npos);
            if (match) return !c.leviesRaised && !c.isOccupied;
        }
    }
    return false;
}

bool SettlementSystem::musterCountyLevies(const std::string& county, const std::string& kingdom) {
    if (!s_instance || !canMusterCountyLevies(county, kingdom)) return false;
    for (auto& c : s_instance->counties) {
        if (c.countyName == county) {
            c.leviesRaised = true;
            int troops = getAuthorityLevyPerCounty();
            spawnArmy(county, c.kingdomName, troops);
            return true;
        }
    }
    return false;
}

std::string SettlementSystem::getPlayerKingdomId() {
    return WorldMapRepository::getInstance().getPlayerKingdomId();
}

void SettlementSystem::setPlayerKingdomId(const std::string& id) {
    WorldMapRepository::getInstance().setPlayerKingdomId(id);
}

std::string SettlementSystem::getPlayerCapitalCounty() {
    return WorldMapRepository::getInstance().getPlayerCapital();
}

std::string SettlementSystem::getKingdomDisplayName(const std::string& id) {
    return WorldMapRepository::getInstance().getKingdomDisplayName(id);
}

sf::Color SettlementSystem::getKingdomColor(const std::string& id) {
    return WorldMapRepository::getInstance().getKingdomColor(id);
}

bool SettlementSystem::isPlayerCapital(const std::string& county) {
    return county == getPlayerCapitalCounty();
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

void SettlementSystem::swayVassal(const std::string& county, int delta) {
    if (!s_instance) return;
    for (auto& c : s_instance->counties) {
        if (c.countyName == county) {
            c.vassalOpinion = std::clamp(c.vassalOpinion + delta, -100, 100);
            if (c.vassalOpinion >= 0) {
                c.inFaction = false;
            }
            break;
        }
    }
}

int SettlementSystem::getVassalOpinion(const std::string& county) {
    if (!s_instance) return 0;
    for (const auto& c : s_instance->counties) {
        if (c.countyName == county) return c.vassalOpinion;
    }
    return 0;
}

bool SettlementSystem::isCountyInFaction(const std::string& county) {
    if (!s_instance) return false;
    for (const auto& c : s_instance->counties) {
        if (c.countyName == county) return c.inFaction;
    }
    return false;
}

void SettlementSystem::triggerCivilWar(const std::string& county) {
    if (!s_instance) return;
    for (auto& c : s_instance->counties) {
        if (c.countyName == county) {
            c.kingdomName = "Rebels";
            c.inFaction = false;
            break;
        }
    }
    spawnArmy(county, "Rebels", 24);
    startWar(county, getPlayerKingdomId(), "Rebels", "Crush Independence Revolt");
    s_instance->independenceFaction.discontent = 0.f;
}

const FactionState& SettlementSystem::getFactionState() {
    static const FactionState empty;
    if (!s_instance) return empty;
    return s_instance->independenceFaction;
}

int SettlementSystem::getCountySupplyLimit(const std::string& county) {
    if (!s_instance) return 30;
    for (const auto& c : s_instance->counties) {
        if (c.countyName == county) return c.supplyLimit;
    }
    return 30;
}

int SettlementSystem::getCountyTroops(const std::string& county) {
    if (!s_instance) return 0;
    int sum = 0;
    for (const auto& a : s_instance->mapArmies) {
        if (a.currentCounty == county) sum += a.strength;
    }
    return sum;
}

int SettlementSystem::getCountyFortTier(const std::string& county) {
    if (!s_instance) return 0;
    for (const auto& c : s_instance->counties) {
        if (c.countyName == county) return c.fortTier;
    }
    return 0;
}

void SettlementSystem::upgradeCountyFort(const std::string& county) {
    if (!s_instance) return;
    for (auto& c : s_instance->counties) {
        if (c.countyName == county) {
            c.fortTier = std::min(2, c.fortTier + 1);
            if (c.fortTier == 1) c.supplyLimit += 10;
            else if (c.fortTier == 2) c.supplyLimit += 15;
            break;
        }
    }
}

void SettlementSystem::triggerSuccession() {
    if (!s_instance) return;

    static int rulerGen = 1;
    rulerGen++;
    s_instance->deceasedKingTitle = s_instance->successorTitle;
    s_instance->successorTitle = "Ecgberht " + std::to_string(rulerGen);

    s_instance->successionEntries.clear();

    std::string playerK = getPlayerKingdomId();
    std::string capital = getPlayerCapitalCounty();

    std::vector<CountyDef*> ownedCounties;
    for (auto& c : s_instance->counties) {
        if (c.kingdomName == playerK) {
            ownedCounties.push_back(&c);
        }
    }

    if (ownedCounties.empty()) return;

    s_instance->successionEntries.push_back({
        capital,
        s_instance->successorTitle + " (Primary Heir)",
        "Capital Realm Seat",
        "Retained",
        sf::Color(90, 225, 90)
    });

    std::vector<std::string> juniorNames = {"Prince Aethelweard", "Prince Cynric", "Prince Osric"};
    size_t juniorIdx = 0;

    for (auto* c : ownedCounties) {
        if (c->countyName == capital) {
            c->vassalOpinion = std::clamp(c->vassalOpinion - 10, -100, 100);
            continue;
        }

        std::string jName = (juniorIdx < juniorNames.size()) ? juniorNames[juniorIdx++] : "Junior Kin";

        if (juniorIdx <= 2) {
            c->kingdomName = "Cadet " + playerK;
            c->vassalOpinion = -40;
            c->inFaction = true;

            s_instance->successionEntries.push_back({
                c->countyName,
                jName + " (Junior Sibling)",
                "Cadet Partition Split",
                "Seceded Realm",
                sf::Color(235, 75, 65)
            });

            spawnArmy(c->countyName, "Cadet " + playerK, 18);
        } else {
            c->vassalOpinion = std::clamp(c->vassalOpinion - 25, -100, 100);
            if (c->vassalOpinion < 0) {
                c->inFaction = true;
            }

            s_instance->successionEntries.push_back({
                c->countyName,
                jName + " (Vassal Lord)",
                "Secondary Appanage",
                "Discontent Vassal",
                sf::Color(240, 195, 65)
            });
        }
    }

    s_instance->shortReignTimer = 180.f;
    s_instance->successionModalOpen = true;
}

void SettlementSystem::spawnArmy(const std::string& county, const std::string& kingdom, int strength) {
    if (!s_instance) return;

    for (auto& a : s_instance->mapArmies) {
        if (a.currentCounty == county && a.ownerKingdom == kingdom) {
            a.strength += strength;
            return;
        }
    }

    sf::Vector2f spawnPos(440.f, 525.f);
    for (const auto& c : s_instance->counties) {
        if (c.countyName == county) {
            spawnPos = c.center + sf::Vector2f(20.f, -12.f);
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
    army.originCounty = county;
    army.isMoving = false;
    army.supply = 100.f;

    for (auto& c : s_instance->counties) {
        if (c.countyName == county && c.kingdomName == kingdom) {
            c.leviesRaised = true;
            break;
        }
    }

    s_instance->mapArmies.push_back(army);
    s_instance->selectedArmyId = static_cast<int>(army.id);
}

void SettlementSystem::disbandArmyInCounty(const std::string& county, const std::string& kingdom) {
    if (!s_instance) return;
    for (auto it = s_instance->mapArmies.begin(); it != s_instance->mapArmies.end();) {
        if (it->currentCounty == county && it->ownerKingdom == kingdom) {
            if (s_instance->selectedArmyId == static_cast<int>(it->id)) {
                s_instance->selectedArmyId = -1;
            }
            std::string oCounty = it->originCounty;
            for (auto& c : s_instance->counties) {
                if (c.countyName == oCounty && c.kingdomName == kingdom) {
                    c.leviesRaised = false;
                    break;
                }
            }
            it = s_instance->mapArmies.erase(it);
        } else {
            ++it;
        }
    }
}

bool SettlementSystem::hasArmyInCounty(const std::string& county, const std::string& kingdom) {
    if (!s_instance) return false;
    for (const auto& a : s_instance->mapArmies) {
        if (a.currentCounty == county && a.ownerKingdom == kingdom) {
            return true;
        }
    }
    return false;
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

    std::string fullKName = getKingdomDisplayName(newKingdom);
    for (auto& s : s_instance->realSettlements) {
        if (s.countyName == county) {
            s.kingdomName = fullKName;
            s.isAllied = (newKingdom == getPlayerKingdomId());
            break;
        }
    }

    std::string enemyK = s_instance->activeWar.enemyKingdom;
    s_instance->mapArmies.erase(
        std::remove_if(s_instance->mapArmies.begin(), s_instance->mapArmies.end(),
                       [&enemyK](const MapArmy& a) { return a.ownerKingdom == enemyK; }),
        s_instance->mapArmies.end()
    );

    for (const auto& al : s_instance->activeWarAllies) {
        s_instance->mapArmies.erase(
            std::remove_if(s_instance->mapArmies.begin(), s_instance->mapArmies.end(),
                           [&al](const MapArmy& a) { return a.ownerKingdom == al; }),
            s_instance->mapArmies.end()
        );
    }
    s_instance->activeWarAllies.clear();

    for (auto& c : s_instance->counties) {
        if (c.countyName == county) {
            c.vassalOpinion = 25;
            c.inFaction = false;
            break;
        }
    }

    getPlayerClaims().erase(county);
    if (enemyK != "Rebels") {
        s_instance->kingdomTruces[enemyK] = 5;
    }
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

void SettlementSystem::initTradeRoutes() {
    tradeRoutes.clear();

    TradeRouteNode thames;
    thames.name = "Thames River Highway";
    thames.waypoints = { {370.f, 530.f}, {420.f, 495.f}, {480.f, 520.f}, {540.f, 525.f} };
    thames.counties = { "Hampshire", "Berkshire", "Middlesex" };
    thames.baseToll = 3.4f;
    tradeRoutes.push_back(thames);

    TradeRouteNode watling;
    watling.name = "Watling Roman Road";
    watling.waypoints = { {480.f, 520.f}, {435.f, 430.f}, {375.f, 440.f}, {455.f, 350.f} };
    watling.counties = { "Middlesex", "Warwick", "Chester", "Yorkshire" };
    watling.baseToll = 4.2f;
    tradeRoutes.push_back(watling);

    TradeRouteNode northSea;
    northSea.name = "North Sea Coastway";
    northSea.waypoints = { {525.f, 465.f}, {495.f, 395.f}, {485.f, 335.f}, {440.f, 245.f}, {425.f, 195.f} };
    northSea.counties = { "Norfolk", "Lincoln", "Yorkshire", "Bamburgh", "Lothian" };
    northSea.baseToll = 3.0f;
    tradeRoutes.push_back(northSea);
}

const std::vector<TradeRouteNode>& SettlementSystem::getTradeRoutes() {
    static const std::vector<TradeRouteNode> empty;
    if (!s_instance) return empty;
    return s_instance->tradeRoutes;
}

bool SettlementSystem::isCountyOnTradeRoute(const std::string& county) {
    if (!s_instance) return false;
    for (const auto& tr : s_instance->tradeRoutes) {
        for (const auto& c : tr.counties) {
            if (c == county) return true;
        }
    }
    return false;
}

bool SettlementSystem::isTradeRouteRaided(const std::string& routeName) {
    if (!s_instance) return false;
    for (const auto& tr : s_instance->tradeRoutes) {
        if (tr.name == routeName) return tr.isRaided;
    }
    return false;
}

void SettlementSystem::setTradeRouteRaided(const std::string& routeName, bool raided, const std::string& raider) {
    if (!s_instance) return;
    for (auto& tr : s_instance->tradeRoutes) {
        if (tr.name == routeName) {
            tr.isRaided = raided;
            tr.raiderKingdom = raider;
            break;
        }
    }
}

float SettlementSystem::getKingdomTradeIncome(const std::string& kingdom) {
    if (!s_instance) return 0.f;
    float income = 0.f;
    for (const auto& tr : s_instance->tradeRoutes) {
        if (tr.isRaided && tr.raiderKingdom == kingdom) {
            income += tr.baseToll * 1.6f;
            continue;
        }
        if (tr.isRaided) continue;

        int ownedConnected = 0;
        for (const auto& cName : tr.counties) {
            for (const auto& c : s_instance->counties) {
                if (c.countyName == cName && c.kingdomName == kingdom) {
                    ownedConnected++;
                }
            }
        }
        if (ownedConnected > 0) {
            income += tr.baseToll * (0.6f + 0.4f * static_cast<float>(ownedConnected));
        }
    }
    return income;
}

float SettlementSystem::getKingdomArmyUpkeep(const std::string& kingdom) {
    if (!s_instance) return 0.f;
    float upkeep = 0.f;
    for (const auto& a : s_instance->mapArmies) {
        if (a.ownerKingdom == kingdom) {
            upkeep += static_cast<float>(a.strength) * 0.12f;
        }
    }
    return upkeep;
}

int SettlementSystem::getKingdomRaisedTroops(const std::string& kingdom) {
    if (!s_instance) return 0;
    int sum = 0;
    for (const auto& a : s_instance->mapArmies) {
        if (a.ownerKingdom == kingdom) sum += a.strength;
    }
    return sum;
}

int SettlementSystem::getKingdomDemesneCount(const std::string& kingdom) {
    if (!s_instance) return 0;
    int count = 0;
    for (const auto& c : s_instance->counties) {
        if (c.kingdomName == kingdom) count++;
    }
    return count;
}

bool SettlementSystem::isWarActive() {
    if (!s_instance) return false;
    return s_instance->activeWar.active;
}

SettlementSystem::SettlementSystem() {
    s_instance = this;
    warBadgeBounds = sf::FloatRect(60.f, 630.f, 372.f, 58.f);
    WorldMapRepository::getInstance().load("assets/data/world_map.json");
    initTradeRoutes();

    fontLoaded = font.loadFromFile("assets/fonts/Cinzel-Bold.ttf") ||
                 font.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
                 font.loadFromFile("font.ttf") ||
                 font.loadFromFile("assets/fonts/font.ttf");

    buildMapGeometry();
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
        std::string pK = getPlayerKingdomId();
        if (rs.kingdomName.find(pK) != std::string::npos || isAllyInWar(rs.kingdomName)) {
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
    updateRealmLabels(dt);
    updateMapCamera(dt);

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

    for (auto& tr : tradeRoutes) {
        tr.isRaided = false;
        tr.raiderKingdom.clear();
        for (const auto& a : mapArmies) {
            if (a.isMoving) continue;
            for (const auto& cName : tr.counties) {
                if (a.currentCounty == cName) {
                    for (const auto& c : counties) {
                        if (c.countyName == cName && c.kingdomName != a.ownerKingdom) {
                            tr.isRaided = true;
                            tr.raiderKingdom = a.ownerKingdom;
                            break;
                        }
                    }
                }
                if (tr.isRaided) break;
            }
            if (tr.isRaided) break;
        }
    }

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

                bool isAlly = false;
                for (const auto& al : activeWarAllies) {
                    if (a.ownerKingdom == al) {
                        isAlly = true;
                        break;
                    }
                }
                if (isAlly && !a.inCombat && !a.isMoving) {
                    if (a.currentCounty != activeWar.targetCounty) {
                        a.targetPos = targetCountyCenter + sf::Vector2f(20.f, -12.f);
                        a.targetCounty = activeWar.targetCounty;
                        a.isMoving = true;
                    }
                }
            }
        }
    }

    if (callAllyStatusTimer > 0.f) {
        callAllyStatusTimer = std::max(0.f, callAllyStatusTimer - dt);
    }

    std::unordered_map<std::string, int> countyTroopCounts;
    for (const auto& a : mapArmies) {
        if (!a.currentCounty.empty()) {
            countyTroopCounts[a.currentCounty] += a.strength;
        }
    }

    for (auto& a : mapArmies) {
        if (a.isMoving) continue;
        int limit = 30;
        bool isHostileUnoccupied = false;
        for (const auto& c : counties) {
            if (c.countyName == a.currentCounty) {
                limit = c.supplyLimit;
                if (activeWar.active && a.ownerKingdom == activeWar.attackerKingdom &&
                    (c.kingdomName == activeWar.enemyKingdom || c.countyName == activeWar.targetCounty) &&
                    !c.isOccupied) {
                    isHostileUnoccupied = true;
                    limit = static_cast<int>(static_cast<float>(limit) * 0.55f);
                }
                break;
            }
        }

        int presentTroops = countyTroopCounts[a.currentCounty];
        bool overLimit = (presentTroops > limit);

        if (overLimit) {
            a.supply = std::max(0.f, a.supply - dt * 12.f);
        } else if (isHostileUnoccupied) {
            a.supply = std::max(0.f, a.supply - dt * 3.5f);
        } else {
            a.supply = std::min(100.f, a.supply + dt * 8.f);
        }

        if (a.supply <= 0.f || overLimit) {
            a.sufferingAttrition = true;
            a.attritionTimer += dt;
            if (a.attritionTimer >= 1.5f) {
                a.attritionTimer = 0.f;
                int loss = std::max(1, static_cast<int>(static_cast<float>(a.strength) * 0.05f));
                a.strength = std::max(1, a.strength - loss);
            }
        } else {
            a.sufferingAttrition = false;
            a.attritionTimer = 0.f;
        }
    }

    for (size_t i = 0; i < mapArmies.size(); ++i) {
        mapArmies[i].inCombat = false;
    }

    auto areArmiesAllied = [&](const std::string& k1, const std::string& k2) -> bool {
        if (k1 == k2) return true;
        std::string pK = getPlayerKingdomId();
        bool k1PlayerSide = (k1 == pK);
        for (const auto& al : activeWarAllies) if (k1 == al) k1PlayerSide = true;
        bool k2PlayerSide = (k2 == pK);
        for (const auto& al : activeWarAllies) if (k2 == al) k2PlayerSide = true;
        return (k1PlayerSide && k2PlayerSide);
    };

    for (size_t i = 0; i < mapArmies.size(); ++i) {
        for (size_t j = i + 1; j < mapArmies.size(); ++j) {
            if (!areArmiesAllied(mapArmies[i].ownerKingdom, mapArmies[j].ownerKingdom)) {
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
                bool isAttackerSide = (it->ownerKingdom == activeWar.attackerKingdom);
                for (const auto& al : activeWarAllies) {
                    if (it->ownerKingdom == al) isAttackerSide = true;
                }

                if (isAttackerSide) {
                    activeWar.warScore = std::max(-100.f, activeWar.warScore - 25.f);
                } else if (it->ownerKingdom == activeWar.enemyKingdom) {
                    activeWar.warScore = std::min(100.f, activeWar.warScore + 30.f);
                }
            }
            for (auto& c : counties) {
                if (c.countyName == it->originCounty && c.kingdomName == it->ownerKingdom) {
                    c.leviesRaised = false;
                    break;
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
            bool isFriendlySieger = (sieger == activeWar.attackerKingdom);
            for (const auto& al : activeWarAllies) {
                if (sieger == al) isFriendlySieger = true;
            }

            if (isFriendlySieger && (c.kingdomName == activeWar.enemyKingdom || c.countyName == activeWar.targetCounty)) {
                int totalSiegeTroops = countyTroopCounts[c.countyName];
                int minRequired = (c.fortTier == 0) ? 8 : ((c.fortTier == 1) ? 16 : 24);

                if (totalSiegeTroops >= minRequired) {
                    float resistance = 1.0f + c.fortTier * 1.6f;
                    c.siegeProgress = std::min(100.f, c.siegeProgress + (dt * 8.f) / resistance);

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

    if (shortReignTimer > 0.f) {
        shortReignTimer = std::max(0.f, shortReignTimer - dt);
    }

    cachedRegistry = &registry;
    if (lawCooldownTimer > 0.f) lawCooldownTimer = std::max(0.f, lawCooldownTimer - dt);
    if (lawStatusTimer > 0.f) lawStatusTimer = std::max(0.f, lawStatusTimer - dt);

    independenceFaction.memberCounties.clear();
    int factionLevyStrength = 0;
    int royalLevyStrength = 0;

    std::string playerK = getPlayerKingdomId();
    for (const auto& a : mapArmies) {
        if (a.ownerKingdom == playerK) {
            royalLevyStrength += a.strength;
        }
    }
    royalLevyStrength = std::max(20, royalLevyStrength);

    int authOpMod = getAuthorityVassalOpinionMod();
    for (auto& c : counties) {
        if (c.kingdomName == playerK) {
            int netOpinion = c.vassalOpinion + authOpMod;
            if (netOpinion < 0) {
                c.inFaction = true;
                independenceFaction.memberCounties.push_back(c.countyName);
                factionLevyStrength += 18;
            } else {
                c.inFaction = false;
            }
        }
    }

    independenceFaction.powerRatio = (static_cast<float>(factionLevyStrength) / static_cast<float>(royalLevyStrength)) * 100.f;

    if (!independenceFaction.memberCounties.empty() && !activeWar.active) {
        float growthMultiplier = (shortReignTimer > 0.f ? 1.5f : 1.0f);
        float authDiscontentMult = (crownAuthority == 1) ? 0.55f :
                                   ((crownAuthority == 2) ? 1.0f :
                                   ((crownAuthority == 3) ? 1.45f : 2.2f));
        float growthSpeed = (independenceFaction.powerRatio >= 65.f)
            ? (0.28f * growthMultiplier * authDiscontentMult)
            : (0.08f * growthMultiplier * authDiscontentMult);

        independenceFaction.discontent = std::min(100.f, independenceFaction.discontent + dt * growthSpeed);

        if (independenceFaction.discontent >= 100.f && independenceFaction.powerRatio >= 60.f) {
            std::string rebelCounty = independenceFaction.memberCounties.front();
            triggerCivilWar(rebelCounty);
        }
    } else {
        independenceFaction.discontent = std::max(0.f, independenceFaction.discontent - dt * 0.45f);
    }

    for (auto& ca : councilAssignments) {
        if (ca.mission == CouncilMissionType::FabricateClaim) {
            ca.progress = std::min(ca.maxProgress, ca.progress + dt * 6.5f);
            if (ca.progress >= ca.maxProgress) {
                getPlayerClaims().insert(ca.targetCounty);
                ca.mission = CouncilMissionType::None;
            }
        } else if (ca.mission == CouncilMissionType::TrainLevies) {
            ca.progress = std::min(ca.maxProgress, ca.progress + dt * 4.f);
        } else if (ca.mission == CouncilMissionType::DevelopCounty) {
            ca.progress += dt * 3.5f;
            if (ca.progress >= ca.maxProgress) {
                ca.progress = 0.f;
                for (auto& v : realSettlements) {
                    if (v.countyName == ca.targetCounty) {
                        sim::VillageData* vd = registry.getVillage(v.villageId);
                        if (vd) {
                            vd->food += 30;
                            vd->wood += 20;
                        }
                        break;
                    }
                }
            }
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
    if (realSettlements.empty()) {
        const std::string capital = getPlayerCapitalCounty();
        for (const auto& c : counties) {
            if (c.countyName == capital) return c.center;
        }
        return mapCenter;
    }
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
        if (s->isAllied || s->kingdomName.find(getPlayerKingdomId()) != std::string::npos || isAllyInWar(s->kingdomName)) {
            return true;
        }
    }
    if (!realSettlements.empty() && x <= realSettlements[0].centerX) return true;
    return false;
}