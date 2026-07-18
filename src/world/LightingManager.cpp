#include "world/LightingManager.h"

LightingManager::LightingManager() : currentAmbientAlpha(0.f) {}

void LightingManager::update(float dt, const sf::View& cameraView, float timeOfDay, float fogDensity) {
    ambientOverlay.setSize(cameraView.getSize());
    ambientOverlay.setPosition(cameraView.getCenter() - (cameraView.getSize() / 2.f));
    
    fogOverlay.setSize(cameraView.getSize());
    fogOverlay.setPosition(cameraView.getCenter() - (cameraView.getSize() / 2.f));

    // Dynamic Time of Day Colors
    sf::Color tint(0, 0, 0, 0);
    if (timeOfDay >= 0.2f && timeOfDay < 0.3f) { // Dawn
        tint = sf::Color(255, 100, 50, 40);
    } else if (timeOfDay >= 0.3f && timeOfDay < 0.7f) { // Day
        tint = sf::Color(0, 0, 0, 0);
    } else if (timeOfDay >= 0.7f && timeOfDay < 0.8f) { // Dusk
        tint = sf::Color(200, 50, 100, 60);
    } else { // Night
        tint = sf::Color(10, 15, 40, 180);
    }
    
    ambientOverlay.setFillColor(tint);

    // Fog interpolation
    sf::Color fogColor = sf::Color(150, 160, 170, static_cast<sf::Uint8>(fogDensity * 255.f));
    if (timeOfDay < 0.2f || timeOfDay > 0.8f) fogColor = sf::Color(20, 25, 30, static_cast<sf::Uint8>(fogDensity * 200.f));
    fogOverlay.setFillColor(fogColor);
}

void LightingManager::drawAmbient(sf::RenderWindow& window) const {
    window.draw(ambientOverlay);
}

void LightingManager::drawFog(sf::RenderWindow& window) const {
    window.draw(fogOverlay);
}