#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct Star {
    sf::Vector2f position;
    float baseBrightness;
    float twinkleSpeed;
    float size;
};

struct CloudTier {
    sf::Vector2f position;
    float speed;
    float scale;
    sf::Color tint;
    sf::IntRect spriteRect;
};

struct SkyPalette {
    sf::Color zenith;
    sf::Color midSky;
    sf::Color horizon;
    sf::Color sunTint;
    sf::Color ambientLight;
};

class SkySystem {
private:
    std::vector<Star> stars;
    std::vector<CloudTier> clouds;
    sf::VertexArray skyBands;
    sf::Texture ditherPattern;

    sf::CircleShape sunShape;
    sf::CircleShape sunHalo;
    sf::CircleShape moonShape;
    sf::CircleShape moonCrescentCut;

    SkyPalette dayPalette;
    SkyPalette sunsetPalette;
    SkyPalette nightPalette;
    SkyPalette dawnPalette;

    float skyWidth = 1280.f;
    float skyHeight = 500.f;
    float starTime = 0.f;

    sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t) const;
    SkyPalette evaluateCurrentPalette(float timeOfDay) const;

public:
    SkySystem();
    void init(float width, float height, int starCount);
    void update(float dt, float timeOfDay, float cameraX);
    void drawSky(sf::RenderTarget& target, float timeOfDay, float cameraX);
    void drawCelestials(sf::RenderTarget& target, float timeOfDay, float cameraX);
    void drawStars(sf::RenderTarget& target, float timeOfDay);
    void drawClouds(sf::RenderTarget& target, const sf::Texture* cloudTexture);
};