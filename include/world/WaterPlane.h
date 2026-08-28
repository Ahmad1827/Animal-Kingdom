#pragma once
#include <SFML/Graphics.hpp>

// Kingdom-style reflective water plane.
//
// This is a *screen-space* effect, not a world object. It takes the frame you
// already rendered (as a texture), mirrors the part above the waterline into
// the part below it, wobbles it, tints it, and lays a surface line on top.
//
// It draws nothing into the simulation and touches no collision. The ape still
// walks on FLAT_GROUND_Y; the water just covers the soil below it, which is
// what gives you Kingdom's thin-island silhouette without changing terrain.
//
// Cost: ONE draw call for the whole reflection (a single textured VertexArray),
// plus 3 small ones for tint / surface / sparkles.
class WaterPlane {
public:
    WaterPlane();

    void update(float dt);

    // surfaceWorldY: world Y where the water surface sits. Everything below
    // this line on screen becomes water. Default 610 = 110px of visible soil
    // under the ape's feet (FLAT_GROUND_Y is 500).
    void setSurfaceWorldY(float y);
    float getSurfaceWorldY() const;

    // Tuning knobs, all safe to change at runtime.
    void setReflectionDepth(float px);   // how far down the mirror image reaches
    void setReflectionAlpha(float a);    // 0..1, opacity of the mirror at the surface
    void setWaveAmplitude(float px);
    void setWaveSpeed(float s);
    void setDeepColor(const sf::Color& c);
    void setShallowColor(const sf::Color& c);

    // sceneTexture must be the frame rendered THIS tick, at window resolution,
    // through the same cameraView. skyTint comes from DayNightCycle so the
    // water darkens at night along with everything else.
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

    // Converts a world Y to a screen Y for the given view. Ignores view
    // rotation - camera shake tops out around 4 degrees and a waterline that
    // stays axis-aligned during a shake looks better than one that tilts.
    static float worldToScreenY(float worldY, const sf::View& view, float targetHeight);

    void drawReflection(sf::RenderTarget& target, const sf::Texture& sceneTex,
                        float surfaceY, float w, float h, sf::Color skyTint) const;
    void drawBody(sf::RenderTarget& target, float surfaceY, float w, float h, sf::Color skyTint) const;
    void drawSurfaceLine(sf::RenderTarget& target, float surfaceY, float w, sf::Color skyTint) const;
    void drawSparkles(sf::RenderTarget& target, float surfaceY, float w, float h,
                      const sf::View& cameraView, sf::Color skyTint) const;
};