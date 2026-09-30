#include "ui/MapActionMenu.h"
#include "simulation/WarfareManager.h"
#include <algorithm>

MapActionMenu::MapActionMenu()
    : font(nullptr), active(false), position(0.f, 0.f),
      menuWidth(265.f), menuHeight(280.f), isKingdomLevel(false), hoveredIdx(-1),
      statusColor(sf::Color::White), statusTimer(0.f) {}

void MapActionMenu::init(const sf::Font& f) {
    font = &f;
}

void MapActionMenu::open(const RealSettlement& rs, sf::Vector2f screenPos, sim::SimulationRegistry& reg, sim::EntityID playerApeId, bool kingdomLevel) {
    targetSettlement = rs;
    isKingdomLevel = kingdomLevel;
    active = true;
    hoveredIdx = -1;

    position.x = std::clamp(screenPos.x, 140.f, 1280.f - 140.f - menuWidth);
    position.y = std::clamp(screenPos.y - 40.f, 110.f, 630.f - menuHeight);

    rebuildOptions(reg, playerApeId);
}

void MapActionMenu::close() {
    active = false;
    hoveredIdx = -1;
    options.clear();
    optionBounds.clear();
}

void MapActionMenu::update(float dt) {
    if (statusTimer > 0.f) {
        statusTimer -= dt;
        if (statusTimer <= 0.f) {
            statusMessage.clear();
        }
    }
}

