#include "ui/GameHUD.h"
#include "simulation/SimulationManager.h"
#include <cmath>
#include <algorithm>
#include <string>

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

void GameHUD::update(float dt, const sim::ApeData* playerApe) {
    if (amberPulseTimer > 0.f) {
        amberPulseTimer -= dt;
        if (amberPulseTimer < 0.f) amberPulseTimer = 0.f;
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

    drawOrnatePanel(window, 8.f, panelY, 110.f, panelH);
    sf::Text titleText("Ape Dynasty", *font, 12);
    titleText.setStyle(sf::Text::Bold);
    titleText.setFillColor(sf::Color(230, 195, 125));
    titleText.setOutlineColor(sf::Color(15, 10, 5));
    titleText.setOutlineThickness(1.2f);
    sf::FloatRect ttb = titleText.getLocalBounds();
    titleText.setOrigin(0.f, ttb.top + ttb.height * 0.5f);
    titleText.setPosition(16.f, centerY);
    window.draw(titleText);

    sim::SimulationRegistry& reg = simManager->getRegistry();
    const sim::VillageData* village = playerApe ? reg.getVillage(playerApe->villageId) : nullptr;

    float resPanelX = 124.f;
    float resPanelW = 390.f;
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
    int maxAmber = playerApe ? playerApe->maxAmber : 100;
    sf::Text amberTxt(std::to_string(curAmber) + "/" + std::to_string(maxAmber), *font, 11);
    amberTxt.setFillColor(sf::Color(245, 220, 145));
    amberTxt.setOutlineColor(sf::Color::Black);
    amberTxt.setOutlineThickness(1.f);
    sf::FloatRect ab = amberTxt.getLocalBounds();
    amberTxt.setOrigin(0.f, ab.top + ab.height * 0.5f);
    amberTxt.setPosition(itemX + 10.f, centerY);
    window.draw(amberTxt);

    itemX += 80.f;

    auto drawBanana = [&](float bx, float by) {
        sf::ConvexShape banana(5);
        banana.setPoint(0, sf::Vector2f(0.f, 4.f));
        banana.setPoint(1, sf::Vector2f(6.f, 6.5f));
        banana.setPoint(2, sf::Vector2f(11.f, 0.f));
        banana.setPoint(3, sf::Vector2f(8.f, 3.5f));
        banana.setPoint(4, sf::Vector2f(2.f, 1.5f));
        banana.setPosition(bx, by - 3.f);
        banana.setFillColor(sf::Color(245, 205, 45));
        banana.setOutlineColor(sf::Color(115, 75, 15));
        banana.setOutlineThickness(0.8f);
        window.draw(banana);

        sf::RectangleShape stem(sf::Vector2f(2.f, 2.f));
        stem.setPosition(bx, by + 1.f);
        stem.setFillColor(sf::Color(65, 40, 15));
        window.draw(stem);
    };

    drawBanana(itemX, centerY);
    int foodVal = village ? village->food : 0;
    sf::Text foodTxt(std::to_string(foodVal), *font, 11);
    foodTxt.setFillColor(sf::Color(235, 225, 205));
    foodTxt.setOutlineColor(sf::Color::Black);
    foodTxt.setOutlineThickness(1.f);
    sf::FloatRect fb = foodTxt.getLocalBounds();
    foodTxt.setOrigin(0.f, fb.top + fb.height * 0.5f);
    foodTxt.setPosition(itemX + 16.f, centerY);
    window.draw(foodTxt);

    sf::Text foodRate("+8", *font, 10);
    foodRate.setFillColor(sf::Color(55, 195, 65));
    foodRate.setOutlineColor(sf::Color::Black);
    foodRate.setOutlineThickness(1.f);
    sf::FloatRect frb = foodRate.getLocalBounds();
    foodRate.setOrigin(0.f, frb.top + frb.height * 0.5f);
    foodRate.setPosition(foodTxt.getPosition().x + fb.width + 5.f, centerY);
    window.draw(foodRate);

    itemX += 88.f;

    auto drawLog = [&](float lx, float ly) {
        sf::RectangleShape logBody(sf::Vector2f(12.f, 7.f));
        logBody.setOrigin(6.f, 3.5f);
        logBody.setPosition(lx + 5.f, ly);
        logBody.setFillColor(sf::Color(135, 85, 45));
        logBody.setOutlineColor(sf::Color(45, 25, 12));
        logBody.setOutlineThickness(0.8f);
        window.draw(logBody);

        sf::CircleShape logRing(3.f);
        logRing.setOrigin(3.f, 3.f);
        logRing.setPosition(lx + 10.f, ly);
        logRing.setFillColor(sf::Color(185, 130, 80));
        logRing.setOutlineColor(sf::Color(60, 32, 16));
        logRing.setOutlineThickness(0.6f);
        window.draw(logRing);
    };

    drawLog(itemX, centerY);
    int woodVal = village ? village->wood : 0;
    sf::Text woodTxt(std::to_string(woodVal), *font, 11);
    woodTxt.setFillColor(sf::Color(235, 225, 205));
    woodTxt.setOutlineColor(sf::Color::Black);
    woodTxt.setOutlineThickness(1.f);
    sf::FloatRect wb = woodTxt.getLocalBounds();
    woodTxt.setOrigin(0.f, wb.top + wb.height * 0.5f);
    woodTxt.setPosition(itemX + 16.f, centerY);
    window.draw(woodTxt);

    sf::Text woodRate("+5", *font, 10);
    woodRate.setFillColor(sf::Color(55, 195, 65));
    woodRate.setOutlineColor(sf::Color::Black);
    woodRate.setOutlineThickness(1.f);
    sf::FloatRect wrb = woodRate.getLocalBounds();
    woodRate.setOrigin(0.f, wrb.top + wrb.height * 0.5f);
    woodRate.setPosition(woodTxt.getPosition().x + wb.width + 5.f, centerY);
    window.draw(woodRate);

    itemX += 88.f;

    auto drawStone = [&](float sx, float sy) {
        sf::CircleShape r1(3.8f);
        r1.setPosition(sx, sy - 3.5f);
        r1.setFillColor(sf::Color(145, 145, 150));
        r1.setOutlineColor(sf::Color(45, 45, 50));
        r1.setOutlineThickness(0.7f);
        window.draw(r1);

        sf::CircleShape r2(4.8f);
        r2.setPosition(sx + 3.5f, sy - 1.5f);
        r2.setFillColor(sf::Color(120, 120, 125));
        r2.setOutlineColor(sf::Color(40, 40, 45));
        r2.setOutlineThickness(0.7f);
        window.draw(r2);
    };

    drawStone(itemX, centerY);
    int stoneVal = village ? village->stone : 0;
    sf::Text stoneTxt(std::to_string(stoneVal), *font, 11);
    stoneTxt.setFillColor(sf::Color(235, 225, 205));
    stoneTxt.setOutlineColor(sf::Color::Black);
    stoneTxt.setOutlineThickness(1.f);
    sf::FloatRect sb = stoneTxt.getLocalBounds();
    stoneTxt.setOrigin(0.f, sb.top + sb.height * 0.5f);
    stoneTxt.setPosition(itemX + 16.f, centerY);
    window.draw(stoneTxt);

    sf::Text stoneRate("+3", *font, 10);
    stoneRate.setFillColor(sf::Color(55, 195, 65));
    stoneRate.setOutlineColor(sf::Color::Black);
    stoneRate.setOutlineThickness(1.f);
    sf::FloatRect srb = stoneRate.getLocalBounds();
    stoneRate.setOrigin(0.f, srb.top + srb.height * 0.5f);
    stoneRate.setPosition(stoneTxt.getPosition().x + sb.width + 5.f, centerY);
    window.draw(stoneRate);

    float popPanelX = 520.f;
    float popPanelW = 96.f;
    drawOrnatePanel(window, popPanelX, panelY, popPanelW, panelH);

    float apeIconX = popPanelX + 13.f;
    sf::CircleShape apeHead(4.5f);
    apeHead.setOrigin(4.5f, 4.5f);
    apeHead.setPosition(apeIconX, centerY - 1.f);
    apeHead.setFillColor(sf::Color(140, 95, 50));
    apeHead.setOutlineColor(sf::Color(35, 20, 10));
    apeHead.setOutlineThickness(0.8f);
    window.draw(apeHead);

    sf::CircleShape apeMuzzle(3.f);
    apeMuzzle.setOrigin(3.f, 3.f);
    apeMuzzle.setPosition(apeIconX, centerY + 1.5f);
    apeMuzzle.setFillColor(sf::Color(205, 160, 115));
    window.draw(apeMuzzle);

    int curApes = village ? static_cast<int>(village->members.size()) : 1;
    int maxApes = 60;
    sf::Text popTxt(std::to_string(curApes) + " / " + std::to_string(maxApes), *font, 11);
    popTxt.setFillColor(sf::Color(235, 225, 205));
    popTxt.setOutlineColor(sf::Color::Black);
    popTxt.setOutlineThickness(1.f);
    sf::FloatRect pb = popTxt.getLocalBounds();
    popTxt.setOrigin(0.f, pb.top + pb.height * 0.5f);
    popTxt.setPosition(popPanelX + 23.f, centerY);
    window.draw(popTxt);

    int year = reg.getYear();
    int day = simManager->getClock().getDays();
    int seasonIdx = static_cast<int>(reg.getSeason());
    static const std::string seasonNames[] = { "Spring", "Summer", "Autumn", "Winter" };
    std::string sName = (seasonIdx >= 0 && seasonIdx < 4) ? seasonNames[seasonIdx] : "Era";

    float datePanelX = 890.f;
    float datePanelW = 205.f;
    drawOrnatePanel(window, datePanelX, panelY, datePanelW, panelH);

    std::string dateStr = "Year " + std::to_string(year) + "   Day " + std::to_string(day) + "   " + sName;
    sf::Text dateText(dateStr, *font, 11);
    dateText.setFillColor(sf::Color(245, 225, 175));
    dateText.setOutlineColor(sf::Color(10, 8, 5));
    dateText.setOutlineThickness(1.f);
    sf::FloatRect dtb = dateText.getLocalBounds();
    dateText.setOrigin(dtb.left + dtb.width * 0.5f, dtb.top + dtb.height * 0.5f);
    dateText.setPosition(datePanelX + datePanelW * 0.5f, centerY);
    window.draw(dateText);

    float celX = 1101.f;
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

    float btnStartX = 1131.f;
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
}