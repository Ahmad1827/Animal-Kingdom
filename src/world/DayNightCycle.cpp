#include "world/DayNightCycle.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DayNightCycle::DayNightCycle(const sf::Vector2f& screenSize) : screenSize(screenSize) {
    skyRect.setSize(screenSize);
    
    sun.setRadius(40.f);
    sun.setOrigin(40.f, 40.f);
    sun.setFillColor(sf::Color(255, 255, 180));

    moon.setRadius(30.f);
    moon.setOrigin(30.f, 30.f);
    moon.setFillColor(sf::Color(200, 200, 255));

    colorNight = sf::Color(10, 15, 35);
    colorSunrise = sf::Color(255, 120, 80);
    colorDay = sf::Color(100, 180, 255);
    colorSunset = sf::Color(200, 80, 100);

    generateStars();
}

void DayNightCycle::generateStars() {
    stars.setPrimitiveType(sf::Points);
    int numStars = 150;
    for (int i = 0; i < numStars; ++i) {
        float x = static_cast<float>(std::rand() % static_cast<int>(screenSize.x));
        float y = static_cast<float>(std::rand() % static_cast<int>(screenSize.y * 0.7f));
        stars.append(sf::Vertex(sf::Vector2f(x, y), sf::Color::Transparent));
    }
}

sf::Color DayNightCycle::interpolateColor(const sf::Color& c1, const sf::Color& c2, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    return sf::Color(
        c1.r + static_cast<sf::Uint8>((c2.r - c1.r) * t),
        c1.g + static_cast<sf::Uint8>((c2.g - c1.g) * t),
        c1.b + static_cast<sf::Uint8>((c2.b - c1.b) * t),
        c1.a + static_cast<sf::Uint8>((c2.a - c1.a) * t)
    );
}

void DayNightCycle::updateSkyColor(float time) {
    sf::Color currentSky;

    if (time >= 0.0f && time < 0.2f) { 
        currentSky = colorNight; 
    } else if (time >= 0.2f && time < 0.3f) { 
        currentSky = interpolateColor(colorNight, colorSunrise, (time - 0.2f) / 0.1f); 
    } else if (time >= 0.3f && time < 0.4f) { 
        currentSky = interpolateColor(colorSunrise, colorDay, (time - 0.3f) / 0.1f); 
    } else if (time >= 0.4f && time < 0.6f) { 
        currentSky = colorDay; 
    } else if (time >= 0.6f && time < 0.7f) { 
        currentSky = interpolateColor(colorDay, colorSunset, (time - 0.6f) / 0.1f); 
    } else if (time >= 0.7f && time < 0.8f) { 
        currentSky = interpolateColor(colorSunset, colorNight, (time - 0.7f) / 0.1f); 
    } else { 
        currentSky = colorNight; 
    }

    skyRect.setFillColor(currentSky);
}

void DayNightCycle::updateCelestialBodies(float time, const sf::Vector2f& cameraCenter) {
    float arcRadius = screenSize.x * 0.45f;
    float horizonY = cameraCenter.y + (screenSize.y * 0.2f);
    
    if (time >= 0.15f && time <= 0.85f) {
        float p = (time - 0.15f) / 0.7f;
        float angle = M_PI - (p * M_PI);
        
        float sunX = cameraCenter.x + arcRadius * std::cos(angle);
        float sunY = horizonY - arcRadius * std::sin(angle);
        sun.setPosition(sunX, sunY);
    } else {
        sun.setPosition(-9999.f, -9999.f);
    }

    float moonTime = time;
    if (moonTime < 0.15f) moonTime += 1.0f;

    if (moonTime >= 0.65f && moonTime <= 1.35f) {
        float p = (moonTime - 0.65f) / 0.7f;
        float angle = M_PI - (p * M_PI);
        
        float moonX = cameraCenter.x + arcRadius * std::cos(angle);
        float moonY = horizonY - arcRadius * std::sin(angle);
        moon.setPosition(moonX, moonY);
    } else {
        moon.setPosition(-9999.f, -9999.f);
    }
}

void DayNightCycle::updateStars(float time, const sf::Vector2f& cameraCenter) {
    float alpha = 0.f;
    
    if (time >= 0.75f) {
        alpha = std::min(255.f, (time - 0.75f) / 0.1f * 255.f);
    } else if (time <= 0.25f) {
        alpha = std::max(0.f, 255.f - (time / 0.1f * 255.f));
    }

    sf::Color starColor(255, 255, 255, static_cast<sf::Uint8>(alpha));
    
    for (size_t i = 0; i < stars.getVertexCount(); ++i) {
        stars[i].color = starColor;
        stars[i].position.x = cameraCenter.x - (screenSize.x / 2.f) + std::fmod(stars[i].position.x, screenSize.x);
        stars[i].position.y = cameraCenter.y - (screenSize.y / 2.f) + std::fmod(stars[i].position.y, screenSize.y);
    }
}

void DayNightCycle::update(float normalizedTime, const sf::Vector2f& cameraCenter) {
    skyRect.setPosition(cameraCenter.x - screenSize.x / 2.f, cameraCenter.y - screenSize.y / 2.f);
    
    updateSkyColor(normalizedTime);
    updateCelestialBodies(normalizedTime, cameraCenter);
    updateStars(normalizedTime, cameraCenter);
}

void DayNightCycle::draw(sf::RenderTarget& target) {
    target.draw(skyRect);
    
    if (stars[0].color.a > 0) {
        target.draw(stars);
    }

    target.draw(sun);
    target.draw(moon);
}