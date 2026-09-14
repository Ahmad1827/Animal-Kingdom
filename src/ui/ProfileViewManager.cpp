#include "ui/ProfileViewManager.h"
#include <algorithm>
#include <sstream>

ProfileViewManager::ProfileViewManager()
    : font(nullptr), currentView(ViewType::None), inspectedApeId(0),
      selectedVillageId(0), selectedKingdomId(0),
      profilePanelPos(146.f, 110.f), panelWidth(320.f), panelHeight(530.f) {}

void ProfileViewManager::init(const sf::Font& f) {
    font = &f;
}

void ProfileViewManager::inspectCharacter(sim::EntityID id, bool recordHistory) {
    if (recordHistory && currentView != ViewType::None) {
        HistoryEntry entry;
        entry.type = currentView;
        if (currentView == ViewType::Character) entry.id = inspectedApeId;
        else if (currentView == ViewType::Village) entry.id = selectedVillageId;
        else if (currentView == ViewType::Kingdom) entry.id = selectedKingdomId;
        navHistory.push_back(entry);
    }

    inspectedApeId = id;
    selectedVillageId = 0;
    selectedKingdomId = 0;
    currentView = ViewType::Character;
    panelHeight = 540.f;
}

void ProfileViewManager::inspectVillage(sim::VillageID id, bool recordHistory) {
    if (recordHistory && currentView != ViewType::None) {
        HistoryEntry entry;
        entry.type = currentView;
        if (currentView == ViewType::Character) entry.id = inspectedApeId;
        else if (currentView == ViewType::Village) entry.id = selectedVillageId;
        else if (currentView == ViewType::Kingdom) entry.id = selectedKingdomId;
        navHistory.push_back(entry);
    }

    selectedVillageId = id;
    inspectedApeId = 0;
    selectedKingdomId = 0;
    currentView = ViewType::Village;
    panelHeight = 440.f;
}

void ProfileViewManager::inspectKingdom(sim::KingdomID id, bool recordHistory) {
    if (recordHistory && currentView != ViewType::None) {
        HistoryEntry entry;
        entry.type = currentView;
        if (currentView == ViewType::Character) entry.id = inspectedApeId;
        else if (currentView == ViewType::Village) entry.id = selectedVillageId;
        else if (currentView == ViewType::Kingdom) entry.id = selectedKingdomId;
        navHistory.push_back(entry);
    }

    selectedKingdomId = id;
    inspectedApeId = 0;
    selectedVillageId = 0;
    currentView = ViewType::Kingdom;
    panelHeight = 430.f;
}

void ProfileViewManager::close() {
    currentView = ViewType::None;
    inspectedApeId = 0;
    selectedVillageId = 0;
    selectedKingdomId = 0;
    navHistory.clear();
    interactiveButtons.clear();
}

