#pragma once
#include <SFML/Graphics.hpp>

class LightingManager {
private:
    sf::RectangleShape ambientOverlay;
    sf::VertexArray atmosphericHaze;
    sf::RectangleShape lightningOverlay;
    
    float lightningTimer;
    float lightningFlashAlpha;
    float currentAmbientAlpha;

public:
    LightingManager();
    void update(float dt, const sf::View& cameraView, float timeOfDay, float fogDensity, float rainIntensity = 0.f, bool isStorm = false);
    void triggerLightning();
    void drawAmbient(sf::RenderTarget& target) const;
    void drawFog(sf::RenderTarget& target) const;
};