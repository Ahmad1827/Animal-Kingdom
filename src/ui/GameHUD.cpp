#include "ui/GameHUD.h"
#include "simulation/SimulationManager.h"
#include "world/SettlementSystem.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <iomanip>
#include <sstream>

GameHUD::GameHUD()
    : font(nullptr), gameSpeed(1), previousGameSpeed(1), isGamePaused(false),
      amberPulseTimer(0.f), lastObservedAmber(-1) {}

void GameHUD::init(const sf::Font& f) {
    font = &f;
}

float GameHUD::getSpeedMultiplier() const {
    if (isGamePaused) return 0.0f;
    switch (gameSpeed) {
        case 1: return 1.0f;
        case 2: return 2.0f;
        case 3: return 5.0f;
        case 4: return 15.0f;
        case 5: return 60.0f;
        default: return 1.0f;
    }
}

void GameHUD::setGameSpeed(int speed) {
    gameSpeed = std::clamp(speed, 1, 5);
    previousGameSpeed = gameSpeed;
    isGamePaused = false;
}

void GameHUD::toggleGamePause() {
    isGamePaused = !isGamePaused;
    if (!isGamePaused && gameSpeed == 0) {
        gameSpeed = (previousGameSpeed >= 1) ? previousGameSpeed : 1;
    }
}

void GameHUD::update(float dt, sim::ApeData* playerApe, sim::SimulationRegistry& reg) {
    if (amberPulseTimer > 0.f) {
        amberPulseTimer -= dt;
        if (amberPulseTimer < 0.f) amberPulseTimer = 0.f;
    }

    if (playerApe) {
        playerApe->maxAmber = 99999;
    }

    int demesneCount = std::max(1, SettlementSystem::getKingdomDemesneCount("Wessex"));
    float tradeInc = SettlementSystem::getKingdomTradeIncome("Wessex");
    float upkeep = SettlementSystem::getKingdomArmyUpkeep("Wessex");
    bool atWar = SettlementSystem::isWarActive();

    float taxMult = SettlementSystem::getAuthorityTaxMultiplier();
    float baseTaxes = (3.5f + static_cast<float>(demesneCount) * 2.2f) * taxMult;
    cachedAmberRate = baseTaxes + tradeInc - upkeep - (atWar ? 2.5f : 0.f);

    const auto& fState = SettlementSystem::getFactionState();
    bool disloyalVassals = !fState.memberCounties.empty();
    cachedPrestigeRate = 1.2f + (static_cast<float>(demesneCount) * 0.4f) + (disloyalVassals ? -0.8f : 0.5f);

    const auto* shamanMission = SettlementSystem::getCouncilAssignment(sim::CouncilRole::Shaman);
    bool shamanActive = (shamanMission && shamanMission->mission != CouncilMissionType::None);
    cachedPietyRate = 0.8f + (shamanActive ? 1.4f : 0.0f) + (atWar ? -0.3f : 0.2f);

    float simMult = getSpeedMultiplier();
    float dayStepTime = 0.35f;

    if (simMult > 0.f) {
        dayTickTimer += dt * simMult;
        while (dayTickTimer >= dayStepTime) {
            dayTickTimer -= dayStepTime;
            reg.advanceDays(1);
        }

        float deltaDays = (dt * simMult) / dayStepTime;
        float dailyAmber = cachedAmberRate / 30.0f;
        float dailyPrestige = cachedPrestigeRate / 30.0f;
        float dailyPiety = cachedPietyRate / 30.0f;

        amberAccumulator += dailyAmber * deltaDays;
        prestigeAccumulator += dailyPrestige * deltaDays;
        pietyAccumulator += dailyPiety * deltaDays;

        if (playerApe) {
            if (std::abs(amberAccumulator) >= 1.0f) {
                int d = static_cast<int>(amberAccumulator);
                playerApe->amberCount = std::clamp(playerApe->amberCount + d, -500, 99999);
                amberAccumulator -= static_cast<float>(d);
            }
            if (std::abs(prestigeAccumulator) >= 1.0f) {
                int d = static_cast<int>(prestigeAccumulator);
                playerApe->prestige = std::clamp(playerApe->prestige + d, 0, 99999);
                prestigeAccumulator -= static_cast<float>(d);
            }
            if (std::abs(pietyAccumulator) >= 1.0f) {
                int d = static_cast<int>(pietyAccumulator);
                playerApe->piety = std::clamp(playerApe->piety + d, 0, 99999);
                pietyAccumulator -= static_cast<float>(d);
            }
        }
    }

    if (playerApe) {
        if (lastObservedAmber != -1 && playerApe->amberCount != lastObservedAmber) {
            amberPulseTimer = 0.35f;
        }
        lastObservedAmber = playerApe->amberCount;
    }
}

