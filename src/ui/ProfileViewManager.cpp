#include "ui/ProfileViewManager.h"
#include <algorithm>
#include <sstream>
#include <string>

ProfileViewManager::ProfileViewManager()
    : font(nullptr), isInspectingCharacter(false), inspectedApeId(0),
      selectedVillageId(0), selectedKingdomId(0),
      profilePanelPos(146.f, 118.f), panelWidth(290.f), panelHeight(380.f) {}

void ProfileViewManager::init(const sf::Font& f) {
    font = &f;
}

void ProfileViewManager::inspectCharacter(sim::EntityID id) {
    inspectedApeId = id;
    isInspectingCharacter = true;
    selectedVillageId = 0;
    selectedKingdomId = 0;
    panelHeight = 440.f;
}

void ProfileViewManager::inspectVillage(sim::VillageID id) {
    selectedVillageId = id;
    isInspectingCharacter = false;
    inspectedApeId = 0;
    selectedKingdomId = 0;
    panelHeight = 370.f;
}

void ProfileViewManager::inspectKingdom(sim::KingdomID id) {
    selectedKingdomId = id;
    isInspectingCharacter = false;
    inspectedApeId = 0;
    selectedVillageId = 0;
    panelHeight = 390.f;
}

void ProfileViewManager::close() {
    isInspectingCharacter = false;
    inspectedApeId = 0;
    selectedVillageId = 0;
    selectedKingdomId = 0;
}

void ProfileViewManager::drawCloseButton(sf::RenderWindow& window, float x, float y) {
    sf::RectangleShape btn(sf::Vector2f(20.f, 20.f));
    btn.setPosition(x, y);
    btn.setFillColor(sf::Color(140, 25, 20, 240));
    btn.setOutlineColor(sf::Color(235, 195, 75));
    btn.setOutlineThickness(1.2f);
    window.draw(btn);

    if (font) {
        sf::Text xTxt("x", *font, 13);
        xTxt.setStyle(sf::Text::Bold);
        xTxt.setFillColor(sf::Color(255, 240, 200));
        sf::FloatRect xb = xTxt.getLocalBounds();
        xTxt.setOrigin(xb.left + xb.width * 0.5f, xb.top + xb.height * 0.5f);
        xTxt.setPosition(x + 10.f, y + 9.f);
        window.draw(xTxt);
    }
}

void ProfileViewManager::drawWrappedText(sf::RenderWindow& window, const std::string& text, float x, float& y, float maxW, unsigned int size, sf::Color col, bool bold) {
    if (!font || text.empty()) return;

    std::istringstream words(text);
    std::string word;
    std::string currentLine;
    float lineHeight = static_cast<float>(size) + 4.f;

    sf::Text measure("", *font, size);
    if (bold) measure.setStyle(sf::Text::Bold);

    while (words >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        measure.setString(testLine);
        if (measure.getLocalBounds().width > maxW && !currentLine.empty()) {
            measure.setString(currentLine);
            measure.setFillColor(col);
            measure.setPosition(x, y);
            window.draw(measure);
            y += lineHeight;
            currentLine = word;
        } else {
            currentLine = testLine;
        }
    }

    if (!currentLine.empty()) {
        measure.setString(currentLine);
        measure.setFillColor(col);
        measure.setPosition(x, y);
        window.draw(measure);
        y += lineHeight;
    }
}

