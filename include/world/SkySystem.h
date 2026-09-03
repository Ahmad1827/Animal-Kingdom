#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

enum class SkyWeather {
    Clear,
    Scattered,
    Cloudy,
    Overcast
};

struct Star {
    sf::Vector2f position;
    float baseBrightness;
    float twinkleSpeed;
    float size;
};

struct CloudSegment {
    sf::Vector2f offset;
    sf::Vector2f size;
};

struct ProceduralCloud {
    sf::Vector2f position;
    float speedMultiplier;
    float scale;
    float layerDepth;
    std::vector<CloudSegment> segments;
};

struct SkyPalette {
    sf::Color zenith;
    sf::Color midSky;
    sf::Color lowerSky;
    sf::Color horizon;
    sf::Color sunTint;
    sf::Color sunGlow;
    sf::Color moonTint;
    sf::Color cloudTop;
    sf::Color cloudMid;
    sf::Color cloudBase;
};

class SkySystem {
private:
    float skyWidth = 1280.f;
    float skyHeight = 540.f;
    float totalTime = 0.f;

    SkyWeather currentWeather = SkyWeather::Scattered;
    SkyWeather targetWeather = SkyWeather::Cloudy;
    float weatherTransition = 1.0f;
    float weatherTimer = 0.f;
    float cloudDensity = 0.45f;

    std::vector<Star> stars;
    std::vector<ProceduralCloud> backgroundClouds;
    std::vector<ProceduralCloud> foregroundClouds;

    sf::VertexArray skyBands;
    sf::VertexArray starVertices;

    SkyPalette dayPalette;
    SkyPalette sunsetPalette;
    SkyPalette nightPalette;
    SkyPalette dawnPalette;
    SkyPalette overcastPalette;

    sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t) const;
    SkyPalette getBasePalette(float timeOfDay) const;
    SkyPalette evaluatePalette(float timeOfDay) const;
    void generateCloudCluster(std::vector<ProceduralCloud>& layer, float startX, float layerDepth);
    void updateWeather(float dt);

public:
    SkySystem();
    void init(float width, float height, int starCount);
    void setWeather(SkyWeather weather);
    void update(float dt, float timeOfDay, float cameraX);
    void drawSky(sf::RenderTarget& target, float timeOfDay, float cameraX);
    void drawCelestials(sf::RenderTarget& target, float timeOfDay, float cameraX);
    void drawStars(sf::RenderTarget& target, float timeOfDay);
    void drawClouds(sf::RenderTarget& target, float timeOfDay, float cameraX);
};