#include "ui/ProfileViewManager.h"
#include <algorithm>
#include <sstream>
#include <cmath>

ProfileViewManager::ProfileViewManager()
    : font(nullptr), currentView(ViewType::None), activeTab(CharacterTab::Overview),
      inspectedApeId(0), selectedVillageId(0), selectedKingdomId(0),
      isDragging(false), dragOffset(0.f, 0.f), profilePanelPos(70.f, 80.f),
      panelWidth(430.f), panelHeight(550.f) {}

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
    panelHeight = 550.f;
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
    panelHeight = 490.f;
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
    panelHeight = 490.f;
}

void ProfileViewManager::close() {
    currentView = ViewType::None;
    inspectedApeId = 0;
    selectedVillageId = 0;
    selectedKingdomId = 0;
    isDragging = false;
    navHistory.clear();
    interactiveButtons.clear();
}

void ProfileViewManager::registerButton(sf::FloatRect bounds, const std::function<void()>& action) {
    ClickableButton btn;
    btn.bounds = bounds;
    btn.onClick = action;
    interactiveButtons.push_back(btn);
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
                if (sharedFather || sharedMother) fam.siblingIds.push_back(c.id);
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

                if (fam.spouseIds.empty() && std::abs(other->age - targetApe->age) <= 5.f && other->age >= 16.f) {
                    fam.spouseIds.push_back(other->id);
                } else if (fam.childrenIds.size() < 3 && targetApe->age >= 24.f && other->age <= 14.f && (targetApe->age - other->age) >= 15.f) {
                    fam.childrenIds.push_back(other->id);
                }
            }
        }
    }

    return fam;
}

bool ProfileViewManager::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    if (!isInspecting()) return false;

    sf::FloatRect profileRect(profilePanelPos.x, profilePanelPos.y, panelWidth, panelHeight);
    sf::FloatRect closeBtnRect(profilePanelPos.x + panelWidth - 30.f, profilePanelPos.y + 7.f, 22.f, 22.f);
    sf::FloatRect backBtnRect(profilePanelPos.x + 8.f, profilePanelPos.y + 7.f, 54.f, 22.f);
    headerDragBounds = sf::FloatRect(profilePanelPos.x, profilePanelPos.y, panelWidth, 42.f);

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        close();
        return true;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDragging = false;
    }

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f vMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), letterboxView);

        if (isDragging) {
            profilePanelPos = vMouse - dragOffset;
            profilePanelPos.x = std::clamp(profilePanelPos.x, 10.f, 1270.f - panelWidth);
            profilePanelPos.y = std::clamp(profilePanelPos.y, 10.f, 710.f - panelHeight);
            return true;
        }

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

        if (headerDragBounds.contains(vMouse)) {
            isDragging = true;
            dragOffset = vMouse - profilePanelPos;
            return true;
        }

        if (profileRect.contains(vMouse)) {
            return true;
        }

        close();
        return false;
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