bool GameHUD::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationManager* simManager) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i clickPixel(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f uiCoords = window.mapPixelToCoords(clickPixel, letterboxView);
        for (int i = 0; i < 6; ++i) {
            if (timeButtonBounds[i].contains(uiCoords)) {
                if (i == 0) toggleGamePause();
                else setGameSpeed(i);
                return true;
            }
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::P) {
            toggleGamePause();
            return true;
        } else if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1) {
            setGameSpeed(1);
            return true;
        } else if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) {
            setGameSpeed(2);
            return true;
        } else if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
            setGameSpeed(3);
            return true;
        } else if (event.key.code == sf::Keyboard::Num4 || event.key.code == sf::Keyboard::Numpad4) {
            setGameSpeed(4);
            return true;
        } else if (event.key.code == sf::Keyboard::Num5 || event.key.code == sf::Keyboard::Numpad5) {
            setGameSpeed(5);
            return true;
        } else if (event.key.code == sf::Keyboard::Add || event.key.code == sf::Keyboard::Equal) {
            if (isGamePaused) isGamePaused = false;
            else setGameSpeed(std::min(gameSpeed + 1, 5));
            return true;
        } else if (event.key.code == sf::Keyboard::Subtract || event.key.code == sf::Keyboard::Dash) {
            if (gameSpeed > 1) setGameSpeed(gameSpeed - 1);
            return true;
        }
    }
    return false;
}

