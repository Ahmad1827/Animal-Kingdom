#include "world/SkySystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

SkySystem::SkySystem() {
    dayPalette.zenith = sf::Color(45, 95, 175);
    dayPalette.midSky = sf::Color(85, 150, 220);
    dayPalette.horizon = sf::Color(190, 225, 245);
    dayPalette.sunTint = sf::Color(255, 250, 200);
    dayPalette.ambientLight = sf::Color(255, 255, 255);

    sunsetPalette.zenith = sf::Color(40, 25, 65);
    sunsetPalette.midSky = sf::Color(175, 60, 65);
    sunsetPalette.horizon = sf::Color(245, 140, 60);
    sunsetPalette.sunTint = sf::Color(255, 100, 40);
    sunsetPalette.ambientLight = sf::Color(240, 160, 110);

    nightPalette.zenith = sf::Color(6, 8, 18);
    nightPalette.midSky = sf::Color(14, 18, 38);
    nightPalette.horizon = sf::Color(32, 28, 55);
    nightPalette.sunTint = sf::Color(180, 200, 240);
    nightPalette.ambientLight = sf::Color(70, 80, 120);

    dawnPalette.zenith = sf::Color(25, 20, 50);
    dawnPalette.midSky = sf::Color(115, 60, 95);
    dawnPalette.horizon = sf::Color(230, 130, 110);
    dawnPalette.sunTint = sf::Color(255, 200, 140);
    dawnPalette.ambientLight = sf::Color(190, 160, 180);
}

void SkySystem::init(float width, float height, int starCount) {
    skyWidth = width;
    skyHeight = height;

    stars.clear();
    stars.reserve(starCount);
    for (int i = 0; i < starCount; ++i) {
        Star s;
        s.position = sf::Vector2f(
            static_cast<float>(std::rand() % static_cast<int>(width)),
            static_cast<float>(std::rand() % static_cast<int>(height * 0.75f))
        );
        s.baseBrightness = 80.f + static_cast<float>(std::rand() % 175);
        s.twinkleSpeed = 1.5f + static_cast<float>(std::rand() % 40) * 0.1f;
        s.size = (std::rand() % 10 > 7) ? 2.0f : 1.0f;
        stars.push_back(s);
    }

    sunShape.setRadius(28.f);
    sunShape.setOrigin(28.f, 28.f);

    sunHalo.setRadius(54.f);
    sunHalo.setOrigin(54.f, 54.f);

    moonShape.setRadius(24.f);
    moonShape.setOrigin(24.f, 24.f);
    moonShape.setFillColor(sf::Color(240, 242, 230));

    moonCrescentCut.setRadius(22.f);
    moonCrescentCut.setOrigin(22.f, 22.f);
}

sf::Color SkySystem::lerpColor(const sf::Color& a, const sf::Color& b, float t) const {
    return sf::Color(
        static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
        static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
        static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
        static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
    );
}

SkyPalette SkySystem::evaluateCurrentPalette(float timeOfDay) const {
    if (timeOfDay < 0.20f) {
        float t = timeOfDay / 0.20f;
        return {
            lerpColor(nightPalette.zenith, dawnPalette.zenith, t),
            lerpColor(nightPalette.midSky, dawnPalette.midSky, t),
            lerpColor(nightPalette.horizon, dawnPalette.horizon, t),
            lerpColor(nightPalette.sunTint, dawnPalette.sunTint, t),
            lerpColor(nightPalette.ambientLight, dawnPalette.ambientLight, t)
        };
    } else if (timeOfDay < 0.45f) {
        float t = (timeOfDay - 0.20f) / 0.25f;
        return {
            lerpColor(dawnPalette.zenith, dayPalette.zenith, t),
            lerpColor(dawnPalette.midSky, dayPalette.midSky, t),
            lerpColor(dawnPalette.horizon, dayPalette.horizon, t),
            lerpColor(dawnPalette.sunTint, dayPalette.sunTint, t),
            lerpColor(dawnPalette.ambientLight, dayPalette.ambientLight, t)
        };
    } else if (timeOfDay < 0.70f) {
        float t = (timeOfDay - 0.45f) / 0.25f;
        return {
            lerpColor(dayPalette.zenith, sunsetPalette.zenith, t),
            lerpColor(dayPalette.midSky, sunsetPalette.midSky, t),
            lerpColor(dayPalette.horizon, sunsetPalette.horizon, t),
            lerpColor(dayPalette.sunTint, sunsetPalette.sunTint, t),
            lerpColor(dayPalette.ambientLight, sunsetPalette.ambientLight, t)
        };
    } else {
        float t = (timeOfDay - 0.70f) / 0.30f;
        return {
            lerpColor(sunsetPalette.zenith, nightPalette.zenith, t),
            lerpColor(sunsetPalette.midSky, nightPalette.midSky, t),
            lerpColor(sunsetPalette.horizon, nightPalette.horizon, t),
            lerpColor(sunsetPalette.sunTint, nightPalette.sunTint, t),
            lerpColor(sunsetPalette.ambientLight, nightPalette.ambientLight, t)
        };
    }
}

