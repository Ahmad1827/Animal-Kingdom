#include "world/SkySystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

SkySystem::SkySystem() {
    dayPalette.zenith     = sf::Color(32, 68, 140);
    dayPalette.midSky     = sf::Color(64, 122, 195);
    dayPalette.lowerSky   = sf::Color(120, 175, 225);
    dayPalette.horizon    = sf::Color(195, 222, 238);
    dayPalette.sunTint    = sf::Color(255, 245, 190);
    dayPalette.sunGlow    = sf::Color(255, 230, 140, 70);
    dayPalette.moonTint   = sf::Color(220, 225, 235);
    dayPalette.cloudTop   = sf::Color(255, 255, 255);
    dayPalette.cloudMid   = sf::Color(200, 212, 228);
    dayPalette.cloudBase  = sf::Color(135, 150, 175);

    sunsetPalette.zenith    = sf::Color(28, 18, 52);
    sunsetPalette.midSky    = sf::Color(92, 38, 78);
    sunsetPalette.lowerSky  = sf::Color(190, 68, 65);
    sunsetPalette.horizon   = sf::Color(248, 148, 62);
    sunsetPalette.sunTint   = sf::Color(255, 175, 70);
    sunsetPalette.sunGlow   = sf::Color(255, 110, 45, 110);
    sunsetPalette.moonTint  = sf::Color(230, 210, 220);
    sunsetPalette.cloudTop  = sf::Color(255, 178, 130);
    sunsetPalette.cloudMid  = sf::Color(165, 75, 95);
    sunsetPalette.cloudBase = sf::Color(62, 35, 60);

    nightPalette.zenith     = sf::Color(6, 8, 16);
    nightPalette.midSky     = sf::Color(12, 16, 32);
    nightPalette.lowerSky   = sf::Color(20, 24, 46);
    nightPalette.horizon    = sf::Color(35, 34, 62);
    nightPalette.sunTint    = sf::Color(120, 130, 160);
    nightPalette.sunGlow    = sf::Color(0, 0, 0, 0);
    nightPalette.moonTint   = sf::Color(235, 240, 255);
    nightPalette.cloudTop   = sf::Color(70, 78, 105);
    nightPalette.cloudMid   = sf::Color(38, 44, 68);
    nightPalette.cloudBase  = sf::Color(18, 22, 38);

    dawnPalette.zenith    = sf::Color(20, 16, 42);
    dawnPalette.midSky    = sf::Color(68, 38, 75);
    dawnPalette.lowerSky  = sf::Color(160, 72, 95);
    dawnPalette.horizon   = sf::Color(235, 142, 120);
    dawnPalette.sunTint   = sf::Color(255, 205, 140);
    dawnPalette.sunGlow   = sf::Color(255, 130, 90, 85);
    dawnPalette.moonTint  = sf::Color(220, 210, 230);
    dawnPalette.cloudTop  = sf::Color(255, 195, 175);
    dawnPalette.cloudMid  = sf::Color(170, 95, 120);
    dawnPalette.cloudBase = sf::Color(72, 42, 68);

    overcastPalette.zenith    = sf::Color(28, 32, 42);
    overcastPalette.midSky    = sf::Color(48, 54, 66);
    overcastPalette.lowerSky  = sf::Color(72, 78, 90);
    overcastPalette.horizon   = sf::Color(105, 112, 122);
    overcastPalette.sunTint   = sf::Color(210, 205, 190);
    overcastPalette.sunGlow   = sf::Color(220, 215, 200, 30);
    overcastPalette.moonTint  = sf::Color(180, 185, 195);
    overcastPalette.cloudTop  = sf::Color(95, 102, 112);
    overcastPalette.cloudMid  = sf::Color(62, 68, 78);
    overcastPalette.cloudBase = sf::Color(35, 38, 46);
}

