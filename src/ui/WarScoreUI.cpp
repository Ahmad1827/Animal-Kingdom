#include "ui/WarScoreUI.h"
#include "simulation/WarfareManager.h"
#include <cmath>
#include <algorithm>

WarScoreUI::WarScoreUI() {
    warBadgeBounds = sf::FloatRect(1030.f, 638.f, 210.f, 48.f);
}

void WarScoreUI::init(const sf::Font& f) {
    font = &f;

    lensButtons.clear();
    float startX = 405.f;
    float startY = 665.f;
    float btnW = 92.f;
    float btnH = 26.f;
    float spacing = 6.f;

    lensButtons.push_back({MapLensMode::DeFacto, "De Facto", "Q", sf::FloatRect(startX + 0 * (btnW + spacing), startY, btnW, btnH)});
    lensButtons.push_back({MapLensMode::DeJure, "De Jure", "W", sf::FloatRect(startX + 1 * (btnW + spacing), startY, btnW, btnH)});
    lensButtons.push_back({MapLensMode::Vassals, "Vassals", "E", sf::FloatRect(startX + 2 * (btnW + spacing), startY, btnW, btnH)});
    lensButtons.push_back({MapLensMode::Diplomacy, "Diplomacy", "R", sf::FloatRect(startX + 3 * (btnW + spacing), startY, btnW, btnH)});
    lensButtons.push_back({MapLensMode::Economy, "Economy", "T", sf::FloatRect(startX + 4 * (btnW + spacing), startY, btnW, btnH)});
}

void WarScoreUI::update(float dt) {
    pulseTime += dt;
}

bool WarScoreUI::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId) {
    sim::ApeData* playerApe = reg.getApe(controlledApeId);
    sim::KingdomID playerKId = playerApe ? playerApe->currentKingdom : 0;
    sim::ActiveWar* playerWar = reg.getActiveWarForKingdom(playerKId);

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Q) { currentLens = MapLensMode::DeFacto; return true; }
        if (event.key.code == sf::Keyboard::W) { currentLens = MapLensMode::DeJure; return true; }
        if (event.key.code == sf::Keyboard::E) { currentLens = MapLensMode::Vassals; return true; }
        if (event.key.code == sf::Keyboard::R) { currentLens = MapLensMode::Diplomacy; return true; }
        if (event.key.code == sf::Keyboard::T) { currentLens = MapLensMode::Economy; return true; }
        if (event.key.code == sf::Keyboard::Escape && peaceModalOpen) {
            peaceModalOpen = false;
            return true;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), letterboxView);

        for (const auto& btn : lensButtons) {
            if (btn.bounds.contains(mPos)) {
                currentLens = btn.mode;
                return true;
            }
        }

        if (playerWar && warBadgeBounds.contains(mPos)) {
            peaceModalOpen = !peaceModalOpen;
            activeWarId = playerWar->id;
            return true;
        }

        if (peaceModalOpen && playerWar) {
            if (closePeaceBtnBounds.contains(mPos)) {
                peaceModalOpen = false;
                return true;
            }

            bool isAttacker = (playerWar->attackerKingdom == playerKId);
            float scoreForPlayer = isAttacker ? playerWar->warScore : -playerWar->warScore;

            if (enforceBtnBounds.contains(mPos)) {
                if (scoreForPlayer >= 75.f) {
                    if (isAttacker) sim::WarfareManager::enforceDemands(reg, playerWar->id);
                    else sim::WarfareManager::surrenderWar(reg, playerWar->id, playerWar->attackerKingdom);
                    peaceModalOpen = false;
                    return true;
                }
            }

            if (whitePeaceBtnBounds.contains(mPos)) {
                if (scoreForPlayer >= -25.f && scoreForPlayer <= 50.f) {
                    sim::WarfareManager::signWhitePeace(reg, playerWar->id);
                    peaceModalOpen = false;
                    return true;
                }
            }

            if (surrenderBtnBounds.contains(mPos)) {
                sim::WarfareManager::surrenderWar(reg, playerWar->id, playerKId);
                peaceModalOpen = false;
                return true;
            }

            sf::FloatRect modalBack(400.f, 210.f, 480.f, 310.f);
            if (modalBack.contains(mPos)) {
                return true;
            }
        }
    }

    return false;
}

