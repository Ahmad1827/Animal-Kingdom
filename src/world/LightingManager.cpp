#include "world/LightingManager.h"
#include <algorithm>

LightingManager::LightingManager() : currentAmbientAlpha(0.f) {}

sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return sf::Color(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}

void LightingManager::update(float dt, const sf::View& cameraView, float timeOfDay, float fogDensity) {
    // Lock overlay sizes to the full default window bounds (1280x720)
    ambientOverlay.setSize(sf::Vector2f(1280.f, 720.f));
    ambientOverlay.setPosition(0.f, 0.f);
    
    fogOverlay.setSize(sf::Vector2f(1280.f, 720.f));
    fogOverlay.setPosition(0.f, 0.f);

    struct TimePhase { float time; sf::Color color; };
    TimePhase phases[] = {
        {0.00f, sf::Color(5, 10, 30, 220)},     // 00:00 Pitch Black
        {0.20f, sf::Color(5, 10, 30, 220)},     // 04:48 Still Dark
        {0.25f, sf::Color(255, 120, 50, 100)}, // 06:00 Dawn
        {0.35f, sf::Color(0, 0, 0, 0)},        // 08:24 Daylight
        {0.65f, sf::Color(0, 0, 0, 0)},        // 15:36 Daylight
        {0.75f, sf::Color(255, 80, 50, 110)},  // 18:00 Sunset
        {0.80f, sf::Color(20, 10, 40, 180)},   // 19:12 Dusk
        {0.85f, sf::Color(5, 10, 30, 220)},    // 20:24 Night
        {1.00f, sf::Color(5, 10, 30, 220)}     // 24:00 Pitch Black
    };

    sf::Color currentTint = phases[0].color;
    for (int i = 0; i < 8; ++i) {
        if (timeOfDay >= phases[i].time && timeOfDay <= phases[i+1].time) {
            float t = (timeOfDay - phases[i].time) / (phases[i+1].time - phases[i].time);
            currentTint = lerpColor(phases[i].color, phases[i+1].color, t);
            break;
        }
    }
    
    ambientOverlay.setFillColor(currentTint);

    sf::Color baseFog(150, 160, 170, static_cast<sf::Uint8>(fogDensity * 255.f));
    if (timeOfDay < 0.2f || timeOfDay > 0.8f) {
        baseFog = sf::Color(20, 25, 30, static_cast<sf::Uint8>(fogDensity * 200.f));
    }
    fogOverlay.setFillColor(baseFog);
}

void LightingManager::drawAmbient(sf::RenderTarget& target) const { target.draw(ambientOverlay); }
void LightingManager::drawFog(sf::RenderTarget& target) const { target.draw(fogOverlay); }