void ProfileViewManager::registerButton(sf::FloatRect bounds, const std::function<void()>& action) {
    ClickableButton btn;
    btn.bounds = bounds;
    btn.onClick = action;
    interactiveButtons.push_back(btn);
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

void ProfileViewManager::drawBackButton(sf::RenderWindow& window, float x, float y) {
    sf::RectangleShape btn(sf::Vector2f(54.f, 20.f));
    btn.setPosition(x, y);
    btn.setFillColor(sf::Color(44, 30, 20, 240));
    btn.setOutlineColor(sf::Color(190, 150, 70));
    btn.setOutlineThickness(1.2f);
    window.draw(btn);

    if (font) {
        sf::Text bTxt("< Back", *font, 10);
        bTxt.setStyle(sf::Text::Bold);
        bTxt.setFillColor(sf::Color(245, 225, 160));
        bTxt.setPosition(x + 6.f, y + 3.f);
        window.draw(bTxt);
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

ProfileViewManager::FamilyInfo ProfileViewManager::resolveFamily(sim::EntityID apeId, sim::SimulationRegistry& reg) {
    FamilyInfo fam;
    sim::ApeData* targetApe = reg.getApe(apeId);
    if (!targetApe) return fam;

    if (targetApe->currentKingdom != 0) {
        sim::KingdomData* kd = reg.getKingdom(targetApe->currentKingdom);
        if (kd && kd->currentKingId != 0 && kd->currentKingId != targetApe->id) {
            fam.liegeId = kd->currentKingId;
            fam.liegeTitle = "King of " + kd->name;
        }
    }

    if (fam.liegeId == 0 && targetApe->villageId != 0) {
        sim::VillageData* vd = reg.getVillage(targetApe->villageId);
        if (vd && vd->leaderId != 0 && vd->leaderId != targetApe->id) {
            fam.liegeId = vd->leaderId;
            fam.liegeTitle = "Alpha of " + vd->name;
        }
    }

    for (const auto& pair : reg.getAllCharacters()) {
        const sim::Character& c = pair.second;
        if (c.id == apeId) {
            fam.fatherId = c.fatherId;
            fam.motherId = c.motherId;
            for (auto sId : c.spouseIds) {
                if (sId != 0 && reg.getApe(sId)) fam.spouseIds.push_back(sId);
            }
            for (auto chId : c.childrenIds) {
                if (chId != 0 && reg.getApe(chId)) fam.childrenIds.push_back(chId);
            }
            break;
        }
    }

    if (fam.fatherId != 0 || fam.motherId != 0) {
        for (const auto& pair : reg.getAllCharacters()) {
            const sim::Character& c = pair.second;
            if (c.id != apeId) {
                bool sharedFather = (fam.fatherId != 0 && c.fatherId == fam.fatherId);
                bool sharedMother = (fam.motherId != 0 && c.motherId == fam.motherId);
                if (sharedFather || sharedMother) {
                    fam.siblingIds.push_back(c.id);
                }
            }
        }
    }

    if (fam.spouseIds.empty() && targetApe->villageId != 0) {
        sim::VillageData* vd = reg.getVillage(targetApe->villageId);
        if (vd) {
            for (sim::EntityID mId : vd->members) {
                if (mId == targetApe->id) continue;
                sim::ApeData* other = reg.getApe(mId);
                if (!other || !other->alive) continue;

                if (fam.spouseIds.empty() && std::abs(other->age - targetApe->age) <= 6.f && other->age >= 16.f) {
                    fam.spouseIds.push_back(other->id);
                } else if (fam.childrenIds.size() < 3 && targetApe->age >= 24.f && other->age <= 15.f && (targetApe->age - other->age) >= 14.f) {
                    fam.childrenIds.push_back(other->id);
                } else if (fam.fatherId == 0 && other->age > targetApe->age + 14.f) {
                    fam.fatherId = other->id;
                } else if (fam.motherId == 0 && other->age > targetApe->age + 14.f && other->id != fam.fatherId) {
                    fam.motherId = other->id;
                } else if (fam.siblingIds.size() < 2 && other->id != fam.fatherId && other->id != fam.motherId && std::abs(other->age - targetApe->age) <= 4.f) {
                    fam.siblingIds.push_back(other->id);
                }
            }
        }
    }

    return fam;
}

bool ProfileViewManager::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    if (!isInspecting()) return false;

    sf::FloatRect profileRect(profilePanelPos.x, profilePanelPos.y, panelWidth, panelHeight);
    sf::FloatRect closeBtnRect(profilePanelPos.x + panelWidth - 26.f, profilePanelPos.y + 6.f, 20.f, 20.f);
    sf::FloatRect backBtnRect(profilePanelPos.x + 8.f, profilePanelPos.y + 6.f, 54.f, 20.f);

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        close();
        return true;
    }

    if (event.type == sf::Event::KeyPressed && currentView == ViewType::Character && inspectedApeId != 0) {
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

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), letterboxView);
        for (auto& btn : interactiveButtons) {
            btn.isHovered = btn.bounds.contains(vMouse);
        }
        if (profileRect.contains(vMouse)) return true;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);

        if (closeBtnRect.contains(vMouse)) {
            close();
            return true;
        }

        if (!navHistory.empty() && backBtnRect.contains(vMouse)) {
            HistoryEntry prev = navHistory.back();
            navHistory.pop_back();
            if (prev.type == ViewType::Character) inspectCharacter(prev.id, false);
            else if (prev.type == ViewType::Village) inspectVillage(prev.id, false);
            else if (prev.type == ViewType::Kingdom) inspectKingdom(prev.id, false);
            return true;
        }

        for (const auto& btn : interactiveButtons) {
            if (btn.bounds.contains(vMouse)) {
                if (btn.onClick) {
                    btn.onClick();
                    return true;
                }
            }
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
    if (!font || currentView == ViewType::None) return;

    interactiveButtons.clear();

    if (currentView == ViewType::Character && inspectedApeId != 0) {
        drawCharacterProfile(window, inspectedApeId, reg, controlledApeId);
    } else if (currentView == ViewType::Village && selectedVillageId != 0) {
        drawVillageProfile(window, selectedVillageId, reg);
    } else if (currentView == ViewType::Kingdom && selectedKingdomId != 0) {
        drawKingdomProfile(window, selectedKingdomId, reg, controlledApeId);
    }
}

