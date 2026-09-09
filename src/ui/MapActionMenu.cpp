#include "ui/MapActionMenu.h"
#include "simulation/WarfareManager.h"
#include <algorithm>

MapActionMenu::MapActionMenu()
    : font(nullptr), active(false), position(0.f, 0.f),
      menuWidth(235.f), menuHeight(280.f), isKingdomLevel(false), hoveredIdx(-1),
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

    bool isSelfRealm = false;
    if (playerKingdom && targetSettlement.kingdomId == playerKingdom->id) isSelfRealm = true;
    else if (!playerKingdom && playerVillage && targetSettlement.villageId == playerVillage->id) isSelfRealm = true;

    if (isSelfRealm) {
        options.push_back({
            "Muster Warband",
            "Assemble defense levies at hearth",
            "Free",
            true,
            sf::Color(245, 195, 65),
            [this, &reg, playerKingdom, targetVillage, playerApeId]() {
                if (playerKingdom && targetVillage) {
                    sim::WarfareManager::issueMusterOrder(reg, playerKingdom->id, targetVillage->id, playerApeId);
                }
                statusMessage = "Warband Called to Arms!";
                statusColor = sf::Color(100, 240, 100);
                statusTimer = 2.5f;
            }
        });

        options.push_back({
            "Distribute Rations",
            "Feed clan elders to raise loyalty",
            "-35 Food",
            (playerVillage && playerVillage->food >= 35),
            sf::Color(115, 215, 115),
            [this, playerVillage]() {
                if (playerVillage && playerVillage->food >= 35) {
                    playerVillage->food -= 35;
                    statusMessage = "Clan Loyalty Reinforced";
                    statusColor = sf::Color(115, 225, 115);
                    statusTimer = 2.5f;
                }
            }
        });
        return;
    }

    bool alreadyAtWar = false;
    if (playerKingdom && targetKingdom && playerKingdom->relations.count(targetKingdom->id)) {
        alreadyAtWar = (playerKingdom->relations[targetKingdom->id] == sim::DiplomacyStatus::War);
    }

    int curAmber = player ? player->amberCount : 0;

    if (isKingdomLevel) {
        options.push_back({
            "Declare War",
            alreadyAtWar ? "Realm is already at war" : "Full territorial conquest",
            "-15 Tension",
            !alreadyAtWar && (playerKingdom != nullptr || player != nullptr),
            sf::Color(235, 60, 60),
            [this, &reg, playerKingdom, targetKingdom]() {
                if (playerKingdom && targetKingdom) {
                    sim::WarfareManager::declareWar(reg, playerKingdom->id, targetKingdom->id, "Royal territorial claim.");
                }
                statusMessage = "Realm War Declared!";
                statusColor = sf::Color(245, 60, 60);
                statusTimer = 3.0f;
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
        options.push_back({
            "Launch Border Raid",
            "Pillage storage pits without war",
            "+25 Tension",
            true,
            sf::Color(235, 140, 45),
            [this, targetVillage, playerVillage, targetKingdom]() {
                if (targetVillage && playerVillage) {
                    int lootFood = std::min(targetVillage->food, 35);
                    int lootWood = std::min(targetVillage->wood, 25);
                    targetVillage->food -= lootFood;
                    targetVillage->wood -= lootWood;
                    playerVillage->food += lootFood;
                    playerVillage->wood += lootWood;
                    if (targetKingdom && targetKingdom->borderTension.count(playerVillage->kingdomId)) {
                        targetKingdom->borderTension[playerVillage->kingdomId] += 25.f;
                    }
                }
                statusMessage = "Settlement Raided!";
                statusColor = sf::Color(245, 150, 50);
                statusTimer = 3.0f;
            }
        });

        options.push_back({
            "Fabricate Claim",
            "Shaman claims boundary right",
            "-30 Amber",
            (curAmber >= 30),
            sf::Color(190, 130, 240),
            [this, player]() {
                if (player && player->amberCount >= 30) {
                    player->amberCount -= 30;
                    statusMessage = "County Casus Belli Created";
                    statusColor = sf::Color(205, 145, 255);
                    statusTimer = 2.5f;
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

    std::string subTitle = isKingdomLevel ? "Realm" : targetSettlement.historicalName;
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

        sf::Text optCost(opt.costText, *font, 9);
        optCost.setStyle(sf::Text::Bold);
        optCost.setFillColor(opt.isEnabled ? opt.accentColor : sf::Color(115, 100, 95));
        sf::FloatRect ocb = optCost.getLocalBounds();
        optCost.setPosition(ob.left + ob.width - ocb.width - 6.f, ob.top + 5.f);
        window.draw(optCost);

        sf::Text optSub(opt.subtitle, *font, 9);
        optSub.setStyle(sf::Text::Italic);
        optSub.setFillColor(opt.isEnabled ? sf::Color(175, 160, 135) : sf::Color(95, 90, 85));
        optSub.setPosition(ob.left + 8.f, ob.top + 22.f);
        window.draw(optSub);
    }
}