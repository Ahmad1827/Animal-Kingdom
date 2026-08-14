#include "world/DayNightCycle.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DayNightCycle::DayNightCycle() {
    sun.setFillColor(sf::Color(255, 255, 200, 255));
    sun.setOutlineThickness(4.f);
    sun.setOutlineColor(sf::Color(255, 200, 50, 100));

    moon.setFillColor(sf::Color(220, 220, 255, 255));
    moon.setOutlineThickness(4.f);
    moon.setOutlineColor(sf::Color(150, 180, 255, 100));

    colorNight = sf::Color(10, 15, 35);
    colorSunrise = sf::Color(255, 120, 80);
    colorDay = sf::Color(255, 255, 255);
    colorSunset = sf::Color(200, 80, 100);

    generateStars(250);
}

void DayNightCycle::generateStars(int count) {
    stars.setPrimitiveType(sf::Points);
    normalizedStars.clear();
    for (int i = 0; i < count; ++i) {
        float nx = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float ny = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        normalizedStars.push_back(sf::Vector2f(nx, ny));
        stars.append(sf::Vertex(sf::Vector2f(0.f, 0.f), sf::Color::Transparent));
    }
}

DayNightState DayNightCycle::calculateState(float normalizedTime) const {
    DayNightState state;
    state.normalizedTime = normalizedTime;
    state.time24h = normalizedTime * 24.0f;
    
    state.isDay = (state.time24h >= 6.0f && state.time24h < 18.0f);

    if (state.isDay) {
        state.sunProgress = (state.time24h - 6.0f) / 12.0f;
        state.moonProgress = 0.0f;
    } else {
        state.sunProgress = 0.0f;
        if (state.time24h >= 18.0f) {
            state.moonProgress = (state.time24h - 18.0f) / 12.0f;
        } else {
            state.moonProgress = (state.time24h + 6.0f) / 12.0f;
        }
    }

    if (state.time24h >= 5.0f && state.time24h < 7.0f) {
        state.darknessAlpha = 0.8f - 0.8f * ((state.time24h - 5.0f) / 2.0f);
    } else if (state.time24h >= 7.0f && state.time24h < 17.0f) {
        state.darknessAlpha = 0.0f;
    } else if (state.time24h >= 17.0f && state.time24h < 19.0f) {
        state.darknessAlpha = 0.8f * ((state.time24h - 17.0f) / 2.0f);
    } else {
        state.darknessAlpha = 0.8f;
    }

    state.starsAlpha = state.darknessAlpha;
    return state;
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

void DayNightCycle::updateSkyColor() {
    sf::Color currentSky;
    float t24 = currentState.time24h;

    if (t24 >= 0.0f && t24 < 4.0f) currentSky = colorNight;
    else if (t24 >= 4.0f && t24 < 6.0f) currentSky = interpolateColor(colorNight, colorSunrise, (t24 - 4.0f) / 2.0f);
    else if (t24 >= 6.0f && t24 < 8.0f) currentSky = interpolateColor(colorSunrise, colorDay, (t24 - 6.0f) / 2.0f);
    else if (t24 >= 8.0f && t24 < 16.0f) currentSky = colorDay;
    else if (t24 >= 16.0f && t24 < 18.0f) currentSky = interpolateColor(colorDay, colorSunset, (t24 - 16.0f) / 2.0f);
    else if (t24 >= 18.0f && t24 < 20.0f) currentSky = interpolateColor(colorSunset, colorNight, (t24 - 18.0f) / 2.0f);
    else currentSky = colorNight;

    skyRect.setFillColor(currentSky);
}

void DayNightCycle::updateCelestialBodies(const sf::View& cameraView) {
    sf::Vector2f viewSize = cameraView.getSize();
    sf::Vector2f cameraCenter = cameraView.getCenter();

    float sunRadius = std::max(60.f, viewSize.y * 0.06f);
    sun.setRadius(sunRadius);
    sun.setOrigin(sunRadius, sunRadius);

    float moonRadius = std::max(45.f, viewSize.y * 0.045f);
    moon.setRadius(moonRadius);
    moon.setOrigin(moonRadius, moonRadius);

    float arcRadiusX = viewSize.x * 0.45f;
    float arcRadiusY = viewSize.y * 0.40f; 
    float horizonY = cameraCenter.y + (viewSize.y * 0.15f);

    if (currentState.isDay) {
        float angle = M_PI - (currentState.sunProgress * M_PI);
        sun.setPosition(cameraCenter.x + arcRadiusX * std::cos(angle), horizonY - arcRadiusY * std::sin(angle));
    } else {
        sun.setPosition(cameraCenter.x + 99999.f, cameraCenter.y + 99999.f);
    }

    if (!currentState.isDay) {
        float angle = M_PI - (currentState.moonProgress * M_PI);
        moon.setPosition(cameraCenter.x + arcRadiusX * std::cos(angle), horizonY - arcRadiusY * std::sin(angle));
    } else {
        moon.setPosition(cameraCenter.x + 99999.f, cameraCenter.y + 99999.f);
    }
}

void DayNightCycle::updateStars(const sf::View& cameraView) {
    sf::Vector2f viewSize = cameraView.getSize();
    sf::Vector2f cameraCenter = cameraView.getCenter();

    float alpha = currentState.starsAlpha * 255.f;
    sf::Color starColor(255, 255, 255, static_cast<sf::Uint8>(alpha));
    
    float viewLeft = cameraCenter.x - viewSize.x / 2.f;
    float viewTop = cameraCenter.y - viewSize.y / 2.f;
    
    for (size_t i = 0; i < normalizedStars.size(); ++i) {
        stars[i].color = starColor;
        stars[i].position.x = viewLeft + (normalizedStars[i].x * viewSize.x);
        stars[i].position.y = viewTop + (normalizedStars[i].y * (viewSize.y * 0.7f));
    }
}

void DayNightCycle::update(float normalizedTime, const sf::View& cameraView) {
    currentState = calculateState(normalizedTime);

    sf::Vector2f viewSize = cameraView.getSize();
    sf::Vector2f cameraCenter = cameraView.getCenter();

    skyRect.setSize(viewSize);
    skyRect.setPosition(cameraCenter.x - viewSize.x / 2.f, cameraCenter.y - viewSize.y / 2.f);
    
    updateSkyColor();
    updateCelestialBodies(cameraView);
    updateStars(cameraView);
}

void DayNightCycle::draw(sf::RenderTarget& target) {
    if (stars.getVertexCount() > 0 && stars[0].color.a > 0) target.draw(stars);
    if (currentState.isDay) target.draw(sun);
    if (!currentState.isDay) target.draw(moon);
}