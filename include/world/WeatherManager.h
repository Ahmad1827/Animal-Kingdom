#pragma once
#include <SFML/Graphics.hpp>
#include <string>

enum class WeatherType { Clear, Cloudy, LightRain, HeavyRain, Fog, Storm };
enum class Season { Spring, Summer, Autumn, Winter };

class WeatherManager {
private:
    WeatherType currentWeather;
    Season currentSeason; // MUST be here
    sf::Vector2f windVector; // MUST be here
    float windIntensity;
    float rainIntensity;
    float fogDensity;
    float transitionTimer;

public:
    WeatherManager();
    void update(float dt);
    
    sf::Vector2f getWindVector() const;
    float getWindIntensity() const;
    float getRainIntensity() const;
    float getFogDensity() const;
    
    std::string getWeatherString() const;
    std::string getSeasonString() const;
};