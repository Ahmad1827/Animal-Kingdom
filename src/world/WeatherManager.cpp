#include "world/WeatherManager.h"
#include <cmath>

WeatherManager::WeatherManager() 
    : currentWeather(WeatherType::Clear), 
      currentSeason(Season::Summer), 
      windVector(1.f, 0.1f), 
      windIntensity(0.2f), 
      rainIntensity(0.f), 
      fogDensity(0.1f), 
      transitionTimer(0.f) {}

void WeatherManager::update(float dt) {
    transitionTimer += dt;
    
    // Slow, natural weather progression
    if (transitionTimer > 60.f) {
        transitionTimer = 0.f;
        int nextWeather = std::rand() % 6;
        currentWeather = static_cast<WeatherType>(nextWeather);
    }

    float targetWind = 0.2f;
    float targetRain = 0.f;
    float targetFog = 0.05f;

    switch (currentWeather) {
        case WeatherType::Clear: targetWind = 0.2f; targetRain = 0.0f; targetFog = 0.05f; break;
        case WeatherType::Cloudy: targetWind = 0.4f; targetRain = 0.0f; targetFog = 0.15f; break;
        case WeatherType::LightRain: targetWind = 0.5f; targetRain = 0.4f; targetFog = 0.3f; break;
        case WeatherType::HeavyRain: targetWind = 0.8f; targetRain = 0.9f; targetFog = 0.5f; break;
        case WeatherType::Storm: targetWind = 1.0f; targetRain = 1.0f; targetFog = 0.6f; break;
        case WeatherType::Fog: targetWind = 0.1f; targetRain = 0.0f; targetFog = 0.9f; break;
    }

    windIntensity += (targetWind - windIntensity) * dt * 0.1f;
    rainIntensity += (targetRain - rainIntensity) * dt * 0.1f;
    fogDensity += (targetFog - fogDensity) * dt * 0.1f;
    
    // Wind changes direction slowly over time
    float angle = transitionTimer * 0.05f;
    windVector = {std::cos(angle), std::abs(std::sin(angle)) * 0.5f};
}

sf::Vector2f WeatherManager::getWindVector() const { return windVector * windIntensity; }
float WeatherManager::getWindIntensity() const { return windIntensity; }
float WeatherManager::getRainIntensity() const { return rainIntensity; }
float WeatherManager::getFogDensity() const { return fogDensity; }

std::string WeatherManager::getWeatherString() const {
    switch (currentWeather) {
        case WeatherType::Clear: return "Clear";
        case WeatherType::Cloudy: return "Cloudy";
        case WeatherType::LightRain: return "Light Rain";
        case WeatherType::HeavyRain: return "Heavy Rain";
        case WeatherType::Storm: return "Storming";
        case WeatherType::Fog: return "Dense Fog";
        default: return "Unknown";
    }
}

std::string WeatherManager::getSeasonString() const {
    switch (currentSeason) {
        case Season::Spring: return "Spring";
        case Season::Summer: return "Summer";
        case Season::Autumn: return "Autumn";
        case Season::Winter: return "Winter";
        default: return "Unknown";
    }
}