bool ProfileViewManager::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    if (!isInspecting()) return false;

    sf::FloatRect profileRect(profilePanelPos.x, profilePanelPos.y, panelWidth, panelHeight);
    sf::FloatRect closeBtnRect(profilePanelPos.x + panelWidth - 26.f, profilePanelPos.y + 6.f, 20.f, 20.f);

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        close();
        return true;
    }

    if (event.type == sf::Event::KeyPressed && isInspectingCharacter && inspectedApeId != 0) {
        sim::ApeData* targetApe = reg.getApe(inspectedApeId);
        sim::ApeData* player = reg.getApe(controlledApeId);

        if (targetApe && player) {
            sim::VillageData* village = reg.getVillage(player->villageId);
            if (village && targetApe->id != village->leaderId && targetApe->villageId == village->id) {
                auto appoint = [&](sim::EntityID& roleSlot, sim::CouncilRole role) {
                    if (roleSlot != 0 && roleSlot != targetApe->id) {
                        sim::ApeData* formerApe = reg.getApe(roleSlot);
                        if (formerApe) {
                            formerApe->councilRole = sim::CouncilRole::None;
                            formerApe->opinions[player->id] = std::clamp(formerApe->opinions[player->id] - 40, -100, 100);
                        }
                    }
                    if (roleSlot != targetApe->id) {
                        targetApe->opinions[player->id] = std::clamp(targetApe->opinions[player->id] + 30, -100, 100);
                    }
                    roleSlot = targetApe->id;
                    targetApe->councilRole = role;
                };

                auto revoke = [&](sim::EntityID& roleSlot) {
                    if (roleSlot == targetApe->id) {
                        roleSlot = 0;
                        targetApe->councilRole = sim::CouncilRole::None;
                        targetApe->opinions[player->id] = std::clamp(targetApe->opinions[player->id] - 30, -100, 100);
                    }
                };

                if (event.key.code == sf::Keyboard::Num1) { appoint(village->warChiefId, sim::CouncilRole::WarChief); return true; }
                if (event.key.code == sf::Keyboard::Num2) { appoint(village->chiefBuilderId, sim::CouncilRole::ChiefBuilder); return true; }
                if (event.key.code == sf::Keyboard::Num3) { appoint(village->leadForagerId, sim::CouncilRole::LeadForager); return true; }
                if (event.key.code == sf::Keyboard::Num4) { appoint(village->shamanId, sim::CouncilRole::Shaman); return true; }
                if (event.key.code == sf::Keyboard::Num0) {
                    revoke(village->warChiefId);
                    revoke(village->chiefBuilderId);
                    revoke(village->leadForagerId);
                    revoke(village->shamanId);
                    return true;
                }
            }
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);

        if (closeBtnRect.contains(vMouse)) {
            close();
            return true;
        }

        if (profileRect.contains(vMouse)) {
            return true;
        }

        close();
        return false;
    }

    if (event.type == sf::Event::MouseWheelScrolled) {
        sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseWheelScroll.x, event.mouseWheelScroll.y), letterboxView);
        if (profileRect.contains(vMouse)) {
            return true;
        }
    }

    return false;
}

void ProfileViewManager::draw(sf::RenderWindow& window, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    if (!font) return;
    if (isInspectingCharacter && inspectedApeId != 0) {
        drawCharacterProfile(window, inspectedApeId, reg, controlledApeId);
    } else if (selectedVillageId != 0) {
        drawVillageProfile(window, selectedVillageId, reg);
    } else if (selectedKingdomId != 0) {
        drawKingdomProfile(window, selectedKingdomId, reg, controlledApeId);
    }
}