void SkySystem::generateCloudCluster(std::vector<ProceduralCloud>& layer, float startX, float layerDepth) {
    ProceduralCloud c;
    c.position = sf::Vector2f(startX, 40.f + static_cast<float>(std::rand() % 160));
    c.scale = (layerDepth < 0.5f) ? (0.75f + (std::rand() % 30) * 0.01f) : (1.0f + (std::rand() % 45) * 0.01f);
    c.speedMultiplier = (layerDepth < 0.5f) ? 0.35f : 0.70f;
    c.layerDepth = layerDepth;

    int subNodes = 5 + std::rand() % 6;
    float currentX = 0.f;

    for (int i = 0; i < subNodes; ++i) {
        CloudSegment seg;
        float w = (60.f + static_cast<float>(std::rand() % 80)) * c.scale;
        float h = (28.f + static_cast<float>(std::rand() % 32)) * c.scale;
        float offsetY = static_cast<float>((std::rand() % 24) - 12) * c.scale;

        seg.offset = sf::Vector2f(currentX, offsetY);
        seg.size = sf::Vector2f(w, h);
        c.segments.push_back(seg);

        currentX += w * 0.45f;
    }

    layer.push_back(c);
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
            static_cast<float>(std::rand() % static_cast<int>(height * 0.65f))
        );
        s.baseBrightness = 70.f + static_cast<float>(std::rand() % 185);
        s.twinkleSpeed = 1.0f + static_cast<float>(std::rand() % 35) * 0.1f;
        s.size = (std::rand() % 12 > 9) ? 2.0f : 1.0f;
        stars.push_back(s);
    }

    backgroundClouds.clear();
    foregroundClouds.clear();

    float spawnX = -200.f;
    while (spawnX < skyWidth + 400.f) {
        generateCloudCluster(backgroundClouds, spawnX, 0.3f);
        spawnX += 180.f + static_cast<float>(std::rand() % 160);
    }

    spawnX = -200.f;
    while (spawnX < skyWidth + 400.f) {
        generateCloudCluster(foregroundClouds, spawnX, 0.8f);
        spawnX += 240.f + static_cast<float>(std::rand() % 220);
    }

    const int bandCount = 18;
    skyBands.setPrimitiveType(sf::Quads);
    skyBands.resize(bandCount * 4);

    float bandH = skyHeight / static_cast<float>(bandCount);
    for (int i = 0; i < bandCount; ++i) {
        float y0 = i * bandH;
        float y1 = (i + 1) * bandH;
        int idx = i * 4;
        skyBands[idx + 0].position = sf::Vector2f(0.f, y0);
        skyBands[idx + 1].position = sf::Vector2f(skyWidth, y0);
        skyBands[idx + 2].position = sf::Vector2f(skyWidth, y1);
        skyBands[idx + 3].position = sf::Vector2f(0.f, y1);
    }

    starVertices.setPrimitiveType(sf::Quads);
    starVertices.resize(stars.size() * 4);
}

void SkySystem::setWeather(SkyWeather weather) {
    targetWeather = weather;
    weatherTransition = 0.0f;
}

void SkySystem::updateWeather(float dt) {
    weatherTimer += dt;
    if (weatherTimer > 90.0f) {
        weatherTimer = 0.f;
        int next = std::rand() % 4;
        setWeather(static_cast<SkyWeather>(next));
    }

    if (weatherTransition < 1.0f) {
        weatherTransition = std::min(1.0f, weatherTransition + dt * 0.08f);
        if (weatherTransition >= 1.0f) {
            currentWeather = targetWeather;
        }
    }

    auto getWeatherDensity = [](SkyWeather w) -> float {
        switch (w) {
            case SkyWeather::Clear: return 0.08f;
            case SkyWeather::Scattered: return 0.40f;
            case SkyWeather::Cloudy: return 0.75f;
            case SkyWeather::Overcast: return 1.00f;
        }
        return 0.4f;
    };

    float d1 = getWeatherDensity(currentWeather);
    float d2 = getWeatherDensity(targetWeather);
    cloudDensity = d1 + (d2 - d1) * weatherTransition;
}