void MapActionMenu::rebuildOptions(sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    options.clear();
    optionBounds.clear();

    sim::ApeData* player = reg.getApe(playerApeId);
    sim::KingdomData* playerKingdom = (player && player->currentKingdom != 0) ? reg.getKingdom(player->currentKingdom) : nullptr;
    sim::VillageData* playerVillage = (player && player->villageId != 0) ? reg.getVillage(player->villageId) : nullptr;

    sim::KingdomData* targetKingdom = (targetSettlement.kingdomId != 0) ? reg.getKingdom(targetSettlement.kingdomId) : nullptr;
    sim::VillageData* targetVillage = reg.getVillage(targetSettlement.villageId);

    std::string playerKName = playerKingdom ? playerKingdom->name : "Wessex";
    int curAmber = player ? player->amberCount : 0;

    bool isForeignOccupied = (targetSettlement.kingdomName == "East Anglia" ||
                              targetSettlement.kingdomName == "Mercia" ||
                              targetSettlement.kingdomName == "Northumbria" ||
                              targetSettlement.kingdomName == "Alba" ||
                              targetSettlement.kingdomName == "Ireland" ||
                              targetSettlement.kingdomName == "Cornwall");

    bool isSelfRealm = !isForeignOccupied && (targetSettlement.countyName == "Hampshire" ||
                                              targetSettlement.countyName == "Wight" ||
                                              targetSettlement.kingdomName == "Wessex" ||
                                              targetSettlement.kingdomName == playerKName);

    if (isSelfRealm) {
        bool hasArmyHere = SettlementSystem::hasArmyInCounty(targetSettlement.countyName, playerKName);

        if (!hasArmyHere) {
            options.push_back({
                "Muster Warband",
                "Assemble defense levies at hearth",
                "Free",
                true,
                sf::Color(245, 195, 65),
                [this, &reg, playerKingdom, targetVillage, playerApeId, playerKName]() {
                    if (playerKingdom && targetVillage) {
                        sim::WarfareManager::issueMusterOrder(reg, playerKingdom->id, targetVillage->id, playerApeId);
                    }
                    SettlementSystem::spawnArmy(targetSettlement.countyName, playerKName, 25);
                    statusMessage = "Warband Called to Arms!";
                    statusColor = sf::Color(100, 240, 100);
                    statusTimer = 2.5f;
                }
            });
        } else {
            options.push_back({
                "Disband Warband",
                "Stand down levies and return to hearth",
                "Disband",
                true,
                sf::Color(225, 110, 80),
                [this, playerKName]() {
                    SettlementSystem::disbandArmyInCounty(targetSettlement.countyName, playerKName);
                    statusMessage = "Warband Disbanded";
                    statusColor = sf::Color(240, 150, 100);
                    statusTimer = 2.5f;
                }
            });
        }

        options.push_back({
            "Distribute Rations",
            "Feed clan elders to raise loyalty",
            "-35 Food",
            (playerVillage && playerVillage->food >= 35),
            sf::Color(115, 215, 115),
            [this, playerVillage]() {
                if (playerVillage && playerVillage->food >= 35) {
                    playerVillage->food -= 35;
                    SettlementSystem::swayVassal(targetSettlement.countyName, 10);
                    statusMessage = "Clan Loyalty Reinforced (+10 Opinion)";
                    statusColor = sf::Color(115, 225, 115);
                    statusTimer = 2.5f;
                }
            }
        });

        int vOpinion = SettlementSystem::getVassalOpinion(targetSettlement.countyName);
        bool inFaction = SettlementSystem::isCountyInFaction(targetSettlement.countyName);

        std::string swayCost = (curAmber >= 20) ? "-20 Amber" : "Need 20 Amber";
        std::string swayTitle = std::string("Sway Chieftain (") + (vOpinion >= 0 ? "+" : "") + std::to_string(vOpinion) + ")";
        options.push_back({
            swayTitle,
            inFaction ? "Bribe chieftain to abandon rebel faction" : "Send diplomatic gifts to ensure loyalty",
            swayCost,
            (curAmber >= 20),
            inFaction ? sf::Color(235, 160, 50) : sf::Color(110, 205, 125),
            [this, player]() {
                if (player && player->amberCount >= 20) {
                    player->amberCount -= 20;
                    SettlementSystem::swayVassal(targetSettlement.countyName, 25);
                    statusMessage = "Chieftain Swayed (+25 Opinion)!";
                    statusColor = sf::Color(120, 230, 130);
                    statusTimer = 3.0f;
                }
            }
        });

        if (inFaction) {
            options.push_back({
                "Confront Faction (Provoke)",
                "Force rebels into open war on your terms",
                "Revolt",
                true,
                sf::Color(225, 55, 45),
                [this]() {
                    SettlementSystem::triggerCivilWar(targetSettlement.countyName);
                    statusMessage = "Civil War Broken Out in " + targetSettlement.countyName + "!";
                    statusColor = sf::Color(245, 60, 50);
                    statusTimer = 3.0f;
                }
            });
        }

        const auto* warChiefTask = SettlementSystem::getCouncilAssignment(sim::CouncilRole::WarChief);
        bool isDrillingHere = (warChiefTask && warChiefTask->mission == CouncilMissionType::TrainLevies && warChiefTask->targetCounty == targetSettlement.countyName);

        options.push_back({
            isDrillingHere ? "Drilling Levies (Active)" : "Task War Chief: Train Levies",
            isDrillingHere ? "War Chief is actively drilling recruits" : "Station War Chief to fortify garrison",
            isDrillingHere ? "Active" : "War Chief",
            !isDrillingHere,
            isDrillingHere ? sf::Color(120, 210, 120) : sf::Color(225, 80, 65),
            [this]() {
                SettlementSystem::assignCouncilMission(sim::CouncilRole::WarChief, CouncilMissionType::TrainLevies, targetSettlement.countyName);
                statusMessage = "War Chief Stationed to Train Levies";
                statusColor = sf::Color(235, 110, 85);
                statusTimer = 2.5f;
            }
        });

        const auto* builderTask = SettlementSystem::getCouncilAssignment(sim::CouncilRole::ChiefBuilder);
        bool isBuildingHere = (builderTask && builderTask->mission == CouncilMissionType::DevelopCounty && builderTask->targetCounty == targetSettlement.countyName);

        options.push_back({
            isBuildingHere ? "Developing County (Active)" : "Task Builder: Develop County",
            isBuildingHere ? "Chief Builder oversees local expansion" : "Station Builder to boost food & wood",
            isBuildingHere ? "Active" : "Builder",
            !isBuildingHere,
            isBuildingHere ? sf::Color(120, 210, 120) : sf::Color(85, 185, 105),
            [this]() {
                SettlementSystem::assignCouncilMission(sim::CouncilRole::ChiefBuilder, CouncilMissionType::DevelopCounty, targetSettlement.countyName);
                statusMessage = "Chief Builder Stationed to Develop County";
                statusColor = sf::Color(95, 215, 115);
                statusTimer = 2.5f;
            }
        });

        int curFort = SettlementSystem::getCountyFortTier(targetSettlement.countyName);
        int curWood = playerVillage ? playerVillage->wood : 0;

        if (curFort == 0) {
            bool canBuildPalisade = (curWood >= 40 && curAmber >= 15);
            options.push_back({
                "Build Wooden Palisade",
                "Erect timber ramparts (+10 Supply, 2x Siege Delay)",
                "-40 Wood, -15 Amber",
                canBuildPalisade,
                canBuildPalisade ? sf::Color(185, 140, 75) : sf::Color(95, 88, 85),
                [this, player, playerVillage]() {
                    if (player && playerVillage && playerVillage->wood >= 40 && player->amberCount >= 15) {
                        playerVillage->wood -= 40;
                        player->amberCount -= 15;
                        SettlementSystem::upgradeCountyFort(targetSettlement.countyName);
                        statusMessage = "Wooden Palisade Erected in " + targetSettlement.countyName + "!";
                        statusColor = sf::Color(215, 175, 95);
                        statusTimer = 3.0f;
                    }
                }
            });
        } else if (curFort == 1) {
            bool canBuildHillfort = (curWood >= 70 && curAmber >= 25);
            options.push_back({
                "Erect Stone Hillfort",
                "Upgrade to masonry citadel (+15 Supply, 4x Siege Delay)",
                "-70 Wood, -25 Amber",
                canBuildHillfort,
                canBuildHillfort ? sf::Color(140, 180, 215) : sf::Color(95, 88, 85),
                [this, player, playerVillage]() {
                    if (player && playerVillage && playerVillage->wood >= 70 && player->amberCount >= 25) {
                        playerVillage->wood -= 70;
                        player->amberCount -= 25;
                        SettlementSystem::upgradeCountyFort(targetSettlement.countyName);
                        statusMessage = "Stone Hillfort Completed in " + targetSettlement.countyName + "!";
                        statusColor = sf::Color(160, 205, 245);
                        statusTimer = 3.0f;
                    }
                }
            });
        } else {
            options.push_back({
                "Hillfort Stronghold (Max)",
                "Citadel is fortified to maximum tier",
                "Max Tier",
                false,
                sf::Color(105, 95, 90),
                nullptr
            });
        }

        if (targetSettlement.countyName == "Hampshire") {
            options.push_back({
                "Dynastic Succession",
                "Pass mantle of Alpha and partition realm",
                "Demise",
                true,
                sf::Color(220, 160, 60),
                [this]() {
                    SettlementSystem::triggerSuccession();
                    statusMessage = "Succession Triggered!";
                    statusColor = sf::Color(255, 215, 90);
                    statusTimer = 3.0f;
                }
            });
        }
        return;
    }   

bool alreadyAtWar = SettlementSystem::isAtWarWith(targetSettlement.kingdomName);
    if (!alreadyAtWar && playerKingdom && targetKingdom && playerKingdom->relations.count(targetKingdom->id)) {
        alreadyAtWar = (playerKingdom->relations[targetKingdom->id] == sim::DiplomacyStatus::War);
    }

    bool hasDeJureClaim = (!targetSettlement.deJureKingdom.empty() &&
                          (targetSettlement.deJureKingdom == playerKName ||
                           targetSettlement.deJureKingdom == "Wessex" ||
                           playerKName.find(targetSettlement.deJureKingdom) != std::string::npos ||
                           targetSettlement.deJureKingdom.find(playerKName) != std::string::npos));

    bool hasFabricatedClaim = (SettlementSystem::getPlayerClaims().count(targetSettlement.countyName) > 0);
    bool hasValidCB = hasDeJureClaim || hasFabricatedClaim;

    if (isKingdomLevel) {
        bool isReigningKing = (playerKingdom && player && playerKingdom->currentKingId == player->id);
        bool hasInvasionStrength = (playerKingdom && playerKingdom->militaryStrength >= 30);
        bool hasInvasionAmber = (curAmber >= 100);
        bool canInvade = isReigningKing && hasInvasionStrength && hasInvasionAmber && !alreadyAtWar;

        std::string invDesc = alreadyAtWar ? "Already at war with this realm"
                            : (canInvade ? "Total territorial subjugation"
                                         : "Requires: King rank, 100 Amber, and 30+ Military");

        options.push_back({
            "Kingdom Invasion",
            invDesc,
            canInvade ? "-100 Amber" : "Locked",
            canInvade,
            canInvade ? sf::Color(225, 45, 40) : sf::Color(90, 82, 78),
            [this, &reg, player, playerKingdom, targetKingdom]() {
                if (player && playerKingdom && targetKingdom) {
                    player->amberCount -= 100;
                    sim::WarfareManager::declareWar(reg, playerKingdom->id, targetKingdom->id, "Kingdom Invasion CB");
                    statusMessage = "Kingdom Invasion Declared on " + targetSettlement.kingdomName + "!";
                    statusColor = sf::Color(245, 50, 45);
                    statusTimer = 3.0f;
                }
            }
        });

        options.push_back({
            "Form Royal Alliance",
            "Dynastic pair-bond pact",
            "Pact",
            (targetKingdom != nullptr && !alreadyAtWar),
            sf::Color(80, 170, 245),
            [this, playerKingdom, targetKingdom]() {
                if (playerKingdom && targetKingdom) {
                    playerKingdom->relations[targetKingdom->id] = sim::DiplomacyStatus::Alliance;
                    targetKingdom->relations[playerKingdom->id] = sim::DiplomacyStatus::Alliance;
                }
                statusMessage = "Royal Alliance Sealed";
                statusColor = sf::Color(90, 185, 255);
                statusTimer = 3.0f;
            }
        });

        options.push_back({
            "Send Royal Tribute",
            "Caravan of sacred amber gifts",
            "-50 Amber",
            (curAmber >= 50),
            sf::Color(240, 205, 75),
            [this, player, targetVillage, targetKingdom]() {
                if (player && player->amberCount >= 50) {
                    player->amberCount -= 50;
                    if (targetVillage) targetVillage->personalOpinions[player->id] += 30;
                    if (targetKingdom && player->currentKingdom != 0) {
                        targetKingdom->borderTension[player->currentKingdom] = std::max(0.f, targetKingdom->borderTension[player->currentKingdom] - 30.f);
                    }
                    statusMessage = "Tribute Accepted by Crown";
                    statusColor = sf::Color(240, 215, 80);
                    statusTimer = 2.5f;
                }
            }
        });
    } else {
        bool hasTruce = SettlementSystem::hasTruceWith(targetSettlement.kingdomName);
        bool canDeclareCountyWar = !alreadyAtWar && !hasTruce && hasValidCB;

        std::string cWarTitle = hasDeJureClaim ? ("War for " + targetSettlement.countyName + " (De Jure)")
                              : (hasFabricatedClaim ? ("War for " + targetSettlement.countyName + " (Claim)")
                                                    : ("War for " + targetSettlement.countyName + " (No Claim)"));

        std::string cWarDesc = hasDeJureClaim ? ("Press De Jure rights of " + targetSettlement.deJureKingdom)
                             : (hasFabricatedClaim ? ("Press Shaman's forged claim on " + targetSettlement.countyName)
                                                   : ("No legal claim on this county"));

        std::string warBadgeCost = alreadyAtWar ? "At War" : (hasTruce ? "Truce" : (hasValidCB ? "County CB" : "No Claim"));

        if (alreadyAtWar) {
            cWarTitle = "War for " + targetSettlement.countyName + " (Active)";
            cWarDesc = "War is currently active against " + targetSettlement.kingdomName;
        } else if (hasTruce) {
            cWarDesc = "A truce is currently in effect with " + targetSettlement.kingdomName;
        }

        options.push_back({
            cWarTitle,
            cWarDesc,
            warBadgeCost,
            canDeclareCountyWar,
            canDeclareCountyWar ? sf::Color(235, 60, 60) : sf::Color(95, 88, 85),
            [this, &reg, playerKName, hasDeJureClaim, playerKingdom, targetKingdom, targetVillage]() {
                std::string reason = hasDeJureClaim ? ("De Jure Claim on " + targetSettlement.countyName)
                                                    : ("Fabricated Claim on " + targetSettlement.countyName);

                SettlementSystem::startWar(targetSettlement.countyName, playerKName, targetSettlement.kingdomName, reason);

                sim::KingdomID myKId = playerKingdom ? playerKingdom->id : 1;
                sim::KingdomID enemyKId = targetKingdom ? targetKingdom->id : targetSettlement.kingdomId;
                if (enemyKId == 0) enemyKId = 2;
                sim::VillageID goalVId = targetVillage ? targetVillage->id : targetSettlement.villageId;
                sim::WarfareManager::declareWarWithGoal(reg, myKId, enemyKId, goalVId, reason);

                statusMessage = "War for " + targetSettlement.countyName + " Declared!";
                statusColor = sf::Color(245, 60, 60);
                statusTimer = 3.0f;
            }
        });
        
        const auto* shamanTask = SettlementSystem::getCouncilAssignment(sim::CouncilRole::Shaman);
        bool isForgingHere = (shamanTask && shamanTask->mission == CouncilMissionType::FabricateClaim && shamanTask->targetCounty == targetSettlement.countyName);

        std::string shamTitle = hasFabricatedClaim ? "Claim Fabricated"
                              : (isForgingHere ? ("Shaman Forging Claim (" + std::to_string(static_cast<int>(shamanTask->progress)) + "%)")
                                               : "Deploy Shaman: Fabricate Claim");
        std::string shamDesc = hasFabricatedClaim ? "Ancestral rights firmly established"
                             : (isForgingHere ? "Shaman researching ancient lineage records"
                                              : "Send Shaman to forge legal Casus Belli");
        std::string shamCost = hasFabricatedClaim ? "Claimed" : (isForgingHere ? "Active" : "-25 Amber");

        bool canDeployShaman = (!hasFabricatedClaim && !hasDeJureClaim && !isForgingHere && curAmber >= 25);

        options.push_back({
            shamTitle,
            shamDesc,
            shamCost,
            canDeployShaman,
            hasFabricatedClaim ? sf::Color(120, 210, 120) : (isForgingHere ? sf::Color(220, 180, 70) : sf::Color(190, 130, 240)),
            [this, player]() {
                if (player && player->amberCount >= 25) {
                    player->amberCount -= 25;
                    SettlementSystem::assignCouncilMission(sim::CouncilRole::Shaman, CouncilMissionType::FabricateClaim, targetSettlement.countyName);
                    statusMessage = "Shaman Deployed to " + targetSettlement.countyName;
                    statusColor = sf::Color(205, 145, 255);
                    statusTimer = 3.0f;
                }
            }
        });

        options.push_back({
            "Bribe Chieftain",
            "Gain local elder favor",
            "-25 Amber",
            (curAmber >= 25),
            sf::Color(240, 215, 80),
            [this, player, targetVillage]() {
                if (player && player->amberCount >= 25) {
                    player->amberCount -= 25;
                    if (targetVillage) targetVillage->personalOpinions[player->id] += 20;
                    statusMessage = "Chieftain Bribed (+20 Opinion)";
                    statusColor = sf::Color(240, 215, 80);
                    statusTimer = 2.5f;
                }
            }
        });
    }
}