void ProfileViewManager::drawVillageProfile(sf::RenderWindow& window, sim::VillageID vId, sim::SimulationRegistry& reg) {
    sim::VillageData* v = reg.getVillage(vId);
    if (!v) return;

    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape shadow(sf::Vector2f(panelWidth + 4.f, panelHeight + 4.f));
    shadow.setPosition(startX + 2.f, startY + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 185));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(24, 17, 13, 252));
    panel.setOutlineColor(sf::Color(185, 140, 65));
    panel.setOutlineThickness(1.5f);
    window.draw(panel);

    sf::RectangleShape hPlate(sf::Vector2f(panelWidth - 6.f, 32.f));
    hPlate.setPosition(startX + 3.f, startY + 3.f);
    hPlate.setFillColor(sf::Color(48, 32, 20));
    window.draw(hPlate);

    float titleX = (!navHistory.empty()) ? (startX + 68.f) : (startX + 12.f);
    float titleY = startY + 8.f;
    drawWrappedText(window, v->name + " (County)", titleX, titleY, panelWidth - 95.f, 13, sf::Color(255, 225, 130), true);

    if (!navHistory.empty()) drawBackButton(window, startX + 8.f, startY + 6.f);
    drawCloseButton(window, startX + panelWidth - 26.f, startY + 6.f);

    float curY = startY + 44.f;

    sim::ApeData* ruler = reg.getApe(v->leaderId);
    std::string rulerName = ruler ? ruler->name : "Unknown Chieftain";

    sim::KingdomData* kd = (v->kingdomId != 0) ? reg.getKingdom(v->kingdomId) : nullptr;
    bool isCrownDomain = (kd && kd->currentKingId == v->leaderId);

    sf::RectangleShape rBox(sf::Vector2f(panelWidth - 24.f, 52.f));
    rBox.setPosition(startX + 12.f, curY);
    rBox.setFillColor(sf::Color(38, 26, 18));
    rBox.setOutlineColor(sf::Color(215, 170, 70));
    rBox.setOutlineThickness(1.2f);
    window.draw(rBox);

    sf::Text rH("RULER & SEAT OF POWER", *font, 9);
    rH.setFillColor(sf::Color(200, 160, 80));
    rH.setStyle(sf::Text::Bold);
    rH.setPosition(startX + 20.f, curY + 6.f);
    window.draw(rH);

    sf::Text rNameTxt(rulerName, *font, 13);
    rNameTxt.setFillColor(sf::Color(255, 240, 190));
    rNameTxt.setStyle(sf::Text::Bold);
    rNameTxt.setPosition(startX + 20.f, curY + 18.f);
    window.draw(rNameTxt);

    std::string rSub = isCrownDomain ? "Direct Crown Domain of Sovereign" : "Vassal Clan Alpha";
    sf::Text rSubTxt(rSub, *font, 9);
    rSubTxt.setStyle(sf::Text::Italic);
    rSubTxt.setFillColor(sf::Color(175, 160, 130));
    rSubTxt.setPosition(startX + 20.f, curY + 34.f);
    window.draw(rSubTxt);

    sf::Text clickPrompt("[Inspect Ruler ->]", *font, 9);
    clickPrompt.setStyle(sf::Text::Bold);
    clickPrompt.setFillColor(sf::Color(255, 215, 75));
    sf::FloatRect cpb = clickPrompt.getLocalBounds();
    clickPrompt.setPosition(startX + panelWidth - cpb.width - 20.f, curY + 34.f);
    window.draw(clickPrompt);

    if (ruler) {
        registerButton(rBox.getGlobalBounds(), [this, ruler]() {
            inspectCharacter(ruler->id);
        });
    }

    curY += 62.f;

    if (kd && kd->currentKingId != 0 && kd->currentKingId != v->leaderId) {
        sim::ApeData* liege = reg.getApe(kd->currentKingId);
        std::string liegeName = liege ? liege->name : "Monarch";

        sf::RectangleShape lBox(sf::Vector2f(panelWidth - 24.f, 32.f));
        lBox.setPosition(startX + 12.f, curY);
        lBox.setFillColor(sf::Color(24, 20, 28));
        lBox.setOutlineColor(sf::Color(140, 110, 185));
        lBox.setOutlineThickness(1.f);
        window.draw(lBox);

        sf::Text liegeTxt("Liege: " + liegeName + " (" + kd->name + ")", *font, 10);
        liegeTxt.setFillColor(sf::Color(225, 205, 255));
        liegeTxt.setPosition(startX + 20.f, curY + 4.f);
        window.draw(liegeTxt);

        sf::Text liegeP("[View King ->]", *font, 9);
        liegeP.setFillColor(sf::Color(185, 155, 240));
        liegeP.setPosition(startX + 20.f, curY + 17.f);
        window.draw(liegeP);

        if (liege) {
            registerButton(lBox.getGlobalBounds(), [this, liege]() {
                inspectCharacter(liege->id);
            });
        }
        curY += 40.f;
    }

    auto drawField = [&](const std::string& label, const std::string& val, sf::Color valCol = sf::Color(240, 230, 210)) {
        sf::Text l(label, *font, 10);
        l.setFillColor(sf::Color(165, 145, 120));
        l.setPosition(startX + 14.f, curY);
        window.draw(l);
        curY += 13.f;

        drawWrappedText(window, val, startX + 14.f, curY, panelWidth - 28.f, 11, valCol, true);
        curY += 5.f;
    };

    std::string allegiance = kd ? ("Kingdom of " + kd->name) : "Independent Territory";
    drawField("Realm Allegiance", allegiance, sf::Color(100, 180, 240));
    drawField("Population", std::to_string(v->members.size()) + " inhabitants");

    sf::RectangleShape resBox(sf::Vector2f(panelWidth - 28.f, 44.f));
    resBox.setPosition(startX + 14.f, curY);
    resBox.setFillColor(sf::Color(16, 11, 8, 230));
    resBox.setOutlineColor(sf::Color(120, 85, 45));
    resBox.setOutlineThickness(1.f);
    window.draw(resBox);

    sf::Text resH("COUNTY STOCKPILES", *font, 9);
    resH.setFillColor(sf::Color(180, 145, 75));
    resH.setStyle(sf::Text::Bold);
    resH.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 5.f);
    window.draw(resH);

    std::string resContent = "Food: " + std::to_string(v->food) + "  Wood: " + std::to_string(v->wood) + "  Stone: " + std::to_string(v->stone);
    sf::Text resVals(resContent, *font, 11);
    resVals.setFillColor(sf::Color(220, 215, 195));
    resVals.setStyle(sf::Text::Bold);
    resVals.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 20.f);
    window.draw(resVals);

    curY += 52.f;
    drawField("Settlement Tier", "Tier " + std::to_string(static_cast<int>(v->tier)), sf::Color(235, 195, 55));
    drawField("Finished Structures", std::to_string(v->finishedStructures.size()) + " buildings");
}