sf::Color SkySystem::lerpColor(const sf::Color& a, const sf::Color& b, float t) const {
    return sf::Color(
        static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
        static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
        static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
        static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
    );
}

SkyPalette SkySystem::getBasePalette(float timeOfDay) const {
    if (timeOfDay < 0.20f) {
        float t = timeOfDay / 0.20f;
        return {
            lerpColor(nightPalette.zenith, dawnPalette.zenith, t),
            lerpColor(nightPalette.midSky, dawnPalette.midSky, t),
            lerpColor(nightPalette.lowerSky, dawnPalette.lowerSky, t),
            lerpColor(nightPalette.horizon, dawnPalette.horizon, t),
            lerpColor(nightPalette.sunTint, dawnPalette.sunTint, t),
            lerpColor(nightPalette.sunGlow, dawnPalette.sunGlow, t),
            lerpColor(nightPalette.moonTint, dawnPalette.moonTint, t),
            lerpColor(nightPalette.cloudTop, dawnPalette.cloudTop, t),
            lerpColor(nightPalette.cloudMid, dawnPalette.cloudMid, t),
            lerpColor(nightPalette.cloudBase, dawnPalette.cloudBase, t)
        };
    } else if (timeOfDay < 0.45f) {
        float t = (timeOfDay - 0.20f) / 0.25f;
        return {
            lerpColor(dawnPalette.zenith, dayPalette.zenith, t),
            lerpColor(dawnPalette.midSky, dayPalette.midSky, t),
            lerpColor(dawnPalette.lowerSky, dayPalette.lowerSky, t),
            lerpColor(dawnPalette.horizon, dayPalette.horizon, t),
            lerpColor(dawnPalette.sunTint, dayPalette.sunTint, t),
            lerpColor(dawnPalette.sunGlow, dayPalette.sunGlow, t),
            lerpColor(dawnPalette.moonTint, dayPalette.moonTint, t),
            lerpColor(dawnPalette.cloudTop, dayPalette.cloudTop, t),
            lerpColor(dawnPalette.cloudMid, dayPalette.cloudMid, t),
            lerpColor(dawnPalette.cloudBase, dayPalette.cloudBase, t)
        };
    } else if (timeOfDay < 0.70f) {
        float t = (timeOfDay - 0.45f) / 0.25f;
        return {
            lerpColor(dayPalette.zenith, sunsetPalette.zenith, t),
            lerpColor(dayPalette.midSky, sunsetPalette.midSky, t),
            lerpColor(dayPalette.lowerSky, sunsetPalette.lowerSky, t),
            lerpColor(dayPalette.horizon, sunsetPalette.horizon, t),
            lerpColor(dayPalette.sunTint, sunsetPalette.sunTint, t),
            lerpColor(dayPalette.sunGlow, sunsetPalette.sunGlow, t),
            lerpColor(dayPalette.moonTint, sunsetPalette.moonTint, t),
            lerpColor(dayPalette.cloudTop, sunsetPalette.cloudTop, t),
            lerpColor(dayPalette.cloudMid, sunsetPalette.cloudMid, t),
            lerpColor(dayPalette.cloudBase, sunsetPalette.cloudBase, t)
        };
    } else {
        float t = (timeOfDay - 0.70f) / 0.30f;
        return {
            lerpColor(sunsetPalette.zenith, nightPalette.zenith, t),
            lerpColor(sunsetPalette.midSky, nightPalette.midSky, t),
            lerpColor(sunsetPalette.lowerSky, nightPalette.lowerSky, t),
            lerpColor(sunsetPalette.horizon, nightPalette.horizon, t),
            lerpColor(sunsetPalette.sunTint, nightPalette.sunTint, t),
            lerpColor(sunsetPalette.sunGlow, nightPalette.sunGlow, t),
            lerpColor(sunsetPalette.moonTint, nightPalette.moonTint, t),
            lerpColor(sunsetPalette.cloudTop, nightPalette.cloudTop, t),
            lerpColor(sunsetPalette.cloudMid, nightPalette.cloudMid, t),
            lerpColor(sunsetPalette.cloudBase, nightPalette.cloudBase, t)
        };
    }
}