void ProfileViewManager::drawHeader(sf::RenderWindow& window, const std::string& title, const std::string& subtitle, sf::Color titleColor) {
    float startX = profilePanelPos.x;
    float startY = profilePanelPos.y;

    sf::RectangleShape shadow(sf::Vector2f(panelWidth + 6.f, panelHeight + 6.f));
    shadow.setPosition(startX + 3.f, startY + 3.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(20, 15, 11, 252));
    panel.setOutlineColor(sf::Color(175, 130, 60));
    panel.setOutlineThickness(1.8f);
    window.draw(panel);

    sf::RectangleShape header(sf::Vector2f(panelWidth - 8.f, 40.f));
    header.setPosition(startX + 4.f, startY + 4.f);
    header.setFillColor(sf::Color(42, 28, 18));
    header.setOutlineColor(sf::Color(85, 60, 35));
    header.setOutlineThickness(1.f);
    window.draw(header);

    for (int g = 0; g < 4; ++g) {
        sf::RectangleShape grip(sf::Vector2f(22.f, 1.5f));
        grip.setPosition(startX + (panelWidth - 22.f) * 0.5f, startY + 6.f + g * 3.f);
        grip.setFillColor(sf::Color(135, 105, 65, 180));
        window.draw(grip);
    }

    float leftOffset = (!navHistory.empty()) ? 66.f : 12.f;

    sf::Text t(title, *font, 12);
    t.setStyle(sf::Text::Bold);
    t.setFillColor(titleColor);
    t.setPosition(startX + leftOffset, startY + 9.f);

    float maxTitleW = panelWidth - leftOffset - 38.f;
    if (t.getLocalBounds().width > maxTitleW) {
        std::string trimmed = title;
        while (!trimmed.empty() && t.getLocalBounds().width > maxTitleW - 10.f) {
            trimmed.pop_back();
            t.setString(trimmed + "...");
        }
    }
    window.draw(t);

    sf::Text sub(subtitle, *font, 9);
    sub.setFillColor(sf::Color(185, 165, 135));
    sub.setPosition(startX + leftOffset, startY + 24.f);
    window.draw(sub);

    if (!navHistory.empty()) {
        sf::RectangleShape bBtn(sf::Vector2f(50.f, 22.f));
        bBtn.setPosition(startX + 8.f, startY + 11.f);
        bBtn.setFillColor(sf::Color(32, 22, 14));
        bBtn.setOutlineColor(sf::Color(185, 140, 65));
        bBtn.setOutlineThickness(1.f);
        window.draw(bBtn);

        sf::Text bTxt("< Back", *font, 9);
        bTxt.setStyle(sf::Text::Bold);
        bTxt.setFillColor(sf::Color(245, 225, 160));
        bTxt.setPosition(startX + 13.f, startY + 15.f);
        window.draw(bTxt);
    }

    sf::RectangleShape cBtn(sf::Vector2f(22.f, 22.f));
    cBtn.setPosition(startX + panelWidth - 30.f, startY + 11.f);
    cBtn.setFillColor(sf::Color(135, 22, 16));
    cBtn.setOutlineColor(sf::Color(235, 195, 75));
    cBtn.setOutlineThickness(1.f);
    window.draw(cBtn);

    sf::Text xTxt("x", *font, 12);
    xTxt.setStyle(sf::Text::Bold);
    xTxt.setFillColor(sf::Color::White);
    xTxt.setPosition(startX + panelWidth - 24.f, startY + 13.f);
    window.draw(xTxt);
}

void ProfileViewManager::drawTabs(sf::RenderWindow& window) {
    float startX = profilePanelPos.x + 10.f;
    float startY = profilePanelPos.y + 44.f;
    float tabW = (panelWidth - 20.f) / 3.f;
    float tabH = 24.f;

    struct TabItem { CharacterTab tab; std::string name; };
    std::vector<TabItem> tabs = {
        {CharacterTab::Overview, "I. Overview"},
        {CharacterTab::Kinship, "II. Kinship"},
        {CharacterTab::Realm, "III. Realm"}
    };

    for (size_t i = 0; i < tabs.size(); ++i) {
        float tx = startX + i * tabW;
        bool isActive = (activeTab == tabs[i].tab);

        sf::RectangleShape tShape(sf::Vector2f(tabW - 2.f, tabH));
        tShape.setPosition(tx, startY);
        tShape.setFillColor(isActive ? sf::Color(65, 45, 26) : sf::Color(28, 20, 15));
        tShape.setOutlineColor(isActive ? sf::Color(235, 195, 75) : sf::Color(75, 55, 35));
        tShape.setOutlineThickness(1.f);
        window.draw(tShape);

        sf::Text tTxt(tabs[i].name, *font, 10);
        tTxt.setStyle(sf::Text::Bold);
        tTxt.setFillColor(isActive ? sf::Color(255, 235, 160) : sf::Color(155, 140, 120));
        sf::FloatRect tb = tTxt.getLocalBounds();
        tTxt.setPosition(tx + (tabW - 2.f - tb.width) * 0.5f, startY + 5.f);
        window.draw(tTxt);

        CharacterTab target = tabs[i].tab;
        registerButton(tShape.getGlobalBounds(), [this, target]() {
            activeTab = target;
        });
    }
}

