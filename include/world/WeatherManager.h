#pragma once
#include <SFML/Graphics.hpp>

enum class WeatherType {
    Clear,
    Cloudy,
    LightRain,
    HeavyRain,
    Fog,
    Storm
};

class WeatherManager {
private:
    WeatherType currentWeather;
    WeatherType targetWeather;
    float transitionTimer;
    float windIntensity;
    float rainIntensity;
    float fogDensity;

public:
    WeatherManager();
    void update(float dt);
    void setWeather(WeatherType type);
    
    float getWindIntensity() const;
    float getRainIntensity() const;
    float getFogDensity() const;
    WeatherType getCurrentWeather() const;
};