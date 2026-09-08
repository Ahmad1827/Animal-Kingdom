#include "ui/ProfileViewManager.h"
#include <algorithm>

ProfileViewManager::ProfileViewManager()
    : font(nullptr), isInspectingCharacter(false), inspectedApeId(0),
      selectedVillageId(0), selectedKingdomId(0), isDraggingProfile(false),
      profilePanelPos(40.f, 85.f) {}

void ProfileViewManager::init(const sf::Font& f) {
    font = &f;
}

void ProfileViewManager::inspectCharacter(sim::EntityID id) {
    inspectedApeId = id;
    isInspectingCharacter = true;
    selectedVillageId = 0;
    selectedKingdomId = 0;
}

void ProfileViewManager::inspectVillage(sim::VillageID id) {
    selectedVillageId = id;
    isInspectingCharacter = false;
    inspectedApeId = 0;
    selectedKingdomId = 0;
}

void ProfileViewManager::inspectKingdom(sim::KingdomID id) {
    selectedKingdomId = id;
    isInspectingCharacter = false;
    inspectedApeId = 0;
    selectedVillageId = 0;
}

void ProfileViewManager::close() {
    isInspectingCharacter = false;
    inspectedApeId = 0;
    selectedVillageId = 0;
    selectedKingdomId = 0;
    isDraggingProfile = false;
}