void ProfileViewManager::drawStatBox(sf::RenderWindow& window, float x, float y, float w, float h, const std::string& label, int val, sf::Color accent) {
    sf::RectangleShape box(sf::Vector2f(w, h));
    box.setPosition(x, y);
    box.setFillColor(sf::Color(16, 12, 9, 240));
    box.setOutlineColor(accent);
    box.setOutlineThickness(1.2f);
    window.draw(box);

    sf::RectangleShape bar(sf::Vector2f(w, 3.f));
    bar.setPosition(x, y);
    bar.setFillColor(accent);
    window.draw(bar);

    sf::Text l(label, *font, 9);
    l.setFillColor(sf::Color(170, 160, 145));
    l.setPosition(x + 6.f, y + 6.f);
    window.draw(l);

    sf::Text v(std::to_string(val), *font, 15);
    v.setStyle(sf::Text::Bold);
    v.setFillColor(sf::Color::White);
    v.setPosition(x + 6.f, y + 17.f);
    window.draw(v);

    std::string grade = (val >= 18) ? "Master" : (val >= 14 ? "Adept" : (val >= 10 ? "Average" : "Poor"));
    sf::Text g(grade, *font, 8);
    g.setFillColor(accent);
    sf::FloatRect gb = g.getLocalBounds();
    g.setPosition(x + w - gb.width - 6.f, y + 20.f);
    window.draw(g);
}

void ProfileViewManager::drawOpinionBar(sf::RenderWindow& window, float x, float y, float w, int opinion) {
    sf::Text l("OPINION OF YOU: " + (opinion >= 0 ? ("+" + std::to_string(opinion)) : std::to_string(opinion)), *font, 9);
    l.setStyle(sf::Text::Bold);
    l.setFillColor(opinion >= 25 ? sf::Color(110, 235, 110) : (opinion >= 0 ? sf::Color(220, 205, 120) : sf::Color(240, 70, 70)));
    l.setPosition(x, y);
    window.draw(l);

    float barY = y + 15.f;
    sf::RectangleShape track(sf::Vector2f(w, 8.f));
    track.setPosition(x, barY);
    track.setFillColor(sf::Color(18, 14, 11));
    track.setOutlineColor(sf::Color(75, 55, 38));
    track.setOutlineThickness(1.f);
    window.draw(track);

    float midX = x + w * 0.5f;
    sf::RectangleShape centerPip(sf::Vector2f(2.f, 10.f));
    centerPip.setPosition(midX - 1.f, barY - 1.f);
    centerPip.setFillColor(sf::Color(140, 120, 95));
    window.draw(centerPip);

    float norm = std::clamp(opinion / 100.f, -1.f, 1.f);
    float fillW = std::abs(norm) * (w * 0.5f);

    if (fillW > 1.f) {
        sf::RectangleShape fill(sf::Vector2f(fillW, 8.f));
        if (norm >= 0.f) {
            fill.setPosition(midX, barY);
            fill.setFillColor(sf::Color(75, 205, 75));
        } else {
            fill.setPosition(midX - fillW, barY);
            fill.setFillColor(sf::Color(225, 50, 45));
        }
        window.draw(fill);
    }
}