void ProfileViewManager::drawCharacterProfile(sf::RenderWindow& window, sim::EntityID apeId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::ApeData* ape = reg.getApe(apeId);
    if (!ape) return;

    sim::ApeData* player = reg.getApe(controlledApeId);
    sim::VillageData* playerVillage = player ? reg.getVillage(player->villageId) : nullptr;
    sim::VillageData* apeVillage = reg.getVillage(ape->villageId);
    sim::KingdomData* apeKingdom = (ape->currentKingdom != 0) ? reg.getKingdom(ape->currentKingdom) : nullptr;

    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape shadow(sf::Vector2f(panelWidth + 4.f, panelHeight + 4.f));
    shadow.setPosition(startX + 2.f, startY + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(24, 17, 13, 250));
    panel.setOutlineColor(sf::Color(185, 140, 65));
    panel.setOutlineThickness(1.5f);
    window.draw(panel);

    sf::RectangleShape headerPlate(sf::Vector2f(panelWidth - 6.f, 32.f));
    headerPlate.setPosition(startX + 3.f, startY + 3.f);
    headerPlate.setFillColor(sf::Color(48, 32, 20));
    window.draw(headerPlate);

    float titleY = startY + 8.f;
    drawWrappedText(window, ape->name, startX + 12.f, titleY, panelWidth - 45.f, 13, sf::Color(255, 225, 130), true);

    drawCloseButton(window, startX + panelWidth - 26.f, startY + 6.f);

    std::string realmName = apeKingdom ? ("Kingdom of " + apeKingdom->name) : (apeVillage ? ("Clan of " + apeVillage->name) : "Wanderer");
    sf::Text realmText(realmName, *font, 11);
    realmText.setFillColor(sf::Color(185, 170, 145));
    realmText.setPosition(startX + 12.f, startY + 42.f);
    window.draw(realmText);

    std::string currentRoleStr = "None";
    if (apeVillage) {
        if (apeVillage->warChiefId == ape->id) currentRoleStr = "War Chief";
        else if (apeVillage->chiefBuilderId == ape->id) currentRoleStr = "Chief Builder";
        else if (apeVillage->leadForagerId == ape->id) currentRoleStr = "Lead Forager";
        else if (apeVillage->shamanId == ape->id) currentRoleStr = "Shaman";
        else if (apeVillage->leaderId == ape->id) currentRoleStr = "Clan Alpha";
    }

    sf::Text roleText("Office: " + currentRoleStr + "  |  Age: " + std::to_string(static_cast<int>(ape->age)), *font, 10);
    roleText.setFillColor(sf::Color(225, 185, 85));
    roleText.setPosition(startX + 12.f, startY + 58.f);
    window.draw(roleText);

    float statBoxW = 58.f;
    float statBoxH = 36.f;
    float statSpacing = 8.f;
    float statStartX = startX + 12.f;
    float statY = startY + 80.f;

    struct StatBadge { std::string label; int val; sf::Color col; };
    std::vector<StatBadge> stats = {
        {"MAR", static_cast<int>(ape->skills.combat * 10.f), sf::Color(190, 60, 60)},
        {"DIP", static_cast<int>(ape->skills.leadership * 10.f), sf::Color(210, 170, 50)},
        {"ADM", static_cast<int>(ape->skills.building * 10.f), sf::Color(70, 150, 70)},
        {"FOR", static_cast<int>(ape->skills.gathering * 10.f), sf::Color(60, 150, 190)}
    };

    for (size_t i = 0; i < stats.size(); ++i) {
        float bx = statStartX + i * (statBoxW + statSpacing);
        sf::RectangleShape box(sf::Vector2f(statBoxW, statBoxH));
        box.setPosition(bx, statY);
        box.setFillColor(sf::Color(16, 11, 8, 230));
        box.setOutlineColor(stats[i].col);
        box.setOutlineThickness(1.f);
        window.draw(box);

        sf::Text sLbl(stats[i].label, *font, 9);
        sLbl.setFillColor(sf::Color(170, 170, 170));
        sLbl.setPosition(bx + 4.f, statY + 2.f);
        window.draw(sLbl);

        sf::Text sVal(std::to_string(stats[i].val), *font, 13);
        sVal.setFillColor(sf::Color::White);
        sVal.setStyle(sf::Text::Bold);
        sVal.setPosition(bx + 4.f, statY + 14.f);
        window.draw(sVal);
    }

    float traitsY = startY + 126.f;
    sf::Text traitsHeader("TRAITS", *font, 10);
    traitsHeader.setFillColor(sf::Color(185, 145, 65));
    traitsHeader.setPosition(startX + 12.f, traitsY);
    window.draw(traitsHeader);

    float tBadgeX = startX + 12.f;
    float tBadgeY = traitsY + 16.f;

    auto traitToStr = [](sim::Trait t) -> std::string {
        switch (t) {
            case sim::Trait::Brave: return "Brave";
            case sim::Trait::Coward: return "Coward";
            case sim::Trait::Greedy: return "Greedy";
            case sim::Trait::Honorable: return "Honorable";
            case sim::Trait::Cruel: return "Cruel";
            case sim::Trait::Charismatic: return "Charismatic";
            case sim::Trait::Lazy: return "Lazy";
            case sim::Trait::Strategic: return "Strategic";
            case sim::Trait::Impulsive: return "Impulsive";
            case sim::Trait::Curious: return "Curious";
            case sim::Trait::Energetic: return "Energetic";
            case sim::Trait::Clever: return "Clever";
            case sim::Trait::Hardworking: return "Hardworking";
            case sim::Trait::Patient: return "Patient";
            case sim::Trait::Aggressive: return "Aggressive";
            case sim::Trait::Perceptive: return "Perceptive";
            default: return "Trait";
        }
    };

    for (auto t : ape->traits) {
        std::string tName = traitToStr(t);
        sf::Text tTxt(tName, *font, 10);
        sf::FloatRect tb = tTxt.getLocalBounds();

        sf::RectangleShape badge(sf::Vector2f(tb.width + 10.f, 18.f));
        badge.setPosition(tBadgeX, tBadgeY);
        badge.setFillColor(sf::Color(38, 28, 20));
        badge.setOutlineColor(sf::Color(130, 95, 45));
        badge.setOutlineThickness(1.f);
        window.draw(badge);

        tTxt.setFillColor(sf::Color(235, 215, 165));
        tTxt.setPosition(tBadgeX + 5.f, tBadgeY + 1.f);
        window.draw(tTxt);

        tBadgeX += tb.width + 16.f;
        if (tBadgeX > startX + panelWidth - 70.f) {
            tBadgeX = startX + 12.f;
            tBadgeY += 22.f;
        }
    }

    float opY = tBadgeY + 28.f;
    int personalOp = 0;
    if (player) {
        if (ape->opinions.count(player->id)) personalOp = ape->opinions[player->id];
        else if (ape->villageId == player->villageId) {
            personalOp = 20;
            ape->opinions[player->id] = personalOp;
        }
    }

    std::string relText = "Opinion: " + (personalOp >= 0 ? ("+" + std::to_string(personalOp)) : std::to_string(personalOp));
    sf::Color relCol = sf::Color(200, 200, 200);

    if (personalOp >= 30) { relCol = sf::Color(100, 235, 100); relText += " (Loyal)"; }
    else if (personalOp > 0) { relCol = sf::Color(180, 225, 140); relText += " (Favorable)"; }
    else if (personalOp == 0) { relCol = sf::Color(210, 210, 210); relText += " (Neutral)"; }
    else if (personalOp <= -30) { relCol = sf::Color(240, 70, 70); relText += " (Hostile)"; }
    else { relCol = sf::Color(235, 150, 90); relText += " (Discontent)"; }

    sf::Text opTxt(relText, *font, 11);
    opTxt.setFillColor(relCol);
    opTxt.setPosition(startX + 12.f, opY);
    window.draw(opTxt);

    if (playerVillage && ape->villageId == playerVillage->id && ape->id != playerVillage->leaderId) {
        float assignBoxY = opY + 24.f;
        sf::RectangleShape assignBox(sf::Vector2f(panelWidth - 24.f, 96.f));
        assignBox.setPosition(startX + 12.f, assignBoxY);
        assignBox.setFillColor(sf::Color(32, 22, 16, 230));
        assignBox.setOutlineColor(sf::Color(130, 95, 45));
        assignBox.setOutlineThickness(1.f);
        window.draw(assignBox);

        sf::Text assignTitle("COUNCIL APPOINTMENT", *font, 10);
        assignTitle.setFillColor(sf::Color(255, 215, 95));
        assignTitle.setStyle(sf::Text::Bold);
        assignTitle.setPosition(startX + 20.f, assignBoxY + 6.f);
        window.draw(assignTitle);

        std::string roleKeys = "[1] War Chief   [2] Builder\n[3] Forager     [4] Shaman\n[0] Revoke Role";
        sf::Text roleKeysTxt(roleKeys, *font, 10);
        roleKeysTxt.setFillColor(sf::Color(210, 205, 190));
        roleKeysTxt.setPosition(startX + 20.f, assignBoxY + 24.f);
        window.draw(roleKeysTxt);
    }
}