SkyPalette SkySystem::evaluatePalette(float timeOfDay) const {
    SkyPalette base = getBasePalette(timeOfDay);
    if (cloudDensity <= 0.65f) return base;

    float stormBlend = (cloudDensity - 0.65f) / 0.35f;
    return {
        lerpColor(base.zenith, overcastPalette.zenith, stormBlend),
        lerpColor(base.midSky, overcastPalette.midSky, stormBlend),
        lerpColor(base.lowerSky, overcastPalette.lowerSky, stormBlend),
        lerpColor(base.horizon, overcastPalette.horizon, stormBlend),
        lerpColor(base.sunTint, overcastPalette.sunTint, stormBlend),
        lerpColor(base.sunGlow, overcastPalette.sunGlow, stormBlend),
        lerpColor(base.moonTint, overcastPalette.moonTint, stormBlend),
        lerpColor(base.cloudTop, overcastPalette.cloudTop, stormBlend),
        lerpColor(base.cloudMid, overcastPalette.cloudMid, stormBlend),
        lerpColor(base.cloudBase, overcastPalette.cloudBase, stormBlend)
    };
}

void SkySystem::update(float dt, float timeOfDay, float cameraX) {
    totalTime += dt;
    updateWeather(dt);

    float baseWind = 9.5f;

    auto updateLayer = [this, dt, baseWind](std::vector<ProceduralCloud>& list, float wrapMargin) {
        for (auto& c : list) {
            c.position.x += baseWind * c.speedMultiplier * dt;
            if (c.position.x > skyWidth + wrapMargin) {
                float minX = skyWidth;
                for (const auto& other : list) {
                    if (other.position.x < minX) minX = other.position.x;
                }
                c.position.x = std::min(-wrapMargin, minX - (180.f + (std::rand() % 160)));
                c.position.y = 40.f + static_cast<float>(std::rand() % 160);
            }
        }
    };

    updateLayer(backgroundClouds, 300.f);
    updateLayer(foregroundClouds, 400.f);
}

void SkySystem::drawSky(sf::RenderTarget& target, float timeOfDay, float cameraX) {
    SkyPalette pal = evaluatePalette(timeOfDay);

    int bandCount = skyBands.getVertexCount() / 4;
    for (int i = 0; i < bandCount; ++i) {
        float normY = static_cast<float>(i) / static_cast<float>(bandCount - 1);
        sf::Color bandColor;

        if (normY < 0.33f) {
            float t = normY / 0.33f;
            bandColor = lerpColor(pal.zenith, pal.midSky, t);
        } else if (normY < 0.70f) {
            float t = (normY - 0.33f) / 0.37f;
            bandColor = lerpColor(pal.midSky, pal.lowerSky, t);
        } else {
            float t = (normY - 0.70f) / 0.30f;
            bandColor = lerpColor(pal.lowerSky, pal.horizon, t);
        }

        int idx = i * 4;
        skyBands[idx + 0].color = bandColor;
        skyBands[idx + 1].color = bandColor;
        skyBands[idx + 2].color = bandColor;
        skyBands[idx + 3].color = bandColor;
    }

    target.draw(skyBands);
}