void ProfileViewManager::drawCharacterProfile(sf::RenderWindow& window, sim::EntityID apeId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::ApeData* ape = reg.getApe(apeId);
    if (!ape) return;

    ape->skills.combat = std::clamp(ape->skills.combat, 1.0f, 25.0f);
    ape->skills.leadership = std::clamp(ape->skills.leadership, 1.0f, 25.0f);
    ape->skills.building = std::clamp(ape->skills.building, 1.0f, 25.0f);
    ape->skills.gathering = std::clamp(ape->skills.gathering, 1.0f, 25.0f);

    sim::VillageData* apeVillage = reg.getVillage(ape->villageId);
    sim::KingdomData* apeKingdom = (ape->currentKingdom != 0) ? reg.getKingdom(ape->currentKingdom) : nullptr;
    std::string realmName = apeKingdom ? ("Kingdom of " + apeKingdom->name) : (apeVillage ? ("Clan of " + apeVillage->name) : "Independent Wanderer");

    drawHeader(window, ape->name, realmName + " | Age " + std::to_string(static_cast<int>(ape->age)));
    drawTabs(window);

    float startX = profilePanelPos.x + 14.f;
    float curY = profilePanelPos.y + 80.f;
    float contentW = panelWidth - 28.f;

    FamilyInfo fam = resolveFamily(apeId, reg);

    if (activeTab == CharacterTab::Overview) {
        int mar = static_cast<int>(std::round(ape->skills.combat));
        int dip = static_cast<int>(std::round(ape->skills.leadership));
        int adm = static_cast<int>(std::round(ape->skills.building));
        int fow = static_cast<int>(std::round(ape->skills.gathering));
        
        float sW = (contentW - 18.f) / 4.f;
        drawStatBox(window, startX + 0 * (sW + 6.f), curY, sW, 40.f, "MAR", mar, sf::Color(215, 60, 60));
        drawStatBox(window, startX + 1 * (sW + 6.f), curY, sW, 40.f, "DIP", dip, sf::Color(235, 185, 55));
        drawStatBox(window, startX + 2 * (sW + 6.f), curY, sW, 40.f, "ADM", adm, sf::Color(85, 185, 85));
        drawStatBox(window, startX + 3 * (sW + 6.f), curY, sW, 40.f, "FOR", fow, sf::Color(70, 165, 225));
        curY += 50.f;

        sf::Text tHeader("PERSONALITY & CONGENITAL TRAITS", *font, 9);
        tHeader.setStyle(sf::Text::Bold);
        tHeader.setFillColor(sf::Color(205, 170, 95));
        tHeader.setPosition(startX, curY);
        window.draw(tHeader);
        curY += 15.f;

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

        if (ape->traits.empty()) {
            sf::Text noT("Unremarkable Disposition", *font, 9);
            noT.setFillColor(sf::Color(140, 130, 120));
            noT.setPosition(startX, curY);
            window.draw(noT);
            curY += 20.f;
        } else {
            float badgeX = startX;
            for (auto tr : ape->traits) {
                std::string tName = traitToStr(tr);
                sf::Text tTxt(tName, *font, 9);
                sf::FloatRect tb = tTxt.getLocalBounds();

                sf::RectangleShape badge(sf::Vector2f(tb.width + 12.f, 20.f));
                badge.setPosition(badgeX, curY);
                badge.setFillColor(sf::Color(36, 26, 18));
                badge.setOutlineColor(sf::Color(165, 125, 65));
                badge.setOutlineThickness(1.f);
                window.draw(badge);

                tTxt.setFillColor(sf::Color(245, 230, 185));
                tTxt.setPosition(badgeX + 6.f, curY + 3.f);
                window.draw(tTxt);

                badgeX += tb.width + 18.f;
                if (badgeX > startX + contentW - 60.f) {
                    badgeX = startX;
                    curY += 24.f;
                }
            }
            curY += 28.f;
        }

        int op = 0;
        sim::ApeData* player = reg.getApe(controlledApeId);
        if (player) {
            if (ape->opinions.count(player->id)) op = ape->opinions[player->id];
            else if (ape->villageId == player->villageId) op = 15;
        }
        drawOpinionBar(window, startX, curY, contentW, op);
        curY += 35.f;

        std::string roleTitle = "Unassigned Commoner";
        switch (ape->councilRole) {
            case sim::CouncilRole::WarChief: roleTitle = "War Chief of the Realm"; break;
            case sim::CouncilRole::ChiefBuilder: roleTitle = "Chief Builder"; break;
            case sim::CouncilRole::LeadForager: roleTitle = "Grand Forager"; break;
            case sim::CouncilRole::Shaman: roleTitle = "Elder Shaman"; break;
            default: break;
        }

        sf::RectangleShape dutyBox(sf::Vector2f(contentW, 46.f));
        dutyBox.setPosition(startX, curY);
        dutyBox.setFillColor(sf::Color(18, 14, 10));
        dutyBox.setOutlineColor(sf::Color(120, 85, 45));
        dutyBox.setOutlineThickness(1.f);
        window.draw(dutyBox);

        sf::Text dH("COUNCIL APPOINTMENT", *font, 9);
        dH.setStyle(sf::Text::Bold);
        dH.setFillColor(sf::Color(190, 155, 75));
        dH.setPosition(startX + 8.f, curY + 6.f);
        window.draw(dH);

        sf::Text rT(roleTitle, *font, 11);
        rT.setStyle(sf::Text::Bold);
        rT.setFillColor(sf::Color(255, 240, 200));
        rT.setPosition(startX + 8.f, curY + 20.f);
        window.draw(rT);

        sf::Text hKey("[1-4] Assign Council, [0] Revoke", *font, 8);
        hKey.setFillColor(sf::Color(160, 145, 120));
        sf::FloatRect hkb = hKey.getLocalBounds();
        hKey.setPosition(startX + contentW - hkb.width - 8.f, curY + 23.f);
        window.draw(hKey);

    } else if (activeTab == CharacterTab::Kinship) {
        auto drawRelCard = [&](const std::string& relationLabel, sim::EntityID targetId) {
            sim::ApeData* rel = reg.getApe(targetId);
            std::string nameStr = rel ? rel->name : "Unknown / Wild Ancestry";
            std::string subStr = rel ? ("Age " + std::to_string(static_cast<int>(rel->age))) : "No lineage record";

            sf::RectangleShape card(sf::Vector2f(contentW, 36.f));
            card.setPosition(startX, curY);
            card.setFillColor(rel ? sf::Color(32, 23, 16) : sf::Color(20, 15, 12));
            card.setOutlineColor(rel ? sf::Color(165, 125, 60) : sf::Color(70, 50, 35));
            card.setOutlineThickness(1.f);
            window.draw(card);

            sf::Text lbl(relationLabel, *font, 9);
            lbl.setStyle(sf::Text::Bold);
            lbl.setFillColor(sf::Color(210, 175, 95));
            lbl.setPosition(startX + 8.f, curY + 4.f);
            window.draw(lbl);

            sf::Text nTxt(nameStr, *font, 11);
            nTxt.setStyle(sf::Text::Bold);
            nTxt.setFillColor(rel ? sf::Color(255, 245, 215) : sf::Color(130, 120, 110));
            nTxt.setPosition(startX + 8.f, curY + 16.f);
            window.draw(nTxt);

            if (rel) {
                sf::Text insp("[Inspect ->]", *font, 8);
                insp.setFillColor(sf::Color(245, 205, 80));
                sf::FloatRect ib = insp.getLocalBounds();
                insp.setPosition(startX + contentW - ib.width - 8.f, curY + 18.f);
                window.draw(insp);

                registerButton(card.getGlobalBounds(), [this, targetId]() {
                    inspectCharacter(targetId);
                });
            }

            curY += 42.f;
        };

        if (fam.liegeId != 0) {
            drawRelCard("Sovereign Liege (" + fam.liegeTitle + ")", fam.liegeId);
        }

        drawRelCard("Father", fam.fatherId);
        drawRelCard("Mother", fam.motherId);

        sim::EntityID spouse = fam.spouseIds.empty() ? 0 : fam.spouseIds.front();
        drawRelCard("Pair-Bond (Consort)", spouse);

        sf::Text chH("CHILDREN (" + std::to_string(fam.childrenIds.size()) + ")", *font, 9);
        chH.setStyle(sf::Text::Bold);
        chH.setFillColor(sf::Color(200, 165, 90));
        chH.setPosition(startX, curY);
        window.draw(chH);
        curY += 14.f;

        if (fam.childrenIds.empty()) {
            sf::Text nc("No recorded offspring", *font, 9);
            nc.setFillColor(sf::Color(135, 125, 115));
            nc.setPosition(startX, curY);
            window.draw(nc);
            curY += 22.f;
        } else {
            for (size_t i = 0; i < std::min<size_t>(fam.childrenIds.size(), 3); ++i) {
                drawRelCard("Child", fam.childrenIds[i]);
            }
        }

    } else if (activeTab == CharacterTab::Realm) {
        if (fam.liegeId != 0) {
            sim::ApeData* liege = reg.getApe(fam.liegeId);
            std::string lName = liege ? liege->name : "Monarch";

            sf::RectangleShape liegeBox(sf::Vector2f(contentW, 44.f));
            liegeBox.setPosition(startX, curY);
            liegeBox.setFillColor(sf::Color(32, 22, 28));
            liegeBox.setOutlineColor(sf::Color(175, 120, 205));
            liegeBox.setOutlineThickness(1.2f);
            window.draw(liegeBox);

            sf::Text lH("FEUDAL OBLIGATION", *font, 9);
            lH.setStyle(sf::Text::Bold);
            lH.setFillColor(sf::Color(215, 175, 245));
            lH.setPosition(startX + 8.f, curY + 5.f);
            window.draw(lH);

            sf::Text lN("Sworn Vassal to " + lName + " (" + fam.liegeTitle + ")", *font, 10);
            lN.setStyle(sf::Text::Bold);
            lN.setFillColor(sf::Color::White);
            lN.setPosition(startX + 8.f, curY + 20.f);
            window.draw(lN);

            if (liege) {
                registerButton(liegeBox.getGlobalBounds(), [this, liege]() {
                    inspectCharacter(liege->id);
                });
            }
            curY += 52.f;
        }

        if (apeVillage) {
            sf::RectangleShape cCard(sf::Vector2f(contentW, 50.f));
            cCard.setPosition(startX, curY);
            cCard.setFillColor(sf::Color(28, 20, 14));
            cCard.setOutlineColor(sf::Color(190, 140, 60));
            cCard.setOutlineThickness(1.2f);
            window.draw(cCard);

            sf::Text cH("HOME SEAT & DOMAIN", *font, 9);
            cH.setStyle(sf::Text::Bold);
            cH.setFillColor(sf::Color(230, 190, 85));
            cH.setPosition(startX + 8.f, curY + 5.f);
            window.draw(cH);

            sf::Text cN(apeVillage->name + " (County)", *font, 12);
            cN.setStyle(sf::Text::Bold);
            cN.setFillColor(sf::Color(255, 240, 190));
            cN.setPosition(startX + 8.f, curY + 18.f);
            window.draw(cN);

            std::string sub = "Tier " + std::to_string(static_cast<int>(apeVillage->tier)) + " | Population: " + std::to_string(apeVillage->members.size());
            sf::Text cSub(sub, *font, 9);
            cSub.setFillColor(sf::Color(180, 165, 140));
            cSub.setPosition(startX + 8.f, curY + 34.f);
            window.draw(cSub);

            sf::Text vBtn("[Inspect County ->]", *font, 9);
            vBtn.setFillColor(sf::Color(255, 215, 80));
            sf::FloatRect vbb = vBtn.getLocalBounds();
            vBtn.setPosition(startX + contentW - vbb.width - 8.f, curY + 32.f);
            window.draw(vBtn);

            sim::VillageID vId = apeVillage->id;
            registerButton(cCard.getGlobalBounds(), [this, vId]() {
                inspectVillage(vId);
            });
            curY += 58.f;
        }
    }
}

