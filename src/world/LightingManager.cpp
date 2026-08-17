#include "world/LightingManager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

LightingManager::LightingManager() 
    : lightningTimer(0.f), lightningFlashAlpha(0.f), currentAmbientAlpha(0.f) {
    atmosphericHaze.setPrimitiveType(sf::Quads);
    atmosphericHaze.resize(4);
}

static sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return sf::Color(
        static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
        static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
        static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
        static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
    );
}

void LightingManager::triggerLightning() {
    lightningFlashAlpha = 140.f;
}

void LightingManager::update(float dt, const sf::View& cameraView, float timeOfDay, float fogDensity, float rainIntensity, bool isStorm) {
    struct TimePhase { 
        float time; 
        sf::Color ambientColor; 
    };

    TimePhase phases[] = {
        {0.00f, sf::Color(10, 18, 48, 105)},
        {0.18f, sf::Color(10, 18, 48, 105)},
        {0.23f, sf::Color(45, 25, 65, 85)},
        {0.27f, sf::Color(255, 145, 80, 40)},
        {0.33f, sf::Color(255, 215, 140, 15)},
        {0.40f, sf::Color(255, 255, 255, 0)},
        {0.65f, sf::Color(255, 255, 255, 0)},
        {0.72f, sf::Color(255, 180, 70, 25)},
        {0.78f, sf::Color(245, 105, 55, 60)},
        {0.83f, sf::Color(65, 30, 75, 80)},
        {0.88f, sf::Color(15, 22, 55, 100)},
        {1.00f, sf::Color(10, 18, 48, 105)}
    };

    const int phaseCount = sizeof(phases) / sizeof(phases[0]);
    sf::Color currentTint = phases[0].ambientColor;

    for (int i = 0; i < phaseCount - 1; ++i) {
        if (timeOfDay >= phases[i].time && timeOfDay <= phases[i + 1].time) {
            float t = (timeOfDay - phases[i].time) / (phases[i + 1].time - phases[i].time);
            currentTint = lerpColor(phases[i].ambientColor, phases[i + 1].ambientColor, t);
            break;
        }
    }

    if (rainIntensity > 0.05f) {
        sf::Color rainTint(25, 35, 50, static_cast<sf::Uint8>(rainIntensity * 45.f));
        currentTint = lerpColor(currentTint, rainTint, rainIntensity * 0.45f);
        currentTint.a = static_cast<sf::Uint8>(std::min(255.f, currentTint.a + rainIntensity * 30.f));
    }

    if (isStorm) {
        lightningTimer += dt;
        if (lightningTimer > 4.5f + (std::rand() % 40) / 10.f) {
            lightningTimer = 0.f;
            triggerLightning();
        }
    } else {
        lightningTimer = 0.f;
    }

    if (lightningFlashAlpha > 0.f) {
        lightningFlashAlpha -= dt * 650.f;
        if (lightningFlashAlpha < 0.f) lightningFlashAlpha = 0.f;
    }

    ambientOverlay.setFillColor(currentTint);

    sf::Color topHaze(180, 200, 220, static_cast<sf::Uint8>(fogDensity * 45.f));
    sf::Color bottomHaze(140, 160, 180, static_cast<sf::Uint8>(fogDensity * 12.f));

    if (timeOfDay < 0.22f || timeOfDay > 0.82f) {
        topHaze = sf::Color(15, 25, 40, static_cast<sf::Uint8>(fogDensity * 60.f));
        bottomHaze = sf::Color(10, 18, 30, static_cast<sf::Uint8>(fogDensity * 20.f));
    }

    atmosphericHaze[0].color = topHaze;
    atmosphericHaze[1].color = topHaze;
    atmosphericHaze[2].color = bottomHaze;
    atmosphericHaze[3].color = bottomHaze;
}

void LightingManager::drawFog(sf::RenderTarget& target) const {
    if (atmosphericHaze[0].color.a == 0 && atmosphericHaze[2].color.a == 0) return;

    sf::View prevView = target.getView();
    target.setView(target.getDefaultView());

    sf::Vector2f winSize(static_cast<float>(target.getSize().x), static_cast<float>(target.getSize().y));

    sf::VertexArray haze = atmosphericHaze;
    haze[0].position = sf::Vector2f(0.f, 0.f);
    haze[1].position = sf::Vector2f(winSize.x, 0.f);
    haze[2].position = sf::Vector2f(winSize.x, winSize.y);
    haze[3].position = sf::Vector2f(0.f, winSize.y);

    target.draw(haze);
    target.setView(prevView);
}

void LightingManager::drawAmbient(sf::RenderTarget& target) const {
    sf::View prevView = target.getView();
    target.setView(target.getDefaultView());

    sf::Vector2f winSize(static_cast<float>(target.getSize().x), static_cast<float>(target.getSize().y));

    if (ambientOverlay.getFillColor().a > 0) {
        sf::RectangleShape screenAmbient(winSize);
        screenAmbient.setPosition(0.f, 0.f);
        screenAmbient.setFillColor(ambientOverlay.getFillColor());
        target.draw(screenAmbient);
    }

    if (lightningFlashAlpha > 1.f) {
        sf::RectangleShape flash(winSize);
        flash.setPosition(0.f, 0.f);
        flash.setFillColor(sf::Color(240, 248, 255, static_cast<sf::Uint8>(lightningFlashAlpha)));
        target.draw(flash);
    }

    target.setView(prevView);
}