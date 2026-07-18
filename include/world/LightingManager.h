#pragma once
#include <SFML/Graphics.hpp>

class LightingManager {
private:
    sf::RectangleShape ambientOverlay;
    float timeOfDay;

public:
    LightingManager();
    void update(float dt, const sf::View& cameraView);
    void draw(sf::RenderWindow& window) const;
};