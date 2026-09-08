#include "ui/GameHUD.h"
#include "simulation/SimulationManager.h"
#include <cmath>
#include <algorithm>

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

void GameHUD::draw(sf::RenderWindow& window, const sim::ApeData* playerApe, sim::SimulationManager* simManager) {
    if (!font) return;
    drawAmberHUD(window, playerApe);
    drawTimeHUD(window, simManager);
}

void GameHUD::drawAmberHUD(sf::RenderWindow& window, const sim::ApeData* playerApe) {
    if (!playerApe) return;

    float startX = 32.f;
    float startY = 24.f;
    float boxW = 142.f;
    float boxH = 34.f;

    sf::RectangleShape dropShadow(sf::Vector2f(boxW, boxH));
    dropShadow.setPosition(startX + 2.f, startY + 2.f);
    dropShadow.setFillColor(sf::Color(0, 0, 0, 140));
    window.draw(dropShadow);

    sf::RectangleShape frame(sf::Vector2f(boxW, boxH));
    frame.setPosition(startX, startY);
    frame.setFillColor(sf::Color(26, 18, 14, 235));
    frame.setOutlineColor(sf::Color(140, 100, 48, 220));
    frame.setOutlineThickness(1.5f);
    window.draw(frame);

    auto drawCornerDot = [&](float cx, float cy) {
        sf::RectangleShape dot(sf::Vector2f(2.f, 2.f));
        dot.setPosition(cx, cy);
        dot.setFillColor(sf::Color(210, 165, 80));
        window.draw(dot);
    };
    drawCornerDot(startX + 2.f, startY + 2.f);
    drawCornerDot(startX + boxW - 4.f, startY + 2.f);
    drawCornerDot(startX + 2.f, startY + boxH - 4.f);
    drawCornerDot(startX + boxW - 4.f, startY + boxH - 4.f);

    float pulseScale = 1.0f;
    if (amberPulseTimer > 0.f) {
        pulseScale = 1.0f + std::sin((amberPulseTimer / 0.35f) * 3.14159f) * 0.22f;
    }

    sf::Vector2f iconCenter(startX + 20.f, startY + (boxH / 2.f));

    sf::ConvexShape diamond(4);
    diamond.setPoint(0, sf::Vector2f(0.f, -8.f * pulseScale));
    diamond.setPoint(1, sf::Vector2f(8.f * pulseScale, 0.f));
    diamond.setPoint(2, sf::Vector2f(0.f, 8.f * pulseScale));
    diamond.setPoint(3, sf::Vector2f(-8.f * pulseScale, 0.f));
    diamond.setPosition(iconCenter);
    diamond.setFillColor(sf::Color(235, 140, 20));
    diamond.setOutlineColor(sf::Color(90, 42, 8));
    diamond.setOutlineThickness(1.f);
    window.draw(diamond);

    sf::ConvexShape facet(3);
    facet.setPoint(0, sf::Vector2f(0.f, -8.f * pulseScale));
    facet.setPoint(1, sf::Vector2f(0.f, 8.f * pulseScale));
    facet.setPoint(2, sf::Vector2f(-8.f * pulseScale, 0.f));
    facet.setPosition(iconCenter);
    facet.setFillColor(sf::Color(255, 205, 55, 210));
    window.draw(facet);

    sf::RectangleShape glint(sf::Vector2f(2.f * pulseScale, 2.f * pulseScale));
    glint.setPosition(iconCenter.x - 3.f * pulseScale, iconCenter.y - 3.f * pulseScale);
    glint.setFillColor(sf::Color(255, 255, 230, 230));
    window.draw(glint);

    std::string textStr = std::to_string(playerApe->amberCount) + " / " + std::to_string(playerApe->maxAmber);
    sf::Text text(textStr, *font, 15);
    text.setPosition(startX + 38.f, startY + 7.f);

    if (playerApe->amberCount == 0) text.setFillColor(sf::Color(165, 145, 130));
    else if (playerApe->amberCount >= playerApe->maxAmber) text.setFillColor(sf::Color(255, 235, 115));
    else text.setFillColor(sf::Color(245, 215, 145));

    text.setOutlineColor(sf::Color(0, 0, 0, 240));
    text.setOutlineThickness(1.5f);
    window.draw(text);
}