void SkySystem::drawStars(sf::RenderTarget& target, float timeOfDay) {
    float nightFactor = 0.f;
    if (timeOfDay > 0.75f) {
        nightFactor = (timeOfDay - 0.75f) / 0.15f;
    } else if (timeOfDay < 0.25f) {
        nightFactor = 1.0f - (timeOfDay / 0.20f);
    }
    nightFactor = std::clamp(nightFactor, 0.f, 1.f) * (1.0f - cloudDensity * 0.85f);

    if (nightFactor <= 0.02f) return;

    for (size_t i = 0; i < stars.size(); ++i) {
        const auto& s = stars[i];
        float twinkle = (std::sin(totalTime * s.twinkleSpeed + s.position.x * 0.5f) + 1.f) * 0.5f;
        sf::Uint8 alpha = static_cast<sf::Uint8>(s.baseBrightness * (0.4f + 0.6f * twinkle) * nightFactor);

        sf::Color sc(220, 230, 255, alpha);
        int idx = i * 4;
        float sz = s.size;

        starVertices[idx + 0] = sf::Vertex(s.position, sc);
        starVertices[idx + 1] = sf::Vertex(s.position + sf::Vector2f(sz, 0.f), sc);
        starVertices[idx + 2] = sf::Vertex(s.position + sf::Vector2f(sz, sz), sc);
        starVertices[idx + 3] = sf::Vertex(s.position + sf::Vector2f(0.f, sz), sc);
    }

    target.draw(starVertices);
}

void SkySystem::drawCelestials(sf::RenderTarget& target, float timeOfDay, float cameraX) {
    SkyPalette pal = evaluatePalette(timeOfDay);

    float sunT = (timeOfDay - 0.20f) / 0.55f;
    if (sunT >= -0.08f && sunT <= 1.08f) {
        float rad = std::clamp(sunT, 0.0f, 1.0f) * 3.14159265f;
        float sunX = 140.f + (skyWidth - 280.f) * sunT;
        float sunY = (skyHeight * 0.86f) - std::sin(rad) * (skyHeight * 0.72f);

        float horizonProximity = 1.0f - std::sin(rad);

        for (int ring = 4; ring >= 1; --ring) {
            float r = 36.f + ring * 18.f;
            sf::CircleShape glow(r);
            glow.setOrigin(r, r);
            glow.setPosition(sunX, sunY);

            sf::Color gc = pal.sunGlow;
            gc.a = static_cast<sf::Uint8>((pal.sunGlow.a / ring) * (1.0f - cloudDensity * 0.6f));
            glow.setFillColor(gc);
            target.draw(glow);
        }

        if (horizonProximity > 0.35f) {
            float hazeW = 280.f + horizonProximity * 220.f;
            float hazeH = 14.f + horizonProximity * 12.f;
            sf::RectangleShape haze(sf::Vector2f(hazeW, hazeH));
            haze.setOrigin(hazeW * 0.5f, hazeH * 0.5f);
            haze.setPosition(sunX, sunY + 6.f);
            sf::Color hc = pal.sunTint;
            hc.a = static_cast<sf::Uint8>(45.f * horizonProximity * (1.0f - cloudDensity * 0.7f));
            haze.setFillColor(hc);
            target.draw(haze);
        }

        sf::CircleShape sun(34.f);
        sun.setOrigin(34.f, 34.f);
        sun.setPosition(sunX, sunY);
        sun.setFillColor(pal.sunTint);
        target.draw(sun);

        sf::CircleShape core(24.f);
        core.setOrigin(24.f, 24.f);
        core.setPosition(sunX, sunY);
        core.setFillColor(sf::Color(255, 255, 245, 220));
        target.draw(core);
    }

    float moonT = (timeOfDay >= 0.72f) ? ((timeOfDay - 0.72f) / 0.52f) : ((timeOfDay + 0.28f) / 0.52f);
    if (moonT >= -0.05f && moonT <= 1.05f) {
        float rad = std::clamp(moonT, 0.0f, 1.0f) * 3.14159265f;
        float moonX = 140.f + (skyWidth - 280.f) * moonT;
        float moonY = (skyHeight * 0.86f) - std::sin(rad) * (skyHeight * 0.72f);

        sf::CircleShape moonGlow(46.f);
        moonGlow.setOrigin(46.f, 46.f);
        moonGlow.setPosition(moonX, moonY);
        moonGlow.setFillColor(sf::Color(160, 190, 240, static_cast<sf::Uint8>(38 * (1.0f - cloudDensity * 0.7f))));
        target.draw(moonGlow);

        sf::CircleShape moon(28.f);
        moon.setOrigin(28.f, 28.f);
        moon.setPosition(moonX, moonY);
        moon.setFillColor(pal.moonTint);
        target.draw(moon);

        sf::CircleShape crater1(4.5f);
        crater1.setOrigin(4.5f, 4.5f);
        crater1.setPosition(moonX - 7.f, moonY - 6.f);
        crater1.setFillColor(sf::Color(180, 190, 205, 140));
        target.draw(crater1);

        sf::CircleShape crater2(6.5f);
        crater2.setOrigin(6.5f, 6.5f);
        crater2.setPosition(moonX + 5.f, moonY + 4.f);
        crater2.setFillColor(sf::Color(180, 190, 205, 120));
        target.draw(crater2);

        sf::CircleShape crater3(3.5f);
        crater3.setOrigin(3.5f, 3.5f);
        crater3.setPosition(moonX - 8.f, moonY + 9.f);
        crater3.setFillColor(sf::Color(180, 190, 205, 130));
        target.draw(crater3);

        sf::CircleShape moonShadow(25.f);
        moonShadow.setOrigin(25.f, 25.f);
        moonShadow.setPosition(moonX + 9.f, moonY - 4.f);
        moonShadow.setFillColor(pal.zenith);
        target.draw(moonShadow);
    }
}