void GameHUD::drawOrnatePanel(sf::RenderWindow& window, float x, float y, float w, float h) {
    sf::RectangleShape shadow(sf::Vector2f(w + 2.f, h + 2.f));
    shadow.setPosition(x + 1.f, y + 1.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(shadow);

    sf::RectangleShape panel(sf::Vector2f(w, h));
    panel.setPosition(x, y);
    panel.setFillColor(sf::Color(14, 10, 8, 235));
    panel.setOutlineColor(sf::Color(78, 56, 32));
    panel.setOutlineThickness(1.f);
    window.draw(panel);

    sf::RectangleShape innerBevel(sf::Vector2f(w - 4.f, h - 4.f));
    innerBevel.setPosition(x + 2.f, y + 2.f);
    innerBevel.setFillColor(sf::Color(22, 16, 12, 190));
    innerBevel.setOutlineColor(sf::Color(38, 26, 16));
    innerBevel.setOutlineThickness(1.f);
    window.draw(innerBevel);

    auto drawCornerPiece = [&](float cx, float cy) {
        sf::RectangleShape dot(sf::Vector2f(1.5f, 1.5f));
        dot.setPosition(cx, cy);
        dot.setFillColor(sf::Color(190, 150, 75));
        window.draw(dot);
    };

    drawCornerPiece(x + 2.f, y + 2.f);
    drawCornerPiece(x + w - 3.5f, y + 2.f);
    drawCornerPiece(x + 2.f, y + h - 3.5f);
    drawCornerPiece(x + w - 3.5f, y + h - 3.5f);
}

void GameHUD::draw(sf::RenderWindow& window, const sim::ApeData* playerApe, sim::SimulationManager* simManager) {
    if (!font || !simManager) return;

    float panelH = 24.f;
    float panelY = 6.f;
    float centerY = panelY + panelH * 0.5f;

    drawOrnatePanel(window, 8.f, panelY, 102.f, panelH);
    sf::Text titleText("WESSEX", *font, 11);
    titleText.setStyle(sf::Text::Bold);
    titleText.setFillColor(sf::Color(240, 210, 135));
    titleText.setOutlineColor(sf::Color(20, 12, 6));
    titleText.setOutlineThickness(1.2f);
    sf::FloatRect ttb = titleText.getLocalBounds();
    titleText.setOrigin(ttb.left + ttb.width * 0.5f, ttb.top + ttb.height * 0.5f);
    titleText.setPosition(8.f + 51.f, centerY);
    window.draw(titleText);

    sim::SimulationRegistry& reg = simManager->getRegistry();

    float resPanelX = 114.f;
    float resPanelW = 425.f;
    drawOrnatePanel(window, resPanelX, panelY, resPanelW, panelH);

    float pulseScale = 1.0f;
    if (amberPulseTimer > 0.f) {
        pulseScale = 1.0f + std::sin((amberPulseTimer / 0.35f) * 3.14159f) * 0.22f;
    }

    float itemX = resPanelX + 14.f;

    sf::ConvexShape diamond(4);
    diamond.setPoint(0, sf::Vector2f(0.f, -5.5f * pulseScale));
    diamond.setPoint(1, sf::Vector2f(5.f * pulseScale, 0.f));
    diamond.setPoint(2, sf::Vector2f(0.f, 5.5f * pulseScale));
    diamond.setPoint(3, sf::Vector2f(-5.f * pulseScale, 0.f));
    diamond.setPosition(itemX, centerY);
    diamond.setFillColor(sf::Color(245, 140, 20));
    diamond.setOutlineColor(sf::Color(90, 42, 8));
    diamond.setOutlineThickness(1.f);
    window.draw(diamond);

    sf::ConvexShape facet(3);
    facet.setPoint(0, sf::Vector2f(0.f, -5.5f * pulseScale));
    facet.setPoint(1, sf::Vector2f(5.f * pulseScale, 0.f));
    facet.setPoint(2, sf::Vector2f(-5.f * pulseScale, 0.f));
    facet.setPosition(itemX, centerY);
    facet.setFillColor(sf::Color(255, 215, 65, 210));
    window.draw(facet);

    int curAmber = playerApe ? playerApe->amberCount : 0;
    sf::Text amberTxt(std::to_string(curAmber), *font, 11);
    amberTxt.setStyle(sf::Text::Bold);
    amberTxt.setFillColor(curAmber >= 0 ? sf::Color(245, 225, 160) : sf::Color(245, 80, 70));
    sf::FloatRect ab = amberTxt.getLocalBounds();
    amberTxt.setOrigin(0.f, ab.top + ab.height * 0.5f);
    amberTxt.setPosition(itemX + 9.f, centerY);
    window.draw(amberTxt);

    std::ostringstream aStream;
    aStream << std::fixed << std::setprecision(1) << (cachedAmberRate >= 0.f ? "+" : "") << cachedAmberRate;
    sf::Text aRateTxt(aStream.str(), *font, 9);
    aRateTxt.setStyle(sf::Text::Bold);
    aRateTxt.setFillColor(cachedAmberRate >= 0.f ? sf::Color(85, 215, 95) : sf::Color(245, 75, 65));
    sf::FloatRect arb = aRateTxt.getLocalBounds();
    aRateTxt.setOrigin(0.f, arb.top + arb.height * 0.5f);
    aRateTxt.setPosition(amberTxt.getPosition().x + ab.width + 5.f, centerY);
    window.draw(aRateTxt);

    itemX += 130.f;

    auto drawCrown = [&](float cx, float cy) {
        sf::ConvexShape crown(5);
        crown.setPoint(0, sf::Vector2f(-6.f, 4.f));
        crown.setPoint(1, sf::Vector2f(-7.f, -4.f));
        crown.setPoint(2, sf::Vector2f(0.f, 0.f));
        crown.setPoint(3, sf::Vector2f(7.f, -4.f));
        crown.setPoint(4, sf::Vector2f(6.f, 4.f));
        crown.setPosition(cx, cy);
        crown.setFillColor(sf::Color(240, 195, 55));
        crown.setOutlineColor(sf::Color(65, 38, 12));
        crown.setOutlineThickness(1.f);
        window.draw(crown);

        sf::CircleShape pip(1.5f);
        pip.setOrigin(1.5f, 1.5f);
        pip.setPosition(cx, cy + 2.f);
        pip.setFillColor(sf::Color(85, 175, 245));
        window.draw(pip);
    };

    drawCrown(itemX, centerY);
    int curPrestige = playerApe ? playerApe->prestige : 100;
    sf::Text prestTxt(std::to_string(curPrestige), *font, 11);
    prestTxt.setStyle(sf::Text::Bold);
    prestTxt.setFillColor(sf::Color(240, 225, 195));
    sf::FloatRect prb = prestTxt.getLocalBounds();
    prestTxt.setOrigin(0.f, prb.top + prb.height * 0.5f);
    prestTxt.setPosition(itemX + 11.f, centerY);
    window.draw(prestTxt);

    std::ostringstream pStream;
    pStream << std::fixed << std::setprecision(1) << (cachedPrestigeRate >= 0.f ? "+" : "") << cachedPrestigeRate;
    sf::Text pRateTxt(pStream.str(), *font, 9);
    pRateTxt.setStyle(sf::Text::Bold);
    pRateTxt.setFillColor(cachedPrestigeRate >= 0.f ? sf::Color(100, 195, 245) : sf::Color(245, 85, 75));
    sf::FloatRect prrb = pRateTxt.getLocalBounds();
    pRateTxt.setOrigin(0.f, prrb.top + prrb.height * 0.5f);
    pRateTxt.setPosition(prestTxt.getPosition().x + prb.width + 5.f, centerY);
    window.draw(pRateTxt);

    itemX += 135.f;

    auto drawFlame = [&](float fx, float cy) {
        sf::CircleShape aura(4.5f);
        aura.setOrigin(4.5f, 4.5f);
        aura.setPosition(fx, cy);
        aura.setFillColor(sf::Color(175, 120, 245, 180));
        window.draw(aura);

        sf::ConvexShape flame(4);
        flame.setPoint(0, sf::Vector2f(0.f, -5.5f));
        flame.setPoint(1, sf::Vector2f(3.5f, 1.5f));
        flame.setPoint(2, sf::Vector2f(0.f, 4.5f));
        flame.setPoint(3, sf::Vector2f(-3.5f, 1.5f));
        flame.setPosition(fx, cy);
        flame.setFillColor(sf::Color(220, 195, 255));
        flame.setOutlineColor(sf::Color(55, 30, 85));
        flame.setOutlineThickness(0.8f);
        window.draw(flame);
    };

    drawFlame(itemX, centerY);
    int curPiety = playerApe ? playerApe->piety : 50;
    sf::Text pietyTxt(std::to_string(curPiety), *font, 11);
    pietyTxt.setStyle(sf::Text::Bold);
    pietyTxt.setFillColor(sf::Color(240, 225, 250));
    sf::FloatRect pitb = pietyTxt.getLocalBounds();
    pietyTxt.setOrigin(0.f, pitb.top + pitb.height * 0.5f);
    pietyTxt.setPosition(itemX + 10.f, centerY);
    window.draw(pietyTxt);

    std::ostringstream piStream;
    piStream << std::fixed << std::setprecision(1) << (cachedPietyRate >= 0.f ? "+" : "") << cachedPietyRate;
    sf::Text piRateTxt(piStream.str(), *font, 9);
    piRateTxt.setStyle(sf::Text::Bold);
    piRateTxt.setFillColor(cachedPietyRate >= 0.f ? sf::Color(190, 150, 245) : sf::Color(245, 85, 75));
    sf::FloatRect pirb = piRateTxt.getLocalBounds();
    piRateTxt.setOrigin(0.f, pirb.top + pirb.height * 0.5f);
    piRateTxt.setPosition(pietyTxt.getPosition().x + pitb.width + 5.f, centerY);
    window.draw(piRateTxt);

    float realmPanelX = 544.f;
    float realmPanelW = 236.f;
    drawOrnatePanel(window, realmPanelX, panelY, realmPanelW, panelH);

    int demesneCount = SettlementSystem::getKingdomDemesneCount("Wessex");
    int maxDemesne = 4;

    float domIconX = realmPanelX + 12.f;
    sf::RectangleShape towerBase(sf::Vector2f(8.f, 7.f));
    towerBase.setOrigin(4.f, 0.f);
    towerBase.setPosition(domIconX, centerY - 2.f);
    towerBase.setFillColor(demesneCount <= maxDemesne ? sf::Color(165, 140, 95) : sf::Color(215, 65, 55));
    towerBase.setOutlineColor(sf::Color(35, 22, 12));
    towerBase.setOutlineThickness(0.8f);
    window.draw(towerBase);

    for (int cr = 0; cr < 3; ++cr) {
        sf::RectangleShape tooth(sf::Vector2f(2.f, 2.5f));
        tooth.setPosition(domIconX - 4.f + cr * 3.f, centerY - 4.5f);
        tooth.setFillColor(towerBase.getFillColor());
        window.draw(tooth);
    }

    sf::RectangleShape slit(sf::Vector2f(1.5f, 3.f));
    slit.setOrigin(0.75f, 1.5f);
    slit.setPosition(domIconX, centerY + 1.5f);
    slit.setFillColor(sf::Color(25, 15, 8));
    window.draw(slit);

    std::string domStr = "Domain: " + std::to_string(demesneCount) + "/" + std::to_string(maxDemesne);
    sf::Text domTxt(domStr, *font, 10);
    domTxt.setStyle(sf::Text::Bold);
    domTxt.setFillColor(demesneCount <= maxDemesne ? sf::Color(240, 225, 195) : sf::Color(255, 110, 100));
    sf::FloatRect db = domTxt.getLocalBounds();
    domTxt.setOrigin(0.f, db.top + db.height * 0.5f);
    domTxt.setPosition(domIconX + 8.f, centerY);
    window.draw(domTxt);

    int raisedTroops = SettlementSystem::getKingdomRaisedTroops("Wessex");
    int levyQuota = SettlementSystem::getAuthorityLevyPerCounty();
    int maxLevies = std::max(25, demesneCount * levyQuota);

    float swordX = realmPanelX + 124.f;
    sf::Color swCol = (raisedTroops > 0) ? sf::Color(245, 120, 50) : sf::Color(190, 175, 150);

    sf::Vertex sw1[] = {
        sf::Vertex(sf::Vector2f(swordX - 4.5f, centerY - 5.5f), swCol),
        sf::Vertex(sf::Vector2f(swordX + 4.5f, centerY + 5.5f), swCol)
    };
    sf::Vertex sw2[] = {
        sf::Vertex(sf::Vector2f(swordX + 4.5f, centerY - 5.5f), swCol),
        sf::Vertex(sf::Vector2f(swordX - 4.5f, centerY + 5.5f), swCol)
    };
    window.draw(sw1, 2, sf::Lines);
    window.draw(sw2, 2, sf::Lines);

    sf::CircleShape pommel(1.5f);
    pommel.setOrigin(1.5f, 1.5f);
    pommel.setPosition(swordX, centerY);
    pommel.setFillColor(sf::Color(235, 190, 60));
    window.draw(pommel);

    std::string levStr = (raisedTroops > 0) ? ("Levies: " + std::to_string(raisedTroops) + "/" + std::to_string(maxLevies))
                                           : ("Levies: " + std::to_string(maxLevies));
    sf::Text levTxt(levStr, *font, 10);
    levTxt.setStyle(sf::Text::Bold);
    levTxt.setFillColor(raisedTroops > 0 ? sf::Color(255, 195, 135) : sf::Color(220, 210, 190));
    sf::FloatRect lb = levTxt.getLocalBounds();
    levTxt.setOrigin(0.f, lb.top + lb.height * 0.5f);
    levTxt.setPosition(swordX + 9.f, centerY);
    window.draw(levTxt);

    float btnStartX = 926.f;
    float btnW = 23.f;
    float btnH = 24.f;
    float btnGap = 2.f;

    for (int i = 0; i < 6; ++i) {
        float bx = btnStartX + i * (btnW + btnGap);
        float by = panelY;
        timeButtonBounds[i] = sf::FloatRect(bx, by, btnW, btnH);

        bool isActive = (i == 0) ? isGamePaused : (!isGamePaused && gameSpeed == i);

        sf::RectangleShape shadow(sf::Vector2f(btnW + 2.f, btnH + 2.f));
        shadow.setPosition(bx + 1.f, by + 1.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(shadow);

        sf::RectangleShape btn(sf::Vector2f(btnW, btnH));
        btn.setPosition(bx, by);

        if (isActive) {
            btn.setFillColor(sf::Color(115, 75, 26));
            btn.setOutlineColor(sf::Color(235, 195, 75));
            btn.setOutlineThickness(1.f);
        } else {
            btn.setFillColor(sf::Color(18, 12, 9));
            btn.setOutlineColor(sf::Color(78, 54, 30));
            btn.setOutlineThickness(1.f);
        }
        window.draw(btn);

        sf::RectangleShape bevel(sf::Vector2f(btnW - 3.f, btnH - 3.f));
        bevel.setPosition(bx + 1.5f, by + 1.5f);
        bevel.setFillColor(isActive ? sf::Color(140, 92, 34) : sf::Color(26, 18, 13));
        window.draw(bevel);

        sf::Color iconCol = isActive ? sf::Color(255, 240, 185) : sf::Color(180, 150, 110);

        if (i == 0) {
            for (int p = 0; p < 2; ++p) {
                sf::RectangleShape bar(sf::Vector2f(2.8f, 9.f));
                bar.setPosition(bx + 6.8f + p * 6.f, by + 7.5f);
                bar.setFillColor(iconCol);
                window.draw(bar);
            }
        } else {
            int numArrows = i;
            float arrowW = (numArrows >= 4) ? 2.5f : ((numArrows == 3) ? 3.0f : 3.8f);
            float arrowH = (numArrows >= 4) ? 7.0f : ((numArrows == 3) ? 8.0f : 9.0f);
            float step = (numArrows >= 4) ? 3.2f : ((numArrows == 3) ? 4.0f : 5.0f);
            float totalW = arrowW + (numArrows - 1) * step;
            float arrowStartX = bx + (btnW - totalW) * 0.5f;
            float arrowCenterY = by + btnH * 0.5f;

            for (int a = 0; a < numArrows; ++a) {
                float ax = arrowStartX + a * step;
                sf::ConvexShape tri(3);
                tri.setPoint(0, sf::Vector2f(ax, arrowCenterY - arrowH * 0.5f));
                tri.setPoint(1, sf::Vector2f(ax + arrowW, arrowCenterY));
                tri.setPoint(2, sf::Vector2f(ax, arrowCenterY + arrowH * 0.5f));
                tri.setFillColor(iconCol);
                window.draw(tri);
            }
        }
    }

    float celX = 1082.f;
    float celW = 24.f;
    drawOrnatePanel(window, celX, panelY, celW, panelH);

    float timeOfDay = simManager->getClock().getTimeOfDay();
    float t24 = timeOfDay * 24.0f;
    bool isDay = (t24 >= 5.5f && t24 < 18.5f);

    if (isDay) {
        sf::CircleShape sun(4.f);
        sun.setOrigin(4.f, 4.f);
        sun.setPosition(celX + 12.f, centerY);
        sun.setFillColor(sf::Color(255, 210, 60));
        sun.setOutlineColor(sf::Color(90, 60, 15));
        sun.setOutlineThickness(0.7f);
        window.draw(sun);

        for (int a = 0; a < 8; ++a) {
            float rad = a * (3.14159f / 4.f);
            sf::RectangleShape ray(sf::Vector2f(2.f, 1.f));
            ray.setOrigin(0.f, 0.5f);
            ray.setPosition(celX + 12.f + std::cos(rad) * 5.5f, centerY + std::sin(rad) * 5.5f);
            ray.setRotation(a * 45.f);
            ray.setFillColor(sf::Color(255, 220, 85));
            window.draw(ray);
        }
    } else {
        sf::CircleShape moon(4.5f);
        moon.setOrigin(4.5f, 4.5f);
        moon.setPosition(celX + 12.f, centerY);
        moon.setFillColor(sf::Color(230, 235, 255));
        moon.setOutlineColor(sf::Color(50, 50, 75));
        moon.setOutlineThickness(0.7f);
        window.draw(moon);

        sf::CircleShape moonShade(3.8f);
        moonShade.setOrigin(3.8f, 3.8f);
        moonShade.setPosition(celX + 14.f, centerY - 1.2f);
        moonShade.setFillColor(sf::Color(22, 16, 12));
        window.draw(moonShade);
    }

    int year = reg.getYear();
    int month = reg.getMonth();
    int day = reg.getDay();
    std::string mName = reg.getMonthName(month);

    float datePanelX = 1112.f;
    float datePanelW = 160.f;
    drawOrnatePanel(window, datePanelX, panelY, datePanelW, panelH);

    std::string dateStr = std::to_string(day) + " " + mName + ", " + std::to_string(year);
    sf::Text dateText(dateStr, *font, 11);
    dateText.setStyle(sf::Text::Bold);
    dateText.setFillColor(sf::Color(245, 225, 175));
    dateText.setOutlineColor(sf::Color(10, 8, 5));
    dateText.setOutlineThickness(1.f);
    sf::FloatRect dtb = dateText.getLocalBounds();
    dateText.setOrigin(dtb.left + dtb.width * 0.5f, dtb.top + dtb.height * 0.5f);
    dateText.setPosition(datePanelX + datePanelW * 0.5f, centerY);
    window.draw(dateText);

    sf::Vector2i mPixel = sf::Mouse::getPosition(window);
    sf::Vector2f mCoords = window.mapPixelToCoords(mPixel);

    sf::FloatRect domainHitBox(realmPanelX + 4.f, panelY, 105.f, panelH);
    sf::FloatRect leviesHitBox(realmPanelX + 115.f, panelY, 115.f, panelH);

    auto drawHoverTooltip = [&](float anchorX, const std::string& title, const std::string& desc, const std::string& note, sf::Color titleCol) {
        sf::Text t1(title, *font, 10);
        t1.setStyle(sf::Text::Bold);
        t1.setFillColor(titleCol);

        sf::Text t2(desc, *font, 9);
        t2.setFillColor(sf::Color(225, 215, 195));

        sf::Text t3(note, *font, 8);
        t3.setStyle(sf::Text::Italic);
        t3.setFillColor(sf::Color(175, 160, 135));

        float tipW = std::max({ t1.getLocalBounds().width, t2.getLocalBounds().width, t3.getLocalBounds().width }) + 22.f;
        tipW = std::max(tipW, 230.f);
        float tipH = 46.f;
        float tipX = std::clamp(anchorX - tipW * 0.5f, 10.f, 1270.f - tipW);
        float tipY = panelY + panelH + 5.f;

        sf::RectangleShape tipShadow(sf::Vector2f(tipW + 4.f, tipH + 4.f));
        tipShadow.setPosition(tipX + 2.f, tipY + 2.f);
        tipShadow.setFillColor(sf::Color(0, 0, 0, 190));
        window.draw(tipShadow);

        sf::RectangleShape tipBox(sf::Vector2f(tipW, tipH));
        tipBox.setPosition(tipX, tipY);
        tipBox.setFillColor(sf::Color(18, 12, 9, 250));
        tipBox.setOutlineColor(titleCol);
        tipBox.setOutlineThickness(1.2f);
        window.draw(tipBox);

        t1.setPosition(tipX + 8.f, tipY + 4.f);
        window.draw(t1);
        t2.setPosition(tipX + 8.f, tipY + 18.f);
        window.draw(t2);
        t3.setPosition(tipX + 8.f, tipY + 31.f);
        window.draw(t3);
    };

    if (domainHitBox.contains(mCoords)) {
        drawHoverTooltip(domainHitBox.left + domainHitBox.width * 0.5f,
                         "DOMAIN LIMIT (" + std::to_string(demesneCount) + "/" + std::to_string(maxDemesne) + ")",
                         "Number of directly ruled counties in your demesne.",
                         "Exceeding limit penalizes vassal loyalty and tax income.",
                         sf::Color(245, 215, 120));
    } else if (leviesHitBox.contains(mCoords)) {
        int auth = SettlementSystem::getCrownAuthority();
        static const std::string authNames[] = { "Autonomous", "Limited", "High", "Absolute" };
        std::string authLabel = (auth >= 1 && auth <= 4) ? authNames[auth - 1] : "Standard";

        drawHoverTooltip(leviesHitBox.left + leviesHitBox.width * 0.5f,
                         "REALM LEVIES (" + std::to_string(raisedTroops) + "/" + std::to_string(maxLevies) + ")",
                         "Warrior muster pool (Crown Authority: " + authLabel + " - " + std::to_string(levyQuota) + "/county).",
                         "Armies raised in the field consume monthly upkeep.",
                         sf::Color(245, 140, 85));
    }
}