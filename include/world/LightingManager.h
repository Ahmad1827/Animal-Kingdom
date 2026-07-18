#pragma once
#include <SFML/Graphics.hpp>

class LightingManager {
private:
    sf::RectangleShape ambientOverlay;
    sf::RectangleShape fogOverlay;
    float currentAmbientAlpha;

public:
    LightingManager();
    void update(float dt, const sf::View& cameraView, float timeOfDay, float fogDensity);
    void drawAmbient(sf::RenderWindow& window) const;
    void drawFog(sf::RenderWindow& window) const;
};