void ProfileViewManager::drawCharacterProfile(sf::RenderWindow& window, sim::EntityID apeId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::ApeData* ape = reg.getApe(apeId);
    if (!ape) return;

    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape shadow(sf::Vector2f(panelWidth + 4.f, panelHeight + 4.f));
    shadow.setPosition(startX + 2.f, startY + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 185));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(24, 17, 13, 252));
    panel.setOutlineColor(sf::Color(185, 140, 65));
    panel.setOutlineThickness(1.5f);
    window.draw(panel);

    sf::RectangleShape headerPlate(sf::Vector2f(panelWidth - 6.f, 32.f));
    headerPlate.setPosition(startX + 3.f, startY + 3.f);
    headerPlate.setFillColor(sf::Color(48, 32, 20));
    window.draw(headerPlate);

    float titleX = (!navHistory.empty()) ? (startX + 68.f) : (startX + 12.f);
    float titleY = startY + 8.f;
    drawWrappedText(window, ape->name, titleX, titleY, panelWidth - 95.f, 13, sf::Color(255, 225, 130), true);

    if (!navHistory.empty()) drawBackButton(window, startX + 8.f, startY + 6.f);
    drawCloseButton(window, startX + panelWidth - 26.f, startY + 6.f);

    float curY = startY + 40.f;

    FamilyInfo fam = resolveFamily(apeId, reg);

    if (fam.liegeId != 0) {
        sim::ApeData* liege = reg.getApe(fam.liegeId);
        std::string lName = liege ? liege->name : "Monarch";

        sf::RectangleShape lBox(sf::Vector2f(panelWidth - 24.f, 26.f));
        lBox.setPosition(startX + 12.f, curY);
        lBox.setFillColor(sf::Color(32, 22, 38));
        lBox.setOutlineColor(sf::Color(160, 120, 210));
        lBox.setOutlineThickness(1.f);
        window.draw(lBox);

        sf::Text lTxt("Liege: " + lName + " (" + fam.liegeTitle + ")", *font, 10);
        lTxt.setFillColor(sf::Color(235, 215, 255));
        lTxt.setPosition(startX + 18.f, curY + 5.f);
        window.draw(lTxt);

        if (liege) {
            registerButton(lBox.getGlobalBounds(), [this, liege]() {
                inspectCharacter(liege->id);
            });
        }
        curY += 32.f;
    }

    sim::VillageData* apeVillage = reg.getVillage(ape->villageId);
    sim::KingdomData* apeKingdom = (ape->currentKingdom != 0) ? reg.getKingdom(ape->currentKingdom) : nullptr;
    std::string realmName = apeKingdom ? ("Kingdom of " + apeKingdom->name) : (apeVillage ? ("Clan of " + apeVillage->name) : "Wanderer");

    sf::Text rText(realmName + " | Age " + std::to_string(static_cast<int>(ape->age)), *font, 10);
    rText.setFillColor(sf::Color(185, 170, 145));
    rText.setPosition(startX + 12.f, curY);
    window.draw(rText);
    curY += 18.f;

    sf::RectangleShape fSection(sf::Vector2f(panelWidth - 24.f, 138.f));
    fSection.setPosition(startX + 12.f, curY);
    fSection.setFillColor(sf::Color(18, 13, 10, 240));
    fSection.setOutlineColor(sf::Color(120, 85, 45));
    fSection.setOutlineThickness(1.f);
    window.draw(fSection);

    sf::Text fH("HERITAGE & KINSHIP", *font, 9);
    fH.setStyle(sf::Text::Bold);
    fH.setFillColor(sf::Color(230, 195, 90));
    fH.setPosition(startX + 18.f, curY + 5.f);
    window.draw(fH);

    float fLineY = curY + 20.f;

    auto drawPill = [&](const std::string& label, sim::EntityID targetId, float px, float py, float pw) {
        sim::ApeData* relApe = reg.getApe(targetId);
        std::string disp = label + ": " + (relApe ? relApe->name : "None");

        sf::RectangleShape pill(sf::Vector2f(pw, 20.f));
        pill.setPosition(px, py);
        pill.setFillColor(relApe ? sf::Color(42, 30, 22) : sf::Color(25, 18, 14));
        pill.setOutlineColor(relApe ? sf::Color(175, 130, 60) : sf::Color(65, 50, 35));
        pill.setOutlineThickness(1.f);
        window.draw(pill);

        sf::Text pTxt(disp, *font, 9);
        pTxt.setFillColor(relApe ? sf::Color(245, 230, 180) : sf::Color(125, 115, 105));
        pTxt.setPosition(px + 6.f, py + 3.f);
        window.draw(pTxt);

        if (relApe) {
            registerButton(pill.getGlobalBounds(), [this, targetId]() {
                inspectCharacter(targetId);
            });
        }
    };

    float halfW = (panelWidth - 36.f) * 0.5f;
    drawPill("Father", fam.fatherId, startX + 16.f, fLineY, halfW);
    drawPill("Mother", fam.motherId, startX + 20.f + halfW, fLineY, halfW);
    fLineY += 25.f;

    sim::EntityID primarySpouse = fam.spouseIds.empty() ? 0 : fam.spouseIds.front();
    drawPill("Pair-Bond", primarySpouse, startX + 16.f, fLineY, panelWidth - 32.f);
    fLineY += 25.f;

    sf::Text chH("Children (" + std::to_string(fam.childrenIds.size()) + "):", *font, 9);
    chH.setFillColor(sf::Color(175, 160, 135));
    chH.setPosition(startX + 18.f, fLineY + 2.f);
    window.draw(chH);

    float chX = startX + 85.f;
    for (size_t cIdx = 0; cIdx < std::min<size_t>(fam.childrenIds.size(), 2); ++cIdx) {
        sim::ApeData* cApe = reg.getApe(fam.childrenIds[cIdx]);
        if (!cApe) continue;

        std::string cStr = cApe->name + " (" + std::to_string(static_cast<int>(cApe->age)) + ")";
        sf::Text cTxt(cStr, *font, 9);
        sf::FloatRect ctb = cTxt.getLocalBounds();

        sf::RectangleShape cPill(sf::Vector2f(ctb.width + 12.f, 18.f));
        cPill.setPosition(chX, fLineY);
        cPill.setFillColor(sf::Color(35, 25, 18));
        cPill.setOutlineColor(sf::Color(140, 100, 50));
        cPill.setOutlineThickness(1.f);
        window.draw(cPill);

        cTxt.setFillColor(sf::Color(235, 215, 165));
        cTxt.setPosition(chX + 6.f, fLineY + 1.f);
        window.draw(cTxt);

        sim::EntityID childId = fam.childrenIds[cIdx];
        registerButton(cPill.getGlobalBounds(), [this, childId]() {
            inspectCharacter(childId);
        });

        chX += ctb.width + 18.f;
    }

    fLineY += 24.f;

    sf::Text sibH("Siblings (" + std::to_string(fam.siblingIds.size()) + "):", *font, 9);
    sibH.setFillColor(sf::Color(175, 160, 135));
    sibH.setPosition(startX + 18.f, fLineY + 2.f);
    window.draw(sibH);

    float sibX = startX + 85.f;
    for (size_t sIdx = 0; sIdx < std::min<size_t>(fam.siblingIds.size(), 2); ++sIdx) {
        sim::ApeData* sApe = reg.getApe(fam.siblingIds[sIdx]);
        if (!sApe) continue;

        sf::Text sTxt(sApe->name, *font, 9);
        sf::FloatRect stb = sTxt.getLocalBounds();

        sf::RectangleShape sPill(sf::Vector2f(stb.width + 12.f, 18.f));
        sPill.setPosition(sibX, fLineY);
        sPill.setFillColor(sf::Color(35, 25, 18));
        sPill.setOutlineColor(sf::Color(140, 100, 50));
        sPill.setOutlineThickness(1.f);
        window.draw(sPill);

        sTxt.setFillColor(sf::Color(235, 215, 165));
        sTxt.setPosition(sibX + 6.f, fLineY + 1.f);
        window.draw(sTxt);

        sim::EntityID sibId = fam.siblingIds[sIdx];
        registerButton(sPill.getGlobalBounds(), [this, sibId]() {
            inspectCharacter(sibId);
        });

        sibX += stb.width + 18.f;
    }

    curY += 148.f;

    float statBoxW = 68.f;
    float statBoxH = 34.f;
    float statSpacing = 8.f;
    float statStartX = startX + 12.f;

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
        box.setPosition(bx, curY);
        box.setFillColor(sf::Color(16, 11, 8, 230));
        box.setOutlineColor(stats[i].col);
        box.setOutlineThickness(1.f);
        window.draw(box);

        sf::Text sLbl(stats[i].label, *font, 9);
        sLbl.setFillColor(sf::Color(170, 170, 170));
        sLbl.setPosition(bx + 6.f, curY + 2.f);
        window.draw(sLbl);

        sf::Text sVal(std::to_string(stats[i].val), *font, 13);
        sVal.setFillColor(sf::Color::White);
        sVal.setStyle(sf::Text::Bold);
        sVal.setPosition(bx + 6.f, curY + 14.f);
        window.draw(sVal);
    }

    curY += 42.f;

    sf::Text traitsHeader("CHARACTER TRAITS", *font, 9);
    traitsHeader.setFillColor(sf::Color(185, 145, 65));
    traitsHeader.setPosition(startX + 14.f, curY);
    window.draw(traitsHeader);
    curY += 16.f;

    float tBadgeX = startX + 12.f;

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
        sf::Text tTxt(tName, *font, 9);
        sf::FloatRect tb = tTxt.getLocalBounds();

        sf::RectangleShape badge(sf::Vector2f(tb.width + 10.f, 18.f));
        badge.setPosition(tBadgeX, curY);
        badge.setFillColor(sf::Color(38, 28, 20));
        badge.setOutlineColor(sf::Color(130, 95, 45));
        badge.setOutlineThickness(1.f);
        window.draw(badge);

        tTxt.setFillColor(sf::Color(235, 215, 165));
        tTxt.setPosition(tBadgeX + 5.f, curY + 1.f);
        window.draw(tTxt);

        tBadgeX += tb.width + 16.f;
        if (tBadgeX > startX + panelWidth - 70.f) {
            tBadgeX = startX + 12.f;
            curY += 22.f;
        }
    }

    curY += 28.f;

    int personalOp = 0;
    sim::ApeData* player = reg.getApe(controlledApeId);
    if (player) {
        if (ape->opinions.count(player->id)) personalOp = ape->opinions[player->id];
        else if (ape->villageId == player->villageId) {
            personalOp = 20;
            ape->opinions[player->id] = personalOp;
        }
    }

    std::string relText = "Opinion of You: " + (personalOp >= 0 ? ("+" + std::to_string(personalOp)) : std::to_string(personalOp));
    sf::Color relCol = (personalOp >= 30) ? sf::Color(100, 235, 100) : (personalOp >= 0 ? sf::Color(180, 225, 140) : sf::Color(240, 70, 70));

    sf::Text opTxt(relText, *font, 10);
    opTxt.setFillColor(relCol);
    opTxt.setPosition(startX + 14.f, curY);
    window.draw(opTxt);
}

