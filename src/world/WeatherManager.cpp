#include "world/WeatherManager.h"
#include <cmath>
#include <algorithm>

WeatherManager::WeatherManager() {
    currentWeather = WeatherType::Clear;
    targetWeather = WeatherType::Clear;
    transitionTimer = 0.f;
    windIntensity = 0.2f;
    rainIntensity = 0.f;
    fogDensity = 0.1f;
}

void WeatherManager::update(float dt) {
    // Smooth transition logic for atmosphere
    float targetWind = 0.2f;
    float targetRain = 0.f;
    float targetFog = 0.1f;

    switch (currentWeather) {
        case WeatherType::Clear: targetWind = 0.2f; targetRain = 0.0f; targetFog = 0.05f; break;
        case WeatherType::Cloudy: targetWind = 0.4f; targetRain = 0.0f; targetFog = 0.15f; break;
        case WeatherType::LightRain: targetWind = 0.5f; targetRain = 0.4f; targetFog = 0.3f; break;
        case WeatherType::HeavyRain: targetWind = 0.8f; targetRain = 0.9f; targetFog = 0.5f; break;
        case WeatherType::Storm: targetWind = 1.0f; targetRain = 1.0f; targetFog = 0.6f; break;
        case WeatherType::Fog: targetWind = 0.1f; targetRain = 0.0f; targetFog = 0.9f; break;
    }

    windIntensity += (targetWind - windIntensity) * dt * 0.5f;
    rainIntensity += (targetRain - rainIntensity) * dt * 0.5f;
    fogDensity += (targetFog - fogDensity) * dt * 0.5f;
}

void WeatherManager::setWeather(WeatherType type) {
    targetWeather = type;
    currentWeather = type; // Instant for now, can be delayed
}

float WeatherManager::getWindIntensity() const { return windIntensity; }
float WeatherManager::getRainIntensity() const { return rainIntensity; }
float WeatherManager::getFogDensity() const { return fogDensity; }
WeatherType WeatherManager::getCurrentWeather() const { return currentWeather; }