void ProfileViewManager::drawVillageProfile(sf::RenderWindow& window, sim::VillageID vId, sim::SimulationRegistry& reg) {
    sim::VillageData* v = reg.getVillage(vId);
    if (!v) return;

    sim::KingdomData* kd = (v->kingdomId != 0) ? reg.getKingdom(v->kingdomId) : nullptr;
    std::string realmStr = kd ? ("Kingdom of " + kd->name) : "Independent County";

    drawHeader(window, v->name + " (County)", realmStr, sf::Color(255, 230, 120));

    float startX = profilePanelPos.x + 12.f;
    float curY = profilePanelPos.y + 48.f;
    float contentW = panelWidth - 24.f;

    sim::ApeData* ruler = reg.getApe(v->leaderId);
    std::string rulerName = ruler ? ruler->name : "Unknown Chieftain";

    sf::RectangleShape rBox(sf::Vector2f(contentW, 46.f));
    rBox.setPosition(startX, curY);
    rBox.setFillColor(sf::Color(36, 24, 16));
    rBox.setOutlineColor(sf::Color(215, 165, 60));
    rBox.setOutlineThickness(1.2f);
    window.draw(rBox);

    sf::Text rH("RULING CHIEFTAIN", *font, 9);
    rH.setStyle(sf::Text::Bold);
    rH.setFillColor(sf::Color(210, 170, 75));
    rH.setPosition(startX + 8.f, curY + 5.f);
    window.draw(rH);

    sf::Text rN(rulerName, *font, 12);
    rN.setStyle(sf::Text::Bold);
    rN.setFillColor(sf::Color(255, 240, 200));
    rN.setPosition(startX + 8.f, curY + 18.f);
    window.draw(rN);

    if (ruler) {
        sf::Text clickPrompt("[Inspect Ruler ->]", *font, 9);
        clickPrompt.setFillColor(sf::Color(255, 215, 75));
        sf::FloatRect cpb = clickPrompt.getLocalBounds();
        clickPrompt.setPosition(startX + contentW - cpb.width - 8.f, curY + 28.f);
        window.draw(clickPrompt);

        registerButton(rBox.getGlobalBounds(), [this, ruler]() {
            inspectCharacter(ruler->id);
        });
    }
    curY += 56.f;

    sf::RectangleShape resBox(sf::Vector2f(contentW, 44.f));
    resBox.setPosition(startX, curY);
    resBox.setFillColor(sf::Color(16, 12, 9));
    resBox.setOutlineColor(sf::Color(115, 80, 40));
    resBox.setOutlineThickness(1.f);
    window.draw(resBox);

    sf::Text resH("COUNTY STOCKPILES", *font, 9);
    resH.setStyle(sf::Text::Bold);
    resH.setFillColor(sf::Color(190, 155, 75));
    resH.setPosition(startX + 8.f, curY + 5.f);
    window.draw(resH);

    std::string resStr = "Food: " + std::to_string(v->food) + "  Wood: " + std::to_string(v->wood) + "  Stone: " + std::to_string(v->stone) + "  Amber: " + std::to_string(v->amber);
    sf::Text resVals(resStr, *font, 10);
    resVals.setStyle(sf::Text::Bold);
    resVals.setFillColor(sf::Color(235, 225, 205));
    resVals.setPosition(startX + 8.f, curY + 22.f);
    window.draw(resVals);
    curY += 54.f;

    auto drawField = [&](const std::string& label, const std::string& val, sf::Color valCol = sf::Color(240, 230, 210)) {
        sf::Text l(label, *font, 9);
        l.setFillColor(sf::Color(165, 145, 120));
        l.setPosition(startX, curY);
        window.draw(l);
        curY += 12.f;

        drawWrappedText(window, val, startX, curY, contentW, 11, valCol, true);
        curY += 6.f;
    };

    drawField("Settlement Development", "Tier " + std::to_string(static_cast<int>(v->tier)) + " Fortress Center", sf::Color(245, 205, 75));
    drawField("Total Inhabitants", std::to_string(v->members.size()) + " clan apes");
    drawField("Constructed Structures", std::to_string(v->finishedStructures.size()) + " buildings");
}