bool ProfileViewManager::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    if (!isInspecting()) return false;

    sf::FloatRect profileRect(profilePanelPos.x, profilePanelPos.y, 390.f, 580.f);
    sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            close();
            return true;
        }

        if (isInspectingCharacter && inspectedApeId != 0) {
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
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (profileRect.contains(vMouse.x, vMouse.y)) {
            isDraggingProfile = true;
            lastMousePos = sf::Vector2i(static_cast<int>(vMouse.x), static_cast<int>(vMouse.y));
            return true;
        }
    }

    if (event.type == sf::Event::MouseMoved && isDraggingProfile) {
        sf::Vector2f vMove = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), letterboxView);
        sf::Vector2i newPos(static_cast<int>(vMove.x), static_cast<int>(vMove.y));
        profilePanelPos.x += (newPos.x - lastMousePos.x);
        profilePanelPos.y += (newPos.y - lastMousePos.y);
        lastMousePos = newPos;
        return true;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (isDraggingProfile) {
            isDraggingProfile = false;
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

    float panelW = 390.f;
    float panelH = 580.f;
    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape shadow(sf::Vector2f(panelW + 8.f, panelH + 8.f));
    shadow.setPosition(startX + 4.f, startY + 4.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(shadow);

    sf::RectangleShape outerFrame(sf::Vector2f(panelW, panelH));
    outerFrame.setPosition(startX, startY);
    outerFrame.setFillColor(sf::Color(22, 16, 12, 250));
    outerFrame.setOutlineColor(sf::Color(190, 145, 60));
    outerFrame.setOutlineThickness(3.f);
    window.draw(outerFrame);

    sf::RectangleShape innerBorder(sf::Vector2f(panelW - 14.f, panelH - 14.f));
    innerBorder.setPosition(startX + 7.f, startY + 7.f);
    innerBorder.setFillColor(sf::Color(32, 24, 18, 240));
    innerBorder.setOutlineColor(sf::Color(100, 75, 35));
    innerBorder.setOutlineThickness(1.f);
    window.draw(innerBorder);

    auto drawCornerBracket = [&](float cx, float cy) {
        sf::RectangleShape h(sf::Vector2f(16.f, 3.f));
        h.setOrigin(8.f, 1.5f);
        h.setPosition(cx, cy);
        h.setFillColor(sf::Color(240, 200, 90));
        window.draw(h);
        sf::RectangleShape v(sf::Vector2f(3.f, 16.f));
        v.setOrigin(1.5f, 8.f);
        v.setPosition(cx, cy);
        v.setFillColor(sf::Color(240, 200, 90));
        window.draw(v);
    };
    drawCornerBracket(startX + 8.f, startY + 8.f);
    drawCornerBracket(startX + panelW - 8.f, startY + 8.f);
    drawCornerBracket(startX + 8.f, startY + panelH - 8.f);
    drawCornerBracket(startX + panelW - 8.f, startY + panelH - 8.f);

    sf::RectangleShape headerPlate(sf::Vector2f(panelW - 20.f, 52.f));
    headerPlate.setPosition(startX + 10.f, startY + 12.f);
    headerPlate.setFillColor(sf::Color(55, 38, 24));
    headerPlate.setOutlineColor(sf::Color(160, 120, 50));
    headerPlate.setOutlineThickness(1.f);
    window.draw(headerPlate);

    sf::Text nameText(ape->name, *font, 20);
    nameText.setFillColor(sf::Color(255, 225, 120));
    nameText.setStyle(sf::Text::Bold);
    sf::FloatRect nb = nameText.getLocalBounds();
    nameText.setOrigin(nb.left + nb.width / 2.f, nb.top + nb.height / 2.f);
    nameText.setPosition(startX + panelW / 2.f, startY + 38.f);
    window.draw(nameText);

    std::string realmName = apeKingdom ? ("Kingdom of " + apeKingdom->name) : (apeVillage ? ("Clan of " + apeVillage->name) : "Wanderer");
    sf::Text realmText(realmName, *font, 14);
    realmText.setFillColor(sf::Color(190, 175, 150));
    sf::FloatRect rb = realmText.getLocalBounds();
    realmText.setOrigin(rb.left + rb.width / 2.f, 0.f);
    realmText.setPosition(startX + panelW / 2.f, startY + 70.f);
    window.draw(realmText);

    std::string currentRoleStr = "None";
    if (apeVillage) {
        if (apeVillage->warChiefId == ape->id) currentRoleStr = "War Chief";
        else if (apeVillage->chiefBuilderId == ape->id) currentRoleStr = "Chief Builder";
        else if (apeVillage->leadForagerId == ape->id) currentRoleStr = "Lead Forager";
        else if (apeVillage->shamanId == ape->id) currentRoleStr = "Shaman";
        else if (apeVillage->leaderId == ape->id) currentRoleStr = "Clan Patriarch";
    }

    sf::Text roleText("Office: " + currentRoleStr + "   |   Age: " + std::to_string(static_cast<int>(ape->age)), *font, 13);
    roleText.setFillColor(sf::Color(230, 190, 90));
    sf::FloatRect rtb = roleText.getLocalBounds();
    roleText.setOrigin(rtb.left + rtb.width / 2.f, 0.f);
    roleText.setPosition(startX + panelW / 2.f, startY + 92.f);
    window.draw(roleText);

    float statBoxW = 80.f;
    float statBoxH = 50.f;
    float statSpacing = 10.f;
    float statStartX = startX + 20.f;
    float statY = startY + 120.f;

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
        box.setFillColor(sf::Color(20, 15, 12, 230));
        box.setOutlineColor(stats[i].col);
        box.setOutlineThickness(1.5f);
        window.draw(box);

        sf::Text sLbl(stats[i].label, *font, 11);
        sLbl.setFillColor(sf::Color(180, 180, 180));
        sf::FloatRect lb = sLbl.getLocalBounds();
        sLbl.setOrigin(lb.left + lb.width / 2.f, 0.f);
        sLbl.setPosition(bx + statBoxW / 2.f, statY + 5.f);
        window.draw(sLbl);

        sf::Text sVal(std::to_string(stats[i].val), *font, 17);
        sVal.setFillColor(sf::Color::White);
        sVal.setStyle(sf::Text::Bold);
        sf::FloatRect vb = sVal.getLocalBounds();
        sVal.setOrigin(vb.left + vb.width / 2.f, 0.f);
        sVal.setPosition(bx + statBoxW / 2.f, statY + 22.f);
        window.draw(sVal);
    }

    float traitsY = startY + 185.f;
    sf::Text traitsHeader("CHARACTER TRAITS", *font, 12);
    traitsHeader.setFillColor(sf::Color(190, 145, 60));
    traitsHeader.setPosition(startX + 20.f, traitsY);
    window.draw(traitsHeader);

    float tBadgeX = startX + 20.f;
    float tBadgeY = traitsY + 22.f;

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
        sf::Text tTxt(tName, *font, 12);
        sf::FloatRect tb = tTxt.getLocalBounds();

        sf::RectangleShape badge(sf::Vector2f(tb.width + 16.f, 22.f));
        badge.setPosition(tBadgeX, tBadgeY);
        badge.setFillColor(sf::Color(45, 35, 25));
        badge.setOutlineColor(sf::Color(140, 105, 50));
        badge.setOutlineThickness(1.f);
        window.draw(badge);

        tTxt.setFillColor(sf::Color(240, 220, 170));
        tTxt.setPosition(tBadgeX + 8.f, tBadgeY + 2.f);
        window.draw(tTxt);

        tBadgeX += tb.width + 24.f;
        if (tBadgeX > startX + panelW - 90.f) {
            tBadgeX = startX + 20.f;
            tBadgeY += 26.f;
        }
    }

    float opY = tBadgeY + 36.f;
    sf::RectangleShape opFrame(sf::Vector2f(panelW - 40.f, 40.f));
    opFrame.setPosition(startX + 20.f, opY);
    opFrame.setFillColor(sf::Color(25, 18, 14));
    opFrame.setOutlineColor(sf::Color(90, 65, 35));
    opFrame.setOutlineThickness(1.f);
    window.draw(opFrame);

    int personalOp = 0;
    if (player) {
        if (ape->opinions.count(player->id)) personalOp = ape->opinions[player->id];
        else if (ape->villageId == player->villageId) {
            personalOp = 20;
            ape->opinions[player->id] = personalOp;
        }
    }

    std::string relText = "Opinion of You: " + (personalOp >= 0 ? ("+" + std::to_string(personalOp)) : std::to_string(personalOp));
    sf::Color relCol = sf::Color(200, 200, 200);

    if (personalOp >= 30) { relCol = sf::Color(100, 235, 100); relText += " (Loyal)"; }
    else if (personalOp > 0) { relCol = sf::Color(180, 225, 140); relText += " (Favorable)"; }
    else if (personalOp == 0) { relCol = sf::Color(210, 210, 210); relText += " (Neutral)"; }
    else if (personalOp <= -30) { relCol = sf::Color(240, 70, 70); relText += " (Hostile)"; }
    else { relCol = sf::Color(235, 150, 90); relText += " (Discontent)"; }

    sf::Text opTxt(relText, *font, 13);
    opTxt.setFillColor(relCol);
    sf::FloatRect ob = opTxt.getLocalBounds();
    opTxt.setOrigin(ob.left + ob.width / 2.f, ob.top + ob.height / 2.f);
    opTxt.setPosition(startX + panelW / 2.f, opY + 20.f);
    window.draw(opTxt);

    if (playerVillage && ape->villageId == playerVillage->id && ape->id != playerVillage->leaderId) {
        float assignBoxY = opY + 50.f;
        sf::RectangleShape assignBox(sf::Vector2f(panelW - 40.f, 150.f));
        assignBox.setPosition(startX + 20.f, assignBoxY);
        assignBox.setFillColor(sf::Color(40, 28, 20, 230));
        assignBox.setOutlineColor(sf::Color(160, 120, 50));
        assignBox.setOutlineThickness(1.f);
        window.draw(assignBox);

        sf::Text assignTitle("COUNCIL APPOINTMENT", *font, 12);
        assignTitle.setFillColor(sf::Color(255, 215, 100));
        assignTitle.setStyle(sf::Text::Bold);
        assignTitle.setPosition(startX + 30.f, assignBoxY + 8.f);
        window.draw(assignTitle);

        std::string roleKeys = "[1] Appoint War Chief\n[2] Appoint Chief Builder\n[3] Appoint Lead Forager\n[4] Appoint Shaman\n[0] Revoke Role";
        sf::Text roleKeysTxt(roleKeys, *font, 13);
        roleKeysTxt.setFillColor(sf::Color(220, 220, 200));
        roleKeysTxt.setPosition(startX + 30.f, assignBoxY + 30.f);
        window.draw(roleKeysTxt);
    }

    sf::Text closePrompt("[ESC] Dismiss Character View", *font, 12);
    closePrompt.setFillColor(sf::Color(160, 140, 110));
    sf::FloatRect cb = closePrompt.getLocalBounds();
    closePrompt.setOrigin(cb.left + cb.width / 2.f, 0.f);
    closePrompt.setPosition(startX + panelW / 2.f, startY + panelH - 24.f);
    window.draw(closePrompt);
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

    float panelW = 380.f;
    float panelH = 560.f;
    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape woodBorder(sf::Vector2f(panelW + 12.f, panelH + 12.f));
    woodBorder.setPosition(startX - 6.f, startY - 6.f);
    woodBorder.setFillColor(sf::Color(45, 30, 20));
    woodBorder.setOutlineColor(sf::Color(15, 10, 5));
    woodBorder.setOutlineThickness(2.f);
    window.draw(woodBorder);

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(220, 205, 172));
    panel.setOutlineColor(sf::Color(165, 125, 60));
    panel.setOutlineThickness(2.f);
    window.draw(panel);

    sf::RectangleShape hPlate(sf::Vector2f(panelW - 16.f, 42.f));
    hPlate.setPosition(startX + 8.f, startY + 8.f);
    hPlate.setFillColor(sf::Color(65, 45, 28));
    window.draw(hPlate);

    sf::Text title("SETTLEMENT: " + v->name, *font, 18);
    title.setFillColor(sf::Color(245, 215, 120));
    title.setStyle(sf::Text::Bold);
    title.setPosition(startX + 18.f, startY + 16.f);
    window.draw(title);

    auto drawField = [&](const std::string& label, const std::string& val, float y, sf::Color valCol = sf::Color(30, 15, 5)) {
        sf::Text l(label, *font, 12);
        l.setFillColor(sf::Color(90, 70, 50));
        l.setPosition(startX + 18.f, y);
        window.draw(l);

        sf::Text vTxt(val, *font, 14);
        vTxt.setFillColor(valCol);
        vTxt.setStyle(sf::Text::Bold);
        vTxt.setPosition(startX + 18.f, y + 16.f);
        window.draw(vTxt);
    };

    float curY = startY + 60.f;
    drawField("Clan Chief / Leader", leaderName, curY);
    curY += 42.f;
    drawField("Allegiance & Realm", allegiance, curY, sf::Color(40, 80, 140));
    curY += 42.f;
    drawField("Population", std::to_string(v->members.size()) + " clan members", curY);
    curY += 42.f;

    sf::RectangleShape resBox(sf::Vector2f(panelW - 32.f, 60.f));
    resBox.setPosition(startX + 16.f, curY);
    resBox.setFillColor(sf::Color(200, 182, 145));
    resBox.setOutlineColor(sf::Color(140, 110, 70));
    resBox.setOutlineThickness(1.f);
    window.draw(resBox);

    sf::Text resH("TRIBAL STOCKPILES", *font, 11);
    resH.setFillColor(sf::Color(70, 50, 30));
    resH.setStyle(sf::Text::Bold);
    resH.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 6.f);
    window.draw(resH);

    std::string resContent = "Food: " + std::to_string(v->food) + "   |   Wood: " + std::to_string(v->wood) + "   |   Stone: " + std::to_string(v->stone);
    sf::Text resVals(resContent, *font, 13);
    resVals.setFillColor(sf::Color(30, 70, 35));
    resVals.setStyle(sf::Text::Bold);
    resVals.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 28.f);
    window.draw(resVals);

    curY += 75.f;
    sf::Text structH("SETTLEMENT STRUCTURES (" + std::to_string(v->finishedStructures.size()) + ")", *font, 12);
    structH.setFillColor(sf::Color(80, 55, 30));
    structH.setStyle(sf::Text::Bold);
    structH.setPosition(startX + 18.f, curY);
    window.draw(structH);
    curY += 20.f;

    std::string structSummary = "• Clan Hearth & Chieftain Lodge\n• Communal Sleeping Nests\n• Granary & Armory Racks\n• Watch Platforms & Boundary Totems";
    sf::Text sList(structSummary, *font, 11);
    sList.setFillColor(sf::Color(55, 40, 25));
    sList.setPosition(startX + 18.f, curY);
    window.draw(sList);
}