bool MapActionMenu::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView) {
    if (!active) return false;

    float cardH = 44.f;
    float startY = position.y + 36.f;
    float currentH = startY + options.size() * (cardH + 4.f) + 12.f - position.y;
    menuHeight = currentH;

    sf::FloatRect menuRect(position.x, position.y, menuWidth, menuHeight);

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        close();
        return true;
    }

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), letterboxView);
        hoveredIdx = -1;
        for (size_t i = 0; i < optionBounds.size(); ++i) {
            if (optionBounds[i].contains(mPos) && options[i].isEnabled) {
                hoveredIdx = static_cast<int>(i);
                break;
            }
        }
        if (menuRect.contains(mPos)) return true;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);

        for (size_t i = 0; i < optionBounds.size(); ++i) {
            if (optionBounds[i].contains(mPos)) {
                if (options[i].isEnabled && options[i].onExecute) {
                    options[i].onExecute();
                    close();
                }
                return true;
            }
        }

        if (menuRect.contains(mPos)) {
            return true;
        }

        close();
        return false;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);
        if (!menuRect.contains(mPos)) {
            close();
        }
    }

    return false;
}

void MapActionMenu::draw(sf::RenderWindow& window) {
    if (!active || !font) {
        if (!statusMessage.empty() && statusTimer > 0.f) {
            sf::RectangleShape toast(sf::Vector2f(320.f, 28.f));
            toast.setPosition(480.f, 114.f);
            toast.setFillColor(sf::Color(25, 18, 12, 245));
            toast.setOutlineColor(statusColor);
            toast.setOutlineThickness(1.2f);
            window.draw(toast);

            sf::Text tTxt(statusMessage, *font, 11);
            tTxt.setStyle(sf::Text::Bold);
            tTxt.setFillColor(statusColor);
            sf::FloatRect tb = tTxt.getLocalBounds();
            tTxt.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
            tTxt.setPosition(640.f, 128.f);
            window.draw(tTxt);
        }
        return;
    }

    float cardH = 44.f;
    float startY = position.y + 36.f;
    float totalH = 36.f + options.size() * (cardH + 4.f) + 10.f;

    sf::RectangleShape shadow(sf::Vector2f(menuWidth + 4.f, totalH + 4.f));
    shadow.setPosition(position.x + 2.f, position.y + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 185));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(menuWidth, totalH));
    panel.setPosition(position);
    panel.setFillColor(sf::Color(22, 16, 12, 250));
    panel.setOutlineColor(sf::Color(185, 140, 65));
    panel.setOutlineThickness(1.5f);
    window.draw(panel);

    sf::RectangleShape header(sf::Vector2f(menuWidth - 6.f, 26.f));
    header.setPosition(position.x + 3.f, position.y + 3.f);
    header.setFillColor(sf::Color(44, 28, 16));
    window.draw(header);

    std::string mainTitle = isKingdomLevel ? targetSettlement.kingdomName : targetSettlement.countyName;
    sf::Text titleText(mainTitle, *font, 11);
    titleText.setStyle(sf::Text::Bold);
    titleText.setFillColor(sf::Color(255, 230, 140));
    titleText.setPosition(position.x + 8.f, position.y + 7.f);
    window.draw(titleText);

    int sLimit = SettlementSystem::getCountySupplyLimit(targetSettlement.countyName);
    int curTroops = SettlementSystem::getCountyTroops(targetSettlement.countyName);
    std::string subTitle = isKingdomLevel ? "Realm" : ("Supply: " + std::to_string(curTroops) + "/" + std::to_string(sLimit));
    sf::Text realmSub(subTitle, *font, 9);
    realmSub.setFillColor(sf::Color(190, 175, 145));
    sf::FloatRect rsb = realmSub.getLocalBounds();
    realmSub.setPosition(position.x + menuWidth - rsb.width - 8.f, position.y + 8.f);
    window.draw(realmSub);

    optionBounds.clear();

    for (size_t i = 0; i < options.size(); ++i) {
        float oy = startY + i * (cardH + 4.f);
        sf::FloatRect ob(position.x + 6.f, oy, menuWidth - 12.f, cardH);
        optionBounds.push_back(ob);

        bool isHov = (hoveredIdx == static_cast<int>(i));
        const auto& opt = options[i];

        sf::RectangleShape card(sf::Vector2f(ob.width, ob.height));
        card.setPosition(ob.left, ob.top);

        if (!opt.isEnabled) {
            card.setFillColor(sf::Color(16, 12, 10, 160));
            card.setOutlineColor(sf::Color(55, 45, 38));
        } else if (isHov) {
            card.setFillColor(sf::Color(50, 36, 24, 250));
            card.setOutlineColor(opt.accentColor);
        } else {
            card.setFillColor(sf::Color(28, 20, 15, 230));
            card.setOutlineColor(sf::Color(85, 62, 38));
        }
        card.setOutlineThickness(1.f);
        window.draw(card);

        sf::RectangleShape accentStripe(sf::Vector2f(3.f, ob.height));
        accentStripe.setPosition(ob.left, ob.top);
        accentStripe.setFillColor(opt.isEnabled ? opt.accentColor : sf::Color(65, 55, 48));
        window.draw(accentStripe);

        sf::Text optTitle(opt.title, *font, 11);
        optTitle.setStyle(sf::Text::Bold);
        optTitle.setFillColor(opt.isEnabled ? (isHov ? sf::Color::White : sf::Color(245, 235, 215)) : sf::Color(120, 115, 110));
        optTitle.setPosition(ob.left + 8.f, ob.top + 4.f);
        window.draw(optTitle);

        if (isHov) {
            sf::Text optCost(opt.costText, *font, 9);
            optCost.setStyle(sf::Text::Bold);
            optCost.setFillColor(opt.isEnabled ? opt.accentColor : sf::Color(115, 100, 95));
            sf::FloatRect ocb = optCost.getLocalBounds();
            optCost.setPosition(ob.left + ob.width - ocb.width - 6.f, ob.top + 5.f);
            window.draw(optCost);
        }

        sf::Text optSub(opt.subtitle, *font, 9);
        optSub.setStyle(sf::Text::Italic);
        optSub.setFillColor(opt.isEnabled ? sf::Color(175, 160, 135) : sf::Color(95, 90, 85));
        optSub.setPosition(ob.left + 8.f, ob.top + 22.f);
        window.draw(optSub);
    }
}