#pragma once
#include <SFML/Graphics.hpp>

class DayNightCycle {
public:
    DayNightCycle(const sf::Vector2f& screenSize);
    void update(float normalizedTime, const sf::Vector2f& cameraCenter);
    void draw(sf::RenderTarget& target);

private:
    sf::RectangleShape skyRect;
    sf::CircleShape sun;
    sf::CircleShape moon;
    sf::VertexArray stars;
    sf::Vector2f screenSize;
    sf::Color colorNight;
    sf::Color colorSunrise;
    sf::Color colorDay;
    sf::Color colorSunset;

    sf::Color interpolateColor(const sf::Color& c1, const sf::Color& c2, float t);
    void updateSkyColor(float time);
    void updateCelestialBodies(float time, const sf::Vector2f& cameraCenter);
    void updateStars(float time, const sf::Vector2f& cameraCenter);
    void generateStars();
};