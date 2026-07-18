#include "world/LightingManager.h"
#include <cmath>

LightingManager::LightingManager() {
    timeOfDay = 0.f;
    ambientOverlay.setFillColor(sf::Color(0, 0, 50, 50)); 
}

void LightingManager::update(float dt, const sf::View& cameraView) {
    timeOfDay += dt * 0.1f; // Slow day cycle
    
    // Pulse light gently for ambient forest feel
    sf::Uint8 alpha = static_cast<sf::Uint8>(40 + std::sin(timeOfDay) * 20);
    ambientOverlay.setFillColor(sf::Color(10, 15, 30, alpha));

    ambientOverlay.setSize(cameraView.getSize());
    ambientOverlay.setPosition(cameraView.getCenter() - (cameraView.getSize() / 2.f));
}

void LightingManager::draw(sf::RenderWindow& window) const {
    window.draw(ambientOverlay);
}