void WarScoreUI::draw(sf::RenderWindow& window, sim::SimulationRegistry& reg, sim::EntityID controlledApeId, bool isExpandedMap) {
    if (!font) return;

    if (isExpandedMap) {
        sf::RectangleShape barBg(sf::Vector2f(495.f, 32.f));
        barBg.setPosition(400.f, 662.f);
        barBg.setFillColor(sf::Color(18, 13, 9, 235));
        barBg.setOutlineColor(sf::Color(165, 125, 60));
        barBg.setOutlineThickness(1.2f);
        window.draw(barBg);

        for (const auto& btn : lensButtons) {
            bool selected = (btn.mode == currentLens);

            sf::RectangleShape b(sf::Vector2f(btn.bounds.width, btn.bounds.height));
            b.setPosition(btn.bounds.left, btn.bounds.top);
            b.setFillColor(selected ? sf::Color(75, 52, 28, 255) : sf::Color(32, 22, 16, 210));
            b.setOutlineColor(selected ? sf::Color(245, 205, 80) : sf::Color(105, 80, 50));
            b.setOutlineThickness(1.f);
            window.draw(b);

            sf::Text keyTxt("[" + btn.shortcutKey + "]", *font, 9);
            keyTxt.setStyle(sf::Text::Bold);
            keyTxt.setFillColor(selected ? sf::Color(255, 230, 140) : sf::Color(180, 160, 130));
            keyTxt.setPosition(btn.bounds.left + 5.f, btn.bounds.top + 6.f);
            window.draw(keyTxt);

            sf::Text lTxt(btn.label, *font, 10);
            lTxt.setStyle(sf::Text::Bold);
            lTxt.setFillColor(selected ? sf::Color::White : sf::Color(210, 195, 175));
            lTxt.setPosition(btn.bounds.left + 26.f, btn.bounds.top + 5.f);
            window.draw(lTxt);
        }
    }

    sim::ApeData* playerApe = reg.getApe(controlledApeId);
    sim::KingdomID playerKId = playerApe ? playerApe->currentKingdom : 0;
    sim::ActiveWar* war = reg.getActiveWarForKingdom(playerKId);

    if (!war || war->isResolved) {
        peaceModalOpen = false;
        return;
    }

    bool isAttacker = (war->attackerKingdom == playerKId);
    float playerWarScore = isAttacker ? war->warScore : -war->warScore;

    sf::RectangleShape badge(sf::Vector2f(warBadgeBounds.width, warBadgeBounds.height));
    badge.setPosition(warBadgeBounds.left, warBadgeBounds.top);
    badge.setFillColor(sf::Color(22, 14, 10, 245));
    badge.setOutlineColor(playerWarScore >= 0.f ? sf::Color(230, 185, 60) : sf::Color(210, 45, 35));
    badge.setOutlineThickness(1.5f);
    window.draw(badge);

    float pulse = 0.5f + 0.5f * std::sin(pulseTime * 6.f);
    sf::CircleShape pip(5.f);
    pip.setOrigin(5.f, 5.f);
    pip.setPosition(warBadgeBounds.left + 16.f, warBadgeBounds.top + 24.f);
    pip.setFillColor(sf::Color(220, 35, 25, static_cast<sf::Uint8>(180 + pulse * 75)));
    pip.setOutlineColor(sf::Color::Black);
    pip.setOutlineThickness(1.f);
    window.draw(pip);

    sim::KingdomData* oppK = reg.getKingdom(isAttacker ? war->defenderKingdom : war->attackerKingdom);
    std::string oppName = oppK ? oppK->name : "Enemy";

    sf::Text warLabel("WAR WITH " + oppName, *font, 9);
    warLabel.setStyle(sf::Text::Bold);
    warLabel.setFillColor(sf::Color(245, 220, 160));
    warLabel.setPosition(warBadgeBounds.left + 28.f, warBadgeBounds.top + 7.f);
    window.draw(warLabel);

    std::string scoreStr = (playerWarScore > 0.f ? "+" : "") + std::to_string(static_cast<int>(std::round(playerWarScore))) + "%";
    sf::Text scoreTxt(scoreStr, *font, 14);
    scoreTxt.setStyle(sf::Text::Bold);
    scoreTxt.setFillColor(playerWarScore >= 0.f ? sf::Color(90, 235, 90) : sf::Color(245, 75, 65));
    scoreTxt.setPosition(warBadgeBounds.left + 28.f, warBadgeBounds.top + 22.f);
    window.draw(scoreTxt);

    sf::Text actPrompt("[Negotiate Peace]", *font, 9);
    actPrompt.setFillColor(sf::Color(190, 175, 135));
    actPrompt.setPosition(warBadgeBounds.left + 95.f, warBadgeBounds.top + 26.f);
    window.draw(actPrompt);

    if (peaceModalOpen) {
        sf::FloatRect mR(390.f, 190.f, 500.f, 335.f);

        sf::RectangleShape shadow(sf::Vector2f(mR.width + 6.f, mR.height + 6.f));
        shadow.setPosition(mR.left + 3.f, mR.top + 3.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 195));
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

        sf::Text title("PEACE TREATY NEGOTIATIONS", *font, 12);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(255, 230, 140));
        title.setPosition(mR.left + 14.f, mR.top + 8.f);
        window.draw(title);

        closePeaceBtnBounds = sf::FloatRect(mR.left + mR.width - 26.f, mR.top + 6.f, 20.f, 20.f);
        sf::RectangleShape closeBtn(sf::Vector2f(20.f, 20.f));
        closeBtn.setPosition(closePeaceBtnBounds.left, closePeaceBtnBounds.top);
        closeBtn.setFillColor(sf::Color(140, 25, 20));
        closeBtn.setOutlineColor(sf::Color(240, 200, 75));
        closeBtn.setOutlineThickness(1.f);
        window.draw(closeBtn);

        sf::Text xT("x", *font, 12);
        xT.setStyle(sf::Text::Bold);
        xT.setFillColor(sf::Color::White);
        xT.setPosition(closePeaceBtnBounds.left + 6.f, closePeaceBtnBounds.top + 1.f);
        window.draw(xT);

        sim::VillageData* goalV = reg.getVillage(war->targetVillageId);
        std::string goalName = goalV ? goalV->name : "Target County";

        float curY = mR.top + 45.f;
        sf::Text sub("Casus Belli: " + war->casusBelli + " (" + goalName + ")", *font, 10);
        sub.setFillColor(sf::Color(195, 180, 150));
        sub.setPosition(mR.left + 16.f, curY);
        window.draw(sub);
        curY += 20.f;

        sf::RectangleShape breakdownBox(sf::Vector2f(mR.width - 32.f, 54.f));
        breakdownBox.setPosition(mR.left + 16.f, curY);
        breakdownBox.setFillColor(sf::Color(14, 10, 8, 235));
        breakdownBox.setOutlineColor(sf::Color(90, 65, 40));
        breakdownBox.setOutlineThickness(1.f);
        window.draw(breakdownBox);

        sf::Text scH("Current War Score: " + scoreStr, *font, 12);
        scH.setStyle(sf::Text::Bold);
        scH.setFillColor(playerWarScore >= 0.f ? sf::Color(105, 240, 105) : sf::Color(245, 80, 70));
        scH.setPosition(breakdownBox.getPosition().x + 8.f, breakdownBox.getPosition().y + 6.f);
        window.draw(scH);

        std::string bDetails = "Battles: " + std::to_string(static_cast<int>(isAttacker ? war->battleScore : -war->battleScore)) +
                               "% | Occupations: " + std::to_string(static_cast<int>(isAttacker ? war->occupationScore : -war->occupationScore)) +
                               "% | Ticking: " + std::to_string(static_cast<int>(isAttacker ? war->tickingScore : -war->tickingScore)) + "%";
        sf::Text dtTxt(bDetails, *font, 9);
        dtTxt.setFillColor(sf::Color(185, 170, 140));
        dtTxt.setPosition(breakdownBox.getPosition().x + 8.f, breakdownBox.getPosition().y + 28.f);
        window.draw(dtTxt);
        curY += 68.f;

        auto drawPeaceOption = [&](sf::FloatRect& bounds, const std::string& optTitle, const std::string& desc, bool enabled, sf::Color col) {
            bounds = sf::FloatRect(mR.left + 16.f, curY, mR.width - 32.f, 44.f);

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

            sf::Text oT(optTitle, *font, 11);
            oT.setStyle(sf::Text::Bold);
            oT.setFillColor(enabled ? sf::Color::White : sf::Color(130, 120, 110));
            oT.setPosition(bounds.left + 12.f, bounds.top + 4.f);
            window.draw(oT);

            sf::Text oD(desc, *font, 9);
            oD.setStyle(sf::Text::Italic);
            oD.setFillColor(enabled ? sf::Color(180, 165, 140) : sf::Color(100, 95, 90));
            oD.setPosition(bounds.left + 12.f, bounds.top + 22.f);
            window.draw(oD);

            curY += 49.f;
        };

        bool canEnforce = (playerWarScore >= 75.f);
        drawPeaceOption(enforceBtnBounds, "1. Enforce Demands",
                        canEnforce ? "Conquer target county, demand reparations, seal 5-year truce." : "Requires at least +75% War Score.",
                        canEnforce, sf::Color(90, 225, 90));

        bool canWhitePeace = (playerWarScore >= -25.f && playerWarScore <= 50.f);
        drawPeaceOption(whitePeaceBtnBounds, "2. White Peace",
                        canWhitePeace ? "Status quo ante bellum. Borders remain unchanged. 3-year truce." : "Requires contested score (-25% to +50%).",
                        canWhitePeace, sf::Color(225, 195, 75));

        drawPeaceOption(surrenderBtnBounds, "3. Unconditional Surrender",
                        "Cede claimed territories and pay humiliating reparations.",
                        true, sf::Color(235, 65, 65));
    }
}