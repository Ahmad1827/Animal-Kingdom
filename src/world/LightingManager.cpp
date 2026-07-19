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
    ambientOverlay.setSize(cameraView.getSize());
    ambientOverlay.setPosition(cameraView.getCenter() - (cameraView.getSize() / 2.f));
    
    fogOverlay.setSize(cameraView.getSize());
    fogOverlay.setPosition(cameraView.getCenter() - (cameraView.getSize() / 2.f));

    // Smooth 7-Stage Day/Night Interpolation
    struct TimePhase { float time; sf::Color color; };
    TimePhase phases[] = {
        {0.00f, sf::Color(5, 10, 30, 200)},     // Midnight
        {0.20f, sf::Color(10, 15, 40, 180)},    // Pre-Dawn
        {0.25f, sf::Color(255, 100, 50, 80)},   // Sunrise
        {0.35f, sf::Color(255, 200, 150, 10)},  // Morning
        {0.50f, sf::Color(0, 0, 0, 0)},         // Noon
        {0.65f, sf::Color(255, 150, 100, 20)},  // Afternoon
        {0.75f, sf::Color(255, 80, 50, 100)},   // Sunset
        {0.80f, sf::Color(100, 40, 80, 150)},   // Dusk
        {0.85f, sf::Color(10, 15, 40, 180)},    // Night
        {1.00f, sf::Color(5, 10, 30, 200)}      // Midnight
    };

    sf::Color currentTint = phases[0].color;
    for (int i = 0; i < 9; ++i) {
        if (timeOfDay >= phases[i].time && timeOfDay <= phases[i+1].time) {
            float t = (timeOfDay - phases[i].time) / (phases[i+1].time - phases[i].time);
            currentTint = lerpColor(phases[i].color, phases[i+1].color, t);
            break;
        }
    }
    
    ambientOverlay.setFillColor(currentTint);

    // Dynamic Atmospheric Fog
    sf::Color baseFog(150, 160, 170, static_cast<sf::Uint8>(fogDensity * 255.f));
    if (timeOfDay < 0.2f || timeOfDay > 0.8f) {
        baseFog = sf::Color(20, 25, 30, static_cast<sf::Uint8>(fogDensity * 200.f));
    }
    fogOverlay.setFillColor(baseFog);
}

void LightingManager::drawAmbient(sf::RenderWindow& window) const { window.draw(ambientOverlay); }
void LightingManager::drawFog(sf::RenderWindow& window) const { window.draw(fogOverlay); }