void GameHUD::drawTimeHUD(sf::RenderWindow& window, sim::SimulationManager* simManager) {
    if (!simManager) return;

    float startX = 186.f;
    float startY = 24.f;
    float boxW = 296.f;
    float boxH = 36.f;

    sf::RectangleShape shadow(sf::Vector2f(boxW + 4.f, boxH + 4.f));
    shadow.setPosition(startX + 2.f, startY + 2.f);
    shadow.setFillColor(sf::Color(10, 8, 6, 160));
    window.draw(shadow);

    sf::RectangleShape outerFrame(sf::Vector2f(boxW, boxH));
    outerFrame.setPosition(startX, startY);
    outerFrame.setFillColor(sf::Color(24, 16, 10));
    outerFrame.setOutlineColor(sf::Color(12, 8, 5));
    outerFrame.setOutlineThickness(2.f);
    window.draw(outerFrame);

    sf::RectangleShape woodBevel(sf::Vector2f(boxW - 4.f, boxH - 4.f));
    woodBevel.setPosition(startX + 2.f, startY + 2.f);
    woodBevel.setFillColor(sf::Color(74, 48, 26));
    woodBevel.setOutlineColor(sf::Color(42, 26, 14));
    woodBevel.setOutlineThickness(1.f);
    window.draw(woodBevel);

    sf::RectangleShape innerPlate(sf::Vector2f(boxW - 8.f, boxH - 8.f));
    innerPlate.setPosition(startX + 4.f, startY + 4.f);
    innerPlate.setFillColor(sf::Color(26, 44, 28));
    innerPlate.setOutlineColor(sf::Color(168, 126, 52));
    innerPlate.setOutlineThickness(1.f);
    window.draw(innerPlate);

    sf::RectangleShape topShade(sf::Vector2f(boxW - 10.f, (boxH - 10.f) * 0.5f));
    topShade.setPosition(startX + 5.f, startY + 5.f);
    topShade.setFillColor(sf::Color(255, 255, 255, 14));
    window.draw(topShade);

    auto drawRivet = [&](float rx, float ry) {
        sf::CircleShape rivet(1.5f);
        rivet.setOrigin(1.5f, 1.5f);
        rivet.setPosition(rx, ry);
        rivet.setFillColor(sf::Color(235, 195, 95));
        rivet.setOutlineColor(sf::Color(30, 18, 10));
        rivet.setOutlineThickness(0.8f);
        window.draw(rivet);
    };

    drawRivet(startX + 6.f, startY + 6.f);
    drawRivet(startX + boxW - 6.f, startY + 6.f);
    drawRivet(startX + 6.f, startY + boxH - 6.f);
    drawRivet(startX + boxW - 6.f, startY + boxH - 6.f);

    int year = simManager->getRegistry().getYear();
    int day = simManager->getClock().getDays();
    int seasonIdx = static_cast<int>(simManager->getRegistry().getSeason());
    static const std::string seasonNames[] = { "Spr", "Sum", "Aut", "Win" };
    std::string sName = (seasonIdx >= 0 && seasonIdx < 4) ? seasonNames[seasonIdx] : "Era";

    sf::RectangleShape dateBadge(sf::Vector2f(120.f, 22.f));
    dateBadge.setPosition(startX + 10.f, startY + 7.f);
    dateBadge.setFillColor(sf::Color(18, 28, 19, 220));
    dateBadge.setOutlineColor(sf::Color(115, 85, 38));
    dateBadge.setOutlineThickness(1.f);
    window.draw(dateBadge);

    std::string dateStr = "Y" + std::to_string(year) + " " + sName + " D" + std::to_string(day);
    sf::Text dateText(dateStr, *font, 12);
    dateText.setStyle(sf::Text::Bold);
    dateText.setFillColor(sf::Color(245, 220, 140));
    dateText.setOutlineColor(sf::Color(10, 8, 5, 255));
    dateText.setOutlineThickness(1.5f);
    sf::FloatRect dtb = dateText.getLocalBounds();
    dateText.setOrigin(dtb.left + dtb.width * 0.5f, dtb.top + dtb.height * 0.5f);
    dateText.setPosition(startX + 10.f + 60.f, startY + 18.f);
    window.draw(dateText);

    for (int y = 0; y < 22; y += 4) {
        sf::RectangleShape stitch(sf::Vector2f(2.f, 2.f));
        stitch.setPosition(startX + 133.f, startY + 7.f + y);
        stitch.setFillColor(sf::Color(180, 140, 60, 180));
        window.draw(stitch);
    }

    float btnStartX = startX + 138.f;
    float btnW = 23.f;
    float btnH = 22.f;
    float btnGap = 2.f;

    for (int i = 0; i < 6; ++i) {
        float bx = btnStartX + i * (btnW + btnGap);
        float by = startY + 7.f;
        timeButtonBounds[i] = sf::FloatRect(bx, by, btnW, btnH);

        bool isActive = (i == 0) ? isGamePaused : (!isGamePaused && gameSpeed == i);

        sf::RectangleShape btnShadow(sf::Vector2f(btnW, btnH));
        btnShadow.setPosition(bx + 1.f, by + 1.f);
        btnShadow.setFillColor(sf::Color(0, 0, 0, 120));
        window.draw(btnShadow);

        sf::RectangleShape btn(sf::Vector2f(btnW, btnH));
        btn.setPosition(bx, by);

        if (isActive) {
            btn.setFillColor(sf::Color(145, 102, 38));
            btn.setOutlineColor(sf::Color(245, 210, 95));
            btn.setOutlineThickness(1.2f);
        } else {
            btn.setFillColor(sf::Color(32, 22, 16));
            btn.setOutlineColor(sf::Color(92, 64, 32));
            btn.setOutlineThickness(1.f);
        }
        window.draw(btn);

        sf::RectangleShape btnGloss(sf::Vector2f(btnW - 2.f, 2.f));
        btnGloss.setPosition(bx + 1.f, by + 1.f);
        btnGloss.setFillColor(isActive ? sf::Color(255, 240, 170, 110) : sf::Color(255, 255, 255, 30));
        window.draw(btnGloss);

        sf::Color iconCol = isActive ? sf::Color(255, 245, 210) : sf::Color(175, 145, 105);
        sf::Color iconShadow(15, 10, 6, 220);

        if (i == 0) {
            for (int p = 0; p < 2; ++p) {
                float px = bx + 7.f + p * 6.f;
                sf::RectangleShape pShadow(sf::Vector2f(3.f, 10.f));
                pShadow.setPosition(px + 1.f, by + 6.f + 1.f);
                pShadow.setFillColor(iconShadow);
                window.draw(pShadow);

                sf::RectangleShape bar(sf::Vector2f(3.f, 10.f));
                bar.setPosition(px, by + 6.f);
                bar.setFillColor(iconCol);
                window.draw(bar);
            }
        } else {
            int numArrows = i;
            float arrowW = (numArrows >= 4) ? 2.8f : ((numArrows == 3) ? 3.4f : 4.4f);
            float arrowH = (numArrows >= 4) ? 7.5f : ((numArrows == 3) ? 8.5f : 9.5f);
            float step = (numArrows >= 4) ? 3.3f : ((numArrows == 3) ? 4.2f : 5.5f);
            float totalW = arrowW + (numArrows - 1) * step;
            float arrowStartX = bx + (btnW - totalW) * 0.5f;
            float arrowCenterY = by + btnH * 0.5f;

            for (int a = 0; a < numArrows; ++a) {
                float ax = arrowStartX + a * step;
                sf::ConvexShape triShadow(3);
                triShadow.setPoint(0, sf::Vector2f(ax + 1.f, arrowCenterY - arrowH * 0.5f + 1.f));
                triShadow.setPoint(1, sf::Vector2f(ax + arrowW + 1.f, arrowCenterY + 1.f));
                triShadow.setPoint(2, sf::Vector2f(ax + 1.f, arrowCenterY + arrowH * 0.5f + 1.f));
                triShadow.setFillColor(iconShadow);
                window.draw(triShadow);

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