void ProfileViewManager::drawVillageProfile(sf::RenderWindow& window, sim::VillageID vId, sim::SimulationRegistry& reg) {
    sim::VillageData* v = reg.getVillage(vId);
    if (!v) return;

    std::string leaderName = "Unknown Leader";
    sim::ApeData* leader = reg.getApe(v->leaderId);
    if (leader) leaderName = leader->name;

    std::string allegiance = "Independent Tribe";
    if (v->kingdomId != 0) {
        sim::KingdomData* k = reg.getKingdom(v->kingdomId);
        if (k) allegiance = "Kingdom of " + k->name;
    }

    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape shadow(sf::Vector2f(panelWidth + 4.f, panelHeight + 4.f));
    shadow.setPosition(startX + 2.f, startY + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(24, 17, 13, 250));
    panel.setOutlineColor(sf::Color(185, 140, 65));
    panel.setOutlineThickness(1.5f);
    window.draw(panel);

    sf::RectangleShape hPlate(sf::Vector2f(panelWidth - 6.f, 32.f));
    hPlate.setPosition(startX + 3.f, startY + 3.f);
    hPlate.setFillColor(sf::Color(48, 32, 20));
    window.draw(hPlate);

    float titleY = startY + 8.f;
    drawWrappedText(window, v->name, startX + 12.f, titleY, panelWidth - 45.f, 13, sf::Color(255, 225, 130), true);

    drawCloseButton(window, startX + panelWidth - 26.f, startY + 6.f);

    float curY = startY + 44.f;

    auto drawField = [&](const std::string& label, const std::string& val, sf::Color valCol = sf::Color(240, 230, 210)) {
        sf::Text l(label, *font, 10);
        l.setFillColor(sf::Color(165, 145, 120));
        l.setPosition(startX + 14.f, curY);
        window.draw(l);
        curY += 13.f;

        drawWrappedText(window, val, startX + 14.f, curY, panelWidth - 28.f, 11, valCol, true);
        curY += 6.f;
    };

    drawField("Clan Alpha / Leader", leaderName);
    drawField("Allegiance", allegiance, sf::Color(100, 180, 240));
    drawField("Population", std::to_string(v->members.size()) + " apes");

    sf::RectangleShape resBox(sf::Vector2f(panelWidth - 28.f, 48.f));
    resBox.setPosition(startX + 14.f, curY);
    resBox.setFillColor(sf::Color(16, 11, 8, 230));
    resBox.setOutlineColor(sf::Color(120, 85, 45));
    resBox.setOutlineThickness(1.f);
    window.draw(resBox);

    sf::Text resH("STOCKPILES", *font, 9);
    resH.setFillColor(sf::Color(180, 145, 75));
    resH.setStyle(sf::Text::Bold);
    resH.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 5.f);
    window.draw(resH);

    std::string resContent = "Food: " + std::to_string(v->food) + "  Wood: " + std::to_string(v->wood) + "  Stone: " + std::to_string(v->stone);
    sf::Text resVals(resContent, *font, 11);
    resVals.setFillColor(sf::Color(220, 215, 195));
    resVals.setStyle(sf::Text::Bold);
    resVals.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 22.f);
    window.draw(resVals);

    curY += 56.f;
    drawField("Settlement Tier", "Tier " + std::to_string(static_cast<int>(v->tier)), sf::Color(235, 195, 55));
    drawField("Structures Built", std::to_string(v->finishedStructures.size()) + " structures");
}