void ProfileViewManager::drawKingdomProfile(sf::RenderWindow& window, sim::KingdomID kId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::KingdomData* k = reg.getKingdom(kId);
    if (!k) return;

    std::string rulerName = "Unknown Ruler";
    sim::ApeData* ruler = reg.getApe(k->currentKingId);
    if (ruler) rulerName = ruler->name;

    std::string dynastyName = "Unknown Dynasty";
    sim::DynastyData* dyn = reg.getDynasty(k->leaderDynastyId);
    if (dyn) dynastyName = dyn->name;

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
                case sim::DiplomacyStatus::War: relStr = "At War"; relCol = sf::Color(255, 100, 100); break;
                case sim::DiplomacyStatus::Rival: relStr = "Rival"; relCol = sf::Color(255, 150, 100); break;
                case sim::DiplomacyStatus::Alliance: relStr = "Alliance"; relCol = sf::Color(100, 200, 255); break;
                case sim::DiplomacyStatus::Trade: relStr = "Trade Partner"; relCol = sf::Color(150, 255, 150); break;
                case sim::DiplomacyStatus::Friendly: relStr = "Friendly"; relCol = sf::Color(180, 255, 180); break;
                default: break;
            }
        }
    } else if (pApe && pApe->currentKingdom == kId) {
        relStr = "Your Realm";
        relCol = sf::Color(255, 215, 100);
    }

    float panelW = 380.f;
    float panelH = 550.f;
    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(35, 25, 20, 245));
    panel.setOutlineColor(sf::Color(180, 140, 70, 220));
    panel.setOutlineThickness(2.f);
    window.draw(panel);

    auto drawText = [&](const std::string& text, float y, int size, sf::Color col, bool bold = false) {
        sf::Text t(text, *font, size);
        t.setFillColor(col);
        t.setOutlineColor(sf::Color::Black);
        t.setOutlineThickness(bold ? 2.f : 1.f);
        sf::FloatRect bounds = t.getLocalBounds();
        t.setOrigin(bounds.left + bounds.width / 2.f, 0.f);
        t.setPosition(startX + panelW / 2.f, y);
        window.draw(t);
    };

    float curY = startY + 25.f;
    drawText("KINGDOM OF " + k->name, curY, 28, k->color, true);
    curY += 40.f;

    sf::RectangleShape div(sf::Vector2f(panelW - 60.f, 2.f));
    div.setPosition(startX + 30.f, curY);
    div.setFillColor(sf::Color(120, 90, 50, 200));
    window.draw(div);

    curY += 20.f;
    drawText("Ruler", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(rulerName, curY, 22, sf::Color::White);
    curY += 24.f;
    drawText(dynastyName + " Dynasty", curY, 16, sf::Color(150, 150, 150));
    curY += 40.f;
    drawText("Capital", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(capitalName, curY, 18, sf::Color::White);
    curY += 35.f;
    drawText("Scale ( Pop / Villages )", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(std::to_string(k->population) + " apes   |   " + std::to_string(k->controlledVillages.size()) + " settlements", curY, 18, sf::Color::White);
    curY += 35.f;
    drawText("Treasury ( Food / Wood / Stone )", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(std::to_string(k->treasuryFood) + "  /  " + std::to_string(k->treasuryWood) + "  /  " + std::to_string(k->treasuryStone), curY, 18, sf::Color(150, 200, 150));
    curY += 35.f;
    drawText("Military Power", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(std::to_string(k->militaryStrength) + " strength", curY, 18, sf::Color(255, 180, 180));
    curY += 40.f;
    drawText("Relationship: " + relStr, curY, 20, relCol, true);
}