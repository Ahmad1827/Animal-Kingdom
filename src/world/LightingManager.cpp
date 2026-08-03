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
    ambientOverlay.setPosition(0.f, 0.f);
    
    fogOverlay.setSize(cameraView.getSize());
    fogOverlay.setPosition(0.f, 0.f);

    struct TimePhase { float time; sf::Color color; };
    TimePhase phases[] = {
        {0.00f, sf::Color(5, 10, 30, 200)},     
        {0.20f, sf::Color(10, 15, 40, 180)},    
        {0.25f, sf::Color(255, 100, 50, 80)},   
        {0.35f, sf::Color(255, 200, 150, 10)},  
        {0.50f, sf::Color(0, 0, 0, 0)},         
        {0.65f, sf::Color(255, 150, 100, 20)},  
        {0.75f, sf::Color(255, 80, 50, 100)},   
        {0.80f, sf::Color(100, 40, 80, 150)},   
        {0.85f, sf::Color(10, 15, 40, 180)},    
        {1.00f, sf::Color(5, 10, 30, 200)}      
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

    sf::Color baseFog(150, 160, 170, static_cast<sf::Uint8>(fogDensity * 255.f));
    if (timeOfDay < 0.2f || timeOfDay > 0.8f) {
        baseFog = sf::Color(20, 25, 30, static_cast<sf::Uint8>(fogDensity * 200.f));
    }
    fogOverlay.setFillColor(baseFog);
}

void LightingManager::drawAmbient(sf::RenderTarget& target) const { target.draw(ambientOverlay); }
void LightingManager::drawFog(sf::RenderTarget& target) const { target.draw(fogOverlay); }