void ProfileViewManager::drawKingdomProfile(sf::RenderWindow& window, sim::KingdomID kId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::KingdomData* k = reg.getKingdom(kId);
    if (!k) return;

    std::string rulerName = "Unknown Ruler";
    sim::ApeData* ruler = reg.getApe(k->currentKingId);
    if (ruler) rulerName = ruler->name;

    std::string dynastyName = "Unknown";
    sim::DynastyData* dyn = reg.getDynasty(k->leaderDynastyId);
    if (dyn && !dyn->name.empty()) {
        dynastyName = dyn->name;
        if (dynastyName.find("Dynasty") == std::string::npos) {
            dynastyName += " Dynasty";
        }
    }

    std::string capitalName = "Unknown Capital";
    sim::VillageData* cap = reg.getVillage(k->capitalVillageId);
    if (cap) capitalName = cap->name;

    std::string relStr = "Neutral";
    sf::Color relCol = sf::Color(200, 200, 200);
    sim::ApeData* pApe = reg.getApe(controlledApeId);
    if (pApe && pApe->currentKingdom != 0 && pApe->currentKingdom != kId) {
        sim::KingdomData* pK = reg.getKingdom(pApe->currentKingdom);
        if (pK && pK->relations.count(kId)) {
            switch (pK->relations[kId]) {
                case sim::DiplomacyStatus::War: relStr = "At War"; relCol = sf::Color(245, 60, 60); break;
                case sim::DiplomacyStatus::Rival: relStr = "Rival"; relCol = sf::Color(245, 140, 50); break;
                case sim::DiplomacyStatus::Alliance: relStr = "Alliance"; relCol = sf::Color(80, 180, 255); break;
                case sim::DiplomacyStatus::Trade: relStr = "Trade Partner"; relCol = sf::Color(120, 235, 120); break;
                case sim::DiplomacyStatus::Friendly: relStr = "Friendly"; relCol = sf::Color(140, 240, 140); break;
                default: break;
            }
        }
    } else if (pApe && pApe->currentKingdom == kId) {
        relStr = "Your Realm";
        relCol = sf::Color(255, 215, 60);
    }

    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape shadow(sf::Vector2f(panelWidth + 4.f, panelHeight + 4.f));
    shadow.setPosition(startX + 2.f, startY + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(24, 17, 13, 250));
    panel.setOutlineColor(sf::Color(185, 140, 65));
    panel.setOutlineThickness(1.5f);
    window.draw(panel);

    sf::RectangleShape hPlate(sf::Vector2f(panelWidth - 6.f, 32.f));
    hPlate.setPosition(startX + 3.f, startY + 3.f);
    hPlate.setFillColor(sf::Color(48, 32, 20));
    window.draw(hPlate);

    float titleY = startY + 8.f;
    drawWrappedText(window, "KINGDOM OF " + k->name, startX + 12.f, titleY, panelWidth - 45.f, 13, k->color, true);

    drawCloseButton(window, startX + panelWidth - 26.f, startY + 6.f);

    float curY = startY + 42.f;

    auto drawField = [&](const std::string& label, const std::string& val, sf::Color valCol = sf::Color(240, 230, 210), bool bold = false) {
        sf::Text l(label, *font, 10);
        l.setFillColor(sf::Color(165, 145, 120));
        l.setPosition(startX + 14.f, curY);
        window.draw(l);
        curY += 13.f;

        drawWrappedText(window, val, startX + 14.f, curY, panelWidth - 28.f, 11, valCol, bold);
        curY += 5.f;
    };

    drawField("Ruler", rulerName + " (" + dynastyName + ")", sf::Color(255, 235, 160), true);
    drawField("Capital", capitalName);
    drawField("Scale", std::to_string(k->population) + " apes | " + std::to_string(k->controlledVillages.size()) + " settlements");

    sf::RectangleShape resBox(sf::Vector2f(panelWidth - 28.f, 48.f));
    resBox.setPosition(startX + 14.f, curY);
    resBox.setFillColor(sf::Color(16, 11, 8, 230));
    resBox.setOutlineColor(sf::Color(120, 85, 45));
    resBox.setOutlineThickness(1.f);
    window.draw(resBox);

    sf::Text resH("ROYAL TREASURY", *font, 9);
    resH.setFillColor(sf::Color(180, 145, 75));
    resH.setStyle(sf::Text::Bold);
    resH.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 5.f);
    window.draw(resH);

    std::string resContent = "Food: " + std::to_string(k->treasuryFood) + "  Wood: " + std::to_string(k->treasuryWood) + "  Stone: " + std::to_string(k->treasuryStone);
    sf::Text resVals(resContent, *font, 11);
    resVals.setFillColor(sf::Color(220, 215, 195));
    resVals.setStyle(sf::Text::Bold);
    resVals.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 22.f);
    window.draw(resVals);

    curY += 56.f;
    drawField("Military Power", std::to_string(k->militaryStrength) + " strength", sf::Color(245, 130, 130), true);
    drawField("Diplomatic Status", relStr, relCol, true);
}