void SkySystem::drawClouds(sf::RenderTarget& target, float timeOfDay, float cameraX) {
    if (cloudDensity <= 0.05f) return;

    SkyPalette pal = evaluatePalette(timeOfDay);

    auto renderList = [&](const std::vector<ProceduralCloud>& list, float parallax, float alphaScale) {
        size_t activeCount = static_cast<size_t>(list.size() * std::clamp(cloudDensity * 1.3f, 0.15f, 1.0f));
        activeCount = std::min(activeCount, list.size());

        for (size_t i = 0; i < activeCount; ++i) {
            const auto& c = list[i];
            float rx = c.position.x - (cameraX * parallax);
            rx = std::fmod(rx, skyWidth + 500.f);
            if (rx < -450.f) rx += (skyWidth + 500.f);

            for (const auto& seg : c.segments) {
                sf::Vector2f sp = sf::Vector2f(rx + seg.offset.x, c.position.y + seg.offset.y);

                sf::RectangleShape baseRect(seg.size);
                baseRect.setOrigin(seg.size.x * 0.5f, seg.size.y * 0.5f);
                baseRect.setPosition(sp.x, sp.y + seg.size.y * 0.22f);
                sf::Color baseC = pal.cloudBase;
                baseC.a = static_cast<sf::Uint8>(baseC.a * alphaScale);
                baseRect.setFillColor(baseC);
                target.draw(baseRect);

                sf::RectangleShape midRect(sf::Vector2f(seg.size.x * 0.92f, seg.size.y * 0.72f));
                midRect.setOrigin(midRect.getSize().x * 0.5f, midRect.getSize().y * 0.5f);
                midRect.setPosition(sp.x, sp.y);
                sf::Color midC = pal.cloudMid;
                midC.a = static_cast<sf::Uint8>(midC.a * alphaScale);
                midRect.setFillColor(midC);
                target.draw(midRect);

                sf::RectangleShape topRect(sf::Vector2f(seg.size.x * 0.78f, seg.size.y * 0.45f));
                topRect.setOrigin(topRect.getSize().x * 0.5f, topRect.getSize().y * 0.5f);
                topRect.setPosition(sp.x, sp.y - seg.size.y * 0.20f);
                sf::Color topC = pal.cloudTop;
                topC.a = static_cast<sf::Uint8>(topC.a * alphaScale);
                topRect.setFillColor(topC);
                target.draw(topRect);
            }
        }
    };

    renderList(backgroundClouds, 0.015f, 0.72f);
    renderList(foregroundClouds, 0.038f, 0.95f);
}