void SkySystem::update(float dt, float timeOfDay, float cameraX) {
    starTime += dt;
}

void SkySystem::drawSky(sf::RenderTarget& target, float timeOfDay, float cameraX) {
    SkyPalette pal = evaluateCurrentPalette(timeOfDay);

    const int bandCount = 12;
    float bandH = skyHeight / static_cast<float>(bandCount);

    sf::VertexArray bands(sf::Quads, bandCount * 4);

    for (int i = 0; i < bandCount; ++i) {
        float y0 = i * bandH;
        float y1 = (i + 1) * bandH;
        float normY = static_cast<float>(i) / static_cast<float>(bandCount - 1);

        sf::Color bandColor;
        if (normY < 0.5f) {
            bandColor = lerpColor(pal.zenith, pal.midSky, normY * 2.0f);
        } else {
            bandColor = lerpColor(pal.midSky, pal.horizon, (normY - 0.5f) * 2.0f);
        }

        int idx = i * 4;
        bands[idx + 0] = sf::Vertex(sf::Vector2f(0.f, y0), bandColor);
        bands[idx + 1] = sf::Vertex(sf::Vector2f(skyWidth, y0), bandColor);
        bands[idx + 2] = sf::Vertex(sf::Vector2f(skyWidth, y1), bandColor);
        bands[idx + 3] = sf::Vertex(sf::Vector2f(0.f, y1), bandColor);
    }

    target.draw(bands);
}

void SkySystem::drawStars(sf::RenderTarget& target, float timeOfDay) {
    float nightFactor = 0.f;
    if (timeOfDay > 0.75f) {
        nightFactor = (timeOfDay - 0.75f) / 0.15f;
    } else if (timeOfDay < 0.25f) {
        nightFactor = 1.0f - (timeOfDay / 0.20f);
    }
    nightFactor = std::clamp(nightFactor, 0.f, 1.f);

    if (nightFactor <= 0.01f) return;

    for (const auto& s : stars) {
        float twinkle = (std::sin(starTime * s.twinkleSpeed + s.position.x) + 1.f) * 0.5f;
        sf::Uint8 alpha = static_cast<sf::Uint8>(s.baseBrightness * (0.5f + 0.5f * twinkle) * nightFactor);

        sf::RectangleShape r(sf::Vector2f(s.size, s.size));
        r.setPosition(s.position);
        r.setFillColor(sf::Color(220, 230, 255, alpha));
        target.draw(r);
    }
}

void SkySystem::drawCelestials(sf::RenderTarget& target, float timeOfDay, float cameraX) {
    SkyPalette pal = evaluateCurrentPalette(timeOfDay);

    float sunAngle = (timeOfDay - 0.20f) / 0.55f;
    if (sunAngle >= 0.0f && sunAngle <= 1.0f) {
        float rad = sunAngle * 3.14159265f;
        float sunX = 100.f + (skyWidth - 200.f) * sunAngle;
        float sunY = (skyHeight * 0.85f) - std::sin(rad) * (skyHeight * 0.70f);

        sunHalo.setPosition(sunX, sunY);
        sunHalo.setFillColor(sf::Color(pal.sunTint.r, pal.sunTint.g, pal.sunTint.b, 60));
        target.draw(sunHalo);

        sunShape.setPosition(sunX, sunY);
        sunShape.setFillColor(pal.sunTint);
        target.draw(sunShape);
    }

    float moonAngle = (timeOfDay >= 0.70f) ? ((timeOfDay - 0.70f) / 0.55f) : ((timeOfDay + 0.30f) / 0.55f);
    if (moonAngle >= 0.0f && moonAngle <= 1.0f) {
        float rad = moonAngle * 3.14159265f;
        float moonX = 100.f + (skyWidth - 200.f) * moonAngle;
        float moonY = (skyHeight * 0.85f) - std::sin(rad) * (skyHeight * 0.70f);

        moonShape.setPosition(moonX, moonY);
        target.draw(moonShape);

        moonCrescentCut.setPosition(moonX + 7.f, moonY - 3.f);
        moonCrescentCut.setFillColor(pal.zenith);
        target.draw(moonCrescentCut);
    }
}