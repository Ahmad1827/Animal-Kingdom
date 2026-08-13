#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// Single Source of Truth for environmental state
struct DayNightState {
    float normalizedTime; // 0.0 to 1.0
    float time24h;        // 0.0 to 24.0
    bool isDay;           // true between 06:00 and 18:00
    float sunProgress;    // 0.0 to 1.0 during the day
    float moonProgress;   // 0.0 to 1.0 during the night
    float darknessAlpha;  // Reference value for synchronized darkness
    float starsAlpha;     // 0.0 to 1.0
};

class DayNightCycle {
public:
    DayNightCycle();
    
    // Calculates the unified state from the WorldClock
    DayNightState calculateState(float normalizedTime) const;

    void update(float normalizedTime, const sf::View& cameraView);
    void draw(sf::RenderTarget& target);

    DayNightState getCurrentState() const { return currentState; }
    
    // --- NEW: Expose dynamic sky color to tint Layer 1 ---
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