void ProfileViewManager::drawKingdomProfile(sf::RenderWindow& window, sim::KingdomID kId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::KingdomData* k = reg.getKingdom(kId);
    if (!k) return;

    drawHeader(window, "KINGDOM OF " + k->name, "Realm Capital Seat & Sovereignty", k->color);

    float startX = profilePanelPos.x + 12.f;
    float curY = profilePanelPos.y + 48.f;
    float contentW = panelWidth - 24.f;

    sim::ApeData* ruler = reg.getApe(k->currentKingId);
    std::string rulerName = ruler ? ruler->name : "Monarch";

    sf::RectangleShape kBox(sf::Vector2f(contentW, 46.f));
    kBox.setPosition(startX, curY);
    kBox.setFillColor(sf::Color(36, 24, 16));
    kBox.setOutlineColor(sf::Color(215, 165, 60));
    kBox.setOutlineThickness(1.2f);
    window.draw(kBox);

    sf::Text rH("SOVEREIGN MONARCH", *font, 9);
    rH.setStyle(sf::Text::Bold);
    rH.setFillColor(sf::Color(210, 170, 75));
    rH.setPosition(startX + 8.f, curY + 5.f);
    window.draw(rH);

    sf::Text rN(rulerName, *font, 12);
    rN.setStyle(sf::Text::Bold);
    rN.setFillColor(sf::Color(255, 240, 200));
    rN.setPosition(startX + 8.f, curY + 18.f);
    window.draw(rN);

    if (ruler) {
        sf::Text clickPrompt("[Inspect Monarch ->]", *font, 9);
        clickPrompt.setFillColor(sf::Color(255, 215, 75));
        sf::FloatRect cpb = clickPrompt.getLocalBounds();
        clickPrompt.setPosition(startX + contentW - cpb.width - 8.f, curY + 28.f);
        window.draw(clickPrompt);

        registerButton(kBox.getGlobalBounds(), [this, ruler]() {
            inspectCharacter(ruler->id);
        });
    }
    curY += 56.f;

    sf::RectangleShape resBox(sf::Vector2f(contentW, 44.f));
    resBox.setPosition(startX, curY);
    resBox.setFillColor(sf::Color(16, 12, 9));
    resBox.setOutlineColor(sf::Color(115, 80, 40));
    resBox.setOutlineThickness(1.f);
    window.draw(resBox);

    sf::Text resH("ROYAL TREASURY", *font, 9);
    resH.setStyle(sf::Text::Bold);
    resH.setFillColor(sf::Color(190, 155, 75));
    resH.setPosition(startX + 8.f, curY + 5.f);
    window.draw(resH);

    std::string resContent = "Food: " + std::to_string(k->treasuryFood) + "  Wood: " + std::to_string(k->treasuryWood) + "  Stone: " + std::to_string(k->treasuryStone);
    sf::Text resVals(resContent, *font, 10);
    resVals.setStyle(sf::Text::Bold);
    resVals.setFillColor(sf::Color(235, 225, 205));
    resVals.setPosition(startX + 8.f, curY + 22.f);
    window.draw(resVals);
    curY += 54.f;

    auto drawField = [&](const std::string& label, const std::string& val, sf::Color valCol = sf::Color(240, 230, 210), bool bold = false) {
        sf::Text l(label, *font, 9);
        l.setFillColor(sf::Color(165, 145, 120));
        l.setPosition(startX, curY);
        window.draw(l);
        curY += 12.f;

        drawWrappedText(window, val, startX, curY, contentW, 11, valCol, bold);
        curY += 6.f;
    };

    sim::VillageData* cap = reg.getVillage(k->capitalVillageId);
    std::string capName = cap ? cap->name : "Capital";
    drawField("Capital County", capName);
    drawField("Realm Strength", std::to_string(k->population) + " subjects | " + std::to_string(k->controlledVillages.size()) + " counties");
    drawField("Army Mobilization", std::to_string(k->militaryStrength) + " warriors", sf::Color(245, 130, 130), true);

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
                default: break;
            }
        }
    } else if (pApe && pApe->currentKingdom == kId) {
        relStr = "Your Realm";
        relCol = sf::Color(255, 215, 60);
    }
    drawField("Diplomatic Status", relStr, relCol, true);
}