void ProfileViewManager::drawKingdomProfile(sf::RenderWindow& window, sim::KingdomID kId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::KingdomData* k = reg.getKingdom(kId);
    if (!k) return;

    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape shadow(sf::Vector2f(panelWidth + 4.f, panelHeight + 4.f));
    shadow.setPosition(startX + 2.f, startY + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 185));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(24, 17, 13, 252));
    panel.setOutlineColor(sf::Color(185, 140, 65));
    panel.setOutlineThickness(1.5f);
    window.draw(panel);

    sf::RectangleShape hPlate(sf::Vector2f(panelWidth - 6.f, 32.f));
    hPlate.setPosition(startX + 3.f, startY + 3.f);
    hPlate.setFillColor(sf::Color(48, 32, 20));
    window.draw(hPlate);

    float titleX = (!navHistory.empty()) ? (startX + 68.f) : (startX + 12.f);
    float titleY = startY + 8.f;
    drawWrappedText(window, "KINGDOM OF " + k->name, titleX, titleY, panelWidth - 95.f, 13, k->color, true);

    if (!navHistory.empty()) drawBackButton(window, startX + 8.f, startY + 6.f);
    drawCloseButton(window, startX + panelWidth - 26.f, startY + 6.f);

    float curY = startY + 44.f;

    sim::ApeData* ruler = reg.getApe(k->currentKingId);
    std::string rulerName = ruler ? ruler->name : "Monarch";

    sf::RectangleShape kBox(sf::Vector2f(panelWidth - 24.f, 48.f));
    kBox.setPosition(startX + 12.f, curY);
    kBox.setFillColor(sf::Color(38, 26, 18));
    kBox.setOutlineColor(sf::Color(215, 170, 70));
    kBox.setOutlineThickness(1.2f);
    window.draw(kBox);

    sf::Text rH("SOVEREIGN MONARCH", *font, 9);
    rH.setFillColor(sf::Color(200, 160, 80));
    rH.setStyle(sf::Text::Bold);
    rH.setPosition(startX + 20.f, curY + 6.f);
    window.draw(rH);

    sf::Text rNameTxt(rulerName, *font, 13);
    rNameTxt.setFillColor(sf::Color(255, 240, 190));
    rNameTxt.setStyle(sf::Text::Bold);
    rNameTxt.setPosition(startX + 20.f, curY + 18.f);
    window.draw(rNameTxt);

    sf::Text clickPrompt("[Inspect Monarch ->]", *font, 9);
    clickPrompt.setStyle(sf::Text::Bold);
    clickPrompt.setFillColor(sf::Color(255, 215, 75));
    sf::FloatRect cpb = clickPrompt.getLocalBounds();
    clickPrompt.setPosition(startX + panelWidth - cpb.width - 20.f, curY + 30.f);
    window.draw(clickPrompt);

    if (ruler) {
        registerButton(kBox.getGlobalBounds(), [this, ruler]() {
            inspectCharacter(ruler->id);
        });
    }

    curY += 60.f;

    auto drawField = [&](const std::string& label, const std::string& val, sf::Color valCol = sf::Color(240, 230, 210), bool bold = false) {
        sf::Text l(label, *font, 10);
        l.setFillColor(sf::Color(165, 145, 120));
        l.setPosition(startX + 14.f, curY);
        window.draw(l);
        curY += 13.f;

        drawWrappedText(window, val, startX + 14.f, curY, panelWidth - 28.f, 11, valCol, bold);
        curY += 5.f;
    };

    sim::VillageData* cap = reg.getVillage(k->capitalVillageId);
    std::string capName = cap ? cap->name : "Capital";
    drawField("Capital Seat", capName);
    drawField("Realm Scale", std::to_string(k->population) + " apes | " + std::to_string(k->controlledVillages.size()) + " settlements");

    sf::RectangleShape resBox(sf::Vector2f(panelWidth - 28.f, 44.f));
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
    resVals.setPosition(resBox.getPosition().x + 8.f, resBox.getPosition().y + 20.f);
    window.draw(resVals);

    curY += 52.f;

    drawField("Military Power", std::to_string(k->militaryStrength) + " strength", sf::Color(245, 130, 130), true);

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
    drawField("Diplomatic Status", relStr, relCol, true);
}