#pragma once
#include <SFML/Graphics.hpp>

class WaterPlane {
public:
    WaterPlane();

    void update(float dt);

    void setSurfaceWorldY(float y);
    float getSurfaceWorldY() const;

    void setReflectionDepth(float px);
    void setReflectionAlpha(float a);
    void setWaveAmplitude(float px);
    void setWaveSpeed(float s);
    void setDeepColor(const sf::Color& c);
    void setShallowColor(const sf::Color& c);

    void draw(sf::RenderTarget& target,
              const sf::Texture& sceneTexture,
              const sf::View& cameraView,
              sf::Color skyTint = sf::Color::White) const;

private:
    float time;
    float surfaceWorldY;

    float reflectionDepthPx;
    float reflectionAlpha;
    float stripHeightPx;

    float waveAmplitude;
    float waveSpeed;
    float waveFrequency;

    sf::Color shallowColor;
    sf::Color deepColor;

    static float worldToScreenY(float worldY, const sf::View& view, float targetHeight);

    void drawBody(sf::RenderTarget& target, float surfaceY, float w, float h, sf::Color skyTint) const;
    void drawReflection(sf::RenderTarget& target, const sf::Texture& sceneTex,
                        float surfaceY, float w, float h) const;
    void drawSurfaceLine(sf::RenderTarget& target, float surfaceY, float w, sf::Color skyTint) const;
    void drawSparkles(sf::RenderTarget& target, float surfaceY, float w, float h,
                      const sf::View& cameraView, sf::Color skyTint) const;
};