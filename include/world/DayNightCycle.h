#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct DayNightState {
    float normalizedTime;
    float time24h;
    bool isDay;
    float sunProgress;
    float moonProgress;
    float darknessAlpha;
    float starsAlpha;
};

class DayNightCycle {
public:
    DayNightCycle();
    
    DayNightState calculateState(float normalizedTime) const;

    void update(float normalizedTime, const sf::View& cameraView);
    void draw(sf::RenderTarget& target);

    DayNightState getCurrentState() const { return currentState; }
    sf::Color getSkyColor() const { return skyRect.getFillColor(); }

private:
    sf::RectangleShape skyRect;
    sf::CircleShape sun;
    sf::CircleShape moon;
    sf::VertexArray stars;
    std::vector<sf::Vector2f> normalizedStars;

    sf::Color colorNight;
    sf::Color colorSunrise;
    sf::Color colorDay;
    sf::Color colorSunset;

    DayNightState currentState;

    sf::Color interpolateColor(const sf::Color& c1, const sf::Color& c2, float t);
    void updateSkyColor();
    void updateCelestialBodies(const sf::View& cameraView);
    void updateStars(const sf::View& cameraView);
    void generateStars(int count);
};