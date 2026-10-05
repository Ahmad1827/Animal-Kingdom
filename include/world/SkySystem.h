#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// ---------------------------------------------------------------------------
// Pixel-art sky. Everything is drawn on a coarse grid of SKY_PIXEL x SKY_PIXEL
// screen units so the sky has the same chunky look as the sprites:
//   - a banded, dithered gradient that reaches the horizon colours where the
//     hills start (not at the bottom of the screen, where nobody sees it)
//   - a dithered glow around the sun and moon
//   - shaped cumulus clouds with three tones, pre-drawn once at init
//   - weather controls how MANY clouds are out, instead of fading them all
// The public interface is unchanged, so PlayState needs no edits.
// ---------------------------------------------------------------------------

enum class SkyWeather {
    Clear,
    Scattered,
    Cloudy,
    Overcast
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
public:
    // Size of one sky pixel in view units. Use 2, 4 or 8 (must divide 1280 and 720).
    // 2 = fine, close to the mountain art.  4 = chunkier, more retro.
    static constexpr int SKY_PIXEL = 2;
    // Where the gradient reaches the horizon colour, as a fraction of the view
    // height. Roughly where the distant hills begin on screen.
    static constexpr float HORIZON_FRACTION = 0.40f;

    SkySystem();
    void init(float width, float height, int starCount);
    void setWeather(SkyWeather weather);
    void update(float dt, float timeOfDay, float cameraX);
    void drawSky(sf::RenderTarget& target, float timeOfDay, float cameraX);
    void drawCelestials(sf::RenderTarget& target, float timeOfDay, float cameraX);
    void drawStars(sf::RenderTarget& target, float timeOfDay);
    void drawClouds(sf::RenderTarget& target, float timeOfDay, float cameraX);

private:
    struct Star {
        sf::Vector2i cell;          // position on the sky-pixel grid
        float brightness;           // 0..1
        float twinkleSpeed;
        float phase;
        bool  big;                  // drawn as a small plus
        sf::Color tint;
    };

    struct CloudShape {             // one pre-drawn cloud in the atlas
        sf::IntRect base, mid, top; // three tone layers, same size
    };

    struct Cloud {
        float x, y;                 // view units
        float speed;                // wind multiplier
        float parallax;
        float threshold;            // appears once cloudDensity passes this
        int   shape;
        bool  far;
    };

    struct Body {                   // sun or moon this frame
        bool  visible = false;
        float x = 0.f, y = 0.f;     // view units
        float height = 0.f;         // 0 at the horizon, 1 at the top of the arc
    };

    float skyWidth = 1280.f;
    float skyHeight = 540.f;
    float totalTime = 0.f;

    SkyWeather currentWeather = SkyWeather::Scattered;
    SkyWeather targetWeather = SkyWeather::Cloudy;
    float weatherTransition = 1.0f;
    float weatherTimer = 0.f;
    float cloudDensity = 0.45f;

    std::vector<Star> stars;
    std::vector<CloudShape> nearShapes, farShapes;
    std::vector<Cloud> clouds;
    float cloudSpan = 2000.f;
    float cloudMargin = 300.f;

    sf::Texture cloudAtlas;
    sf::Texture bodyAtlas;          // sun disc, sun core, moon dark side, moon lit side
    sf::IntRect sunDiscRect, sunCoreRect, moonDarkRect, moonLitRect;

    sf::Texture gradientTexture;
    std::vector<sf::Uint8> gradientPixels;
    sf::Vector2u gradientSize;

    SkyPalette dayPalette;
    SkyPalette sunsetPalette;
    SkyPalette nightPalette;
    SkyPalette dawnPalette;
    SkyPalette overcastPalette;

    sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t) const;
    SkyPalette lerpPalette(const SkyPalette& a, const SkyPalette& b, float t) const;
    SkyPalette getBasePalette(float timeOfDay) const;
    SkyPalette evaluatePalette(float timeOfDay) const;
    sf::Color skyColorAt(const SkyPalette& pal, float heightFraction) const;

    Body sunAt(float timeOfDay) const;
    Body moonAt(float timeOfDay) const;
    float nightAmount(float timeOfDay) const;

    void buildCloudAtlas();
    void buildBodyAtlas();
    void updateWeather(float dt);
};