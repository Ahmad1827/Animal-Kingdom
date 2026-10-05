#include "world/SkySystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace {

const float PI = 3.14159265f;
const int   P  = SkySystem::SKY_PIXEL;

// 4x4 ordered-dither matrix, values 0..15.
const int BAYER[4][4] = {
    { 0,  8,  2, 10},
    {12,  4, 14,  6},
    { 3, 11,  1,  9},
    {15,  7, 13,  5},
};
inline float bayer(int x, int y) { return (static_cast<float>(BAYER[y & 3][x & 3]) + 0.5f) / 16.f; }

inline float frand() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }
inline float frand(float a, float b) { return a + (b - a) * frand(); }

// Snap a view coordinate to the sky-pixel grid.
inline float snap(float v) { return std::floor(v / static_cast<float>(P)) * static_cast<float>(P); }

const int GRADIENT_STEPS = 16;   // colour bands between zenith and horizon
const int GLOW_STEPS = 8;

struct Puff { float cx, cy, r; };

} // namespace

SkySystem::SkySystem() {
    dayPalette.zenith     = sf::Color(34, 84, 164);
    dayPalette.midSky     = sf::Color(72, 138, 208);
    dayPalette.lowerSky   = sf::Color(134, 190, 230);
    dayPalette.horizon    = sf::Color(206, 230, 236);
    dayPalette.sunTint    = sf::Color(255, 244, 186);
    dayPalette.sunGlow    = sf::Color(255, 240, 190, 80);
    dayPalette.moonTint   = sf::Color(220, 225, 235);
    dayPalette.cloudTop   = sf::Color(255, 255, 255);
    dayPalette.cloudMid   = sf::Color(214, 226, 240);
    dayPalette.cloudBase  = sf::Color(160, 180, 212);

    sunsetPalette.zenith    = sf::Color(34, 26, 70);
    sunsetPalette.midSky    = sf::Color(104, 48, 96);
    sunsetPalette.lowerSky  = sf::Color(206, 84, 76);
    sunsetPalette.horizon   = sf::Color(250, 164, 72);
    sunsetPalette.sunTint   = sf::Color(255, 190, 90);
    sunsetPalette.sunGlow   = sf::Color(255, 150, 60, 150);
    sunsetPalette.moonTint  = sf::Color(230, 210, 220);
    sunsetPalette.cloudTop  = sf::Color(255, 186, 128);
    sunsetPalette.cloudMid  = sf::Color(196, 96, 104);
    sunsetPalette.cloudBase = sf::Color(92, 50, 88);

    nightPalette.zenith     = sf::Color(8, 10, 24);
    nightPalette.midSky     = sf::Color(14, 20, 44);
    nightPalette.lowerSky   = sf::Color(24, 34, 68);
    nightPalette.horizon    = sf::Color(44, 54, 96);
    nightPalette.sunTint    = sf::Color(120, 130, 160);
    nightPalette.sunGlow    = sf::Color(0, 0, 0, 0);
    nightPalette.moonTint   = sf::Color(236, 240, 255);
    nightPalette.cloudTop   = sf::Color(62, 74, 112);
    nightPalette.cloudMid   = sf::Color(36, 46, 78);
    nightPalette.cloudBase  = sf::Color(20, 26, 50);

    dawnPalette.zenith    = sf::Color(30, 30, 76);
    dawnPalette.midSky    = sf::Color(96, 62, 118);
    dawnPalette.lowerSky  = sf::Color(214, 112, 122);
    dawnPalette.horizon   = sf::Color(252, 188, 134);
    dawnPalette.sunTint   = sf::Color(255, 214, 150);
    dawnPalette.sunGlow   = sf::Color(255, 176, 110, 130);
    dawnPalette.moonTint  = sf::Color(220, 210, 230);
    dawnPalette.cloudTop  = sf::Color(255, 206, 178);
    dawnPalette.cloudMid  = sf::Color(204, 124, 140);
    dawnPalette.cloudBase = sf::Color(104, 66, 104);

    overcastPalette.zenith    = sf::Color(44, 50, 64);
    overcastPalette.midSky    = sf::Color(66, 74, 90);
    overcastPalette.lowerSky  = sf::Color(96, 104, 118);
    overcastPalette.horizon   = sf::Color(134, 142, 150);
    overcastPalette.sunTint   = sf::Color(214, 210, 196);
    overcastPalette.sunGlow   = sf::Color(220, 215, 200, 40);
    overcastPalette.moonTint  = sf::Color(180, 185, 195);
    overcastPalette.cloudTop  = sf::Color(150, 158, 170);
    overcastPalette.cloudMid  = sf::Color(108, 116, 130);
    overcastPalette.cloudBase = sf::Color(70, 76, 90);
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void SkySystem::buildCloudAtlas() {
    // Each cloud is a row of overlapping round puffs sitting on a flat base.
    // Three masks are cut from the same puffs: the full silhouette (darkest,
    // shows as the underside), the puffs nudged up a little (shadow tone) and
    // nudged up-left further (the lit bulk). Drawn in that order, each puff
    // keeps a crescent of shade underneath it.
    struct Spec { int w, h; bool far; };
    std::vector<Spec> specs;
    for (int i = 0; i < 10; ++i) {          // near clouds: big
        int w = static_cast<int>(frand(190.f, 420.f)) / P;
        int h = static_cast<int>(static_cast<float>(w) * frand(0.24f, 0.32f));
        specs.push_back({w, std::max(h, 8), false});
    }
    for (int i = 0; i < 8; ++i) {           // far clouds: small and flat
        int w = static_cast<int>(frand(90.f, 220.f)) / P;
        int h = static_cast<int>(static_cast<float>(w) * frand(0.20f, 0.28f));
        specs.push_back({w, std::max(h, 5), true});
    }

    unsigned atlasW = 0, atlasH = 0;
    for (const auto& s : specs) {
        atlasW = std::max(atlasW, static_cast<unsigned>(s.w * 3 + 3));
        atlasH += static_cast<unsigned>(s.h + 1);
    }

    sf::Image img;
    img.create(atlasW, atlasH, sf::Color::Transparent);
    nearShapes.clear();
    farShapes.clear();

    int rowY = 0;
    for (const auto& s : specs) {
        const float w = static_cast<float>(s.w), h = static_cast<float>(s.h);
        const float baseY = h - 1.f;

        // A row of overlapping puffs, tallest near `peak`, all dipping below the
        // base line so the bottom comes out flat. A few larger puffs are stacked
        // on top around the peak to break up the outline.
        std::vector<Puff> puffs;
        const int count = std::max(4, static_cast<int>(w / (h * 0.40f)));
        const float peak = frand(0.32f, 0.68f);
        auto hillAt = [&](float u) {
            const float d = std::abs(u - peak) / std::max(peak, 1.f - peak);
            return std::pow(std::max(0.f, 1.f - d), 0.8f);
        };
        for (int i = 0; i < count; ++i) {
            const float u = std::clamp((static_cast<float>(i) + frand(-0.3f, 0.3f)) / static_cast<float>(count - 1), 0.f, 1.f);
            const float r = std::max(2.5f, h * (0.17f + 0.25f * hillAt(u)) * frand(0.8f, 1.2f));
            const float cx = r + u * (w - 2.f * r);
            puffs.push_back({cx, baseY - r * frand(0.30f, 0.70f), r});
        }
        std::sort(puffs.begin(), puffs.end(), [](const Puff& a, const Puff& b) { return a.cx < b.cx; });
        const int crown = 1 + std::rand() % 3;
        for (int i = 0; i < crown; ++i) {
            const float u = std::clamp(peak + frand(-0.22f, 0.22f), 0.12f, 0.88f);
            const float r = h * frand(0.26f, 0.36f);
            const float cx = std::clamp(u * w, r, w - r);
            puffs.push_back({cx, r + frand(0.f, h * 0.12f), r});
        }

        auto inside = [&](float x, float y, float dx, float dy, float rs) {
            for (const auto& p : puffs) {
                const float ox = x - (p.cx + dx * p.r), oy = y - (p.cy + dy * p.r);
                const float rr = p.r * rs;
                if (ox * ox + oy * oy <= rr * rr) return true;
            }
            return false;
        };

        CloudShape shape;
        shape.base = sf::IntRect(0, rowY, s.w, s.h);
        shape.mid  = sf::IntRect(s.w + 1, rowY, s.w, s.h);
        shape.top  = sf::IntRect((s.w + 1) * 2, rowY, s.w, s.h);

        float left = w, right = 0.f;
        for (const auto& pf : puffs) { left = std::min(left, pf.cx); right = std::max(right, pf.cx); }
        for (int y = 0; y < s.h; ++y) {
            for (int x = 0; x < s.w; ++x) {
                const float fx = static_cast<float>(x) + 0.5f, fy = static_cast<float>(y) + 0.5f;
                bool body = inside(fx, fy, 0.f, 0.f, 1.f);
                // flat base slab joining the puffs
                if (!body && fy >= baseY - 1.5f && fx >= left && fx <= right) body = true;
                if (!body || fy > baseY + 0.5f) continue;

                img.setPixel(shape.base.left + x, rowY + y, sf::Color::White);
                if (inside(fx, fy, 0.f, -0.10f, 1.f) && fy < baseY - 0.5f)
                    img.setPixel(shape.mid.left + x, rowY + y, sf::Color::White);
                if (inside(fx, fy, -0.16f, -0.36f, 0.78f) && fy < baseY - 1.5f)
                    img.setPixel(shape.top.left + x, rowY + y, sf::Color::White);
            }
        }

        // Tidy up: where two puffs' shading meets it can leave a speck of the
        // darker tone. Find small enclosed patches and paint them over.
        auto fillPockets = [&](const sf::IntRect& layer) {
            std::vector<char> seen(static_cast<size_t>(s.w * s.h), 0);
            auto open = [&](int x, int y) {
                return img.getPixel(static_cast<unsigned>(shape.base.left + x), static_cast<unsigned>(rowY + y)).a != 0 &&
                       img.getPixel(static_cast<unsigned>(layer.left + x), static_cast<unsigned>(rowY + y)).a == 0;
            };
            for (int y = 0; y < s.h; ++y) {
                for (int x = 0; x < s.w; ++x) {
                    if (seen[static_cast<size_t>(y * s.w + x)] || !open(x, y)) continue;
                    std::vector<sf::Vector2i> patch{ {x, y} };
                    seen[static_cast<size_t>(y * s.w + x)] = 1;
                    for (size_t i = 0; i < patch.size(); ++i) {
                        const sf::Vector2i dirs[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
                        for (const auto& d : dirs) {
                            const int nx = patch[i].x + d.x, ny = patch[i].y + d.y;
                            if (nx < 0 || ny < 0 || nx >= s.w || ny >= s.h) continue;
                            if (seen[static_cast<size_t>(ny * s.w + nx)] || !open(nx, ny)) continue;
                            seen[static_cast<size_t>(ny * s.w + nx)] = 1;
                            patch.push_back({nx, ny});
                        }
                    }
                    if (patch.size() <= 14) {
                        for (const auto& q : patch)
                            img.setPixel(static_cast<unsigned>(layer.left + q.x), static_cast<unsigned>(rowY + q.y), sf::Color::White);
                    }
                }
            }
        };
        fillPockets(shape.mid);
        fillPockets(shape.top);

        (s.far ? farShapes : nearShapes).push_back(shape);
        rowY += s.h + 1;
    }

    cloudAtlas.loadFromImage(img);
    cloudAtlas.setSmooth(false);
}

void SkySystem::buildBodyAtlas() {
    const int sunR  = std::max(4, 30 / P);
    const int coreR = std::max(3, 22 / P);
    const int moonR = std::max(4, 26 / P);

    const int sunD = sunR * 2 + 1, coreD = coreR * 2 + 1, moonD = moonR * 2 + 1;
    sf::Image img;
    img.create(static_cast<unsigned>(sunD + coreD + moonD * 2 + 3),
               static_cast<unsigned>(std::max(sunD, moonD)), sf::Color::Transparent);

    auto disc = [&](int ox, int r, sf::Color c) {
        for (int y = -r; y <= r; ++y)
            for (int x = -r; x <= r; ++x)
                if (x * x + y * y <= r * r + r / 2)
                    img.setPixel(static_cast<unsigned>(ox + r + x), static_cast<unsigned>(r + y), c);
    };

    int ox = 0;
    sunDiscRect = sf::IntRect(ox, 0, sunD, sunD);   disc(ox, sunR, sf::Color::White);  ox += sunD + 1;
    sunCoreRect = sf::IntRect(ox, 0, coreD, coreD); disc(ox, coreR, sf::Color::White); ox += coreD + 1;
    moonDarkRect = sf::IntRect(ox, 0, moonD, moonD); disc(ox, moonR, sf::Color::White); ox += moonD + 1;

    // Lit crescent: the disc minus a second disc pushed up and to the right.
    moonLitRect = sf::IntRect(ox, 0, moonD, moonD);
    const float mr = static_cast<float>(moonR);
    const float sx = mr * 0.42f, sy = -mr * 0.16f, sr = mr * 0.92f;
    struct Crater { float x, y, r; };
    const Crater craters[] = { {-0.52f, -0.20f, 0.17f}, {-0.30f, 0.42f, 0.22f}, {-0.66f, 0.26f, 0.11f} };
    for (int y = -moonR; y <= moonR; ++y) {
        for (int x = -moonR; x <= moonR; ++x) {
            if (x * x + y * y > moonR * moonR + moonR / 2) continue;
            const float fx = static_cast<float>(x), fy = static_cast<float>(y);
            if ((fx - sx) * (fx - sx) + (fy - sy) * (fy - sy) <= sr * sr) continue;
            sf::Color c = sf::Color::White;
            for (const auto& cr : craters) {
                const float dx = fx - cr.x * mr, dy = fy - cr.y * mr;
                if (dx * dx + dy * dy <= (cr.r * mr) * (cr.r * mr)) c = sf::Color(196, 204, 226);
            }
            img.setPixel(static_cast<unsigned>(ox + moonR + x), static_cast<unsigned>(moonR + y), c);
        }
    }

    bodyAtlas.loadFromImage(img);
    bodyAtlas.setSmooth(false);
}

void SkySystem::init(float width, float height, int starCount) {
    skyWidth = width;
    skyHeight = height;

    // Stars live on the sky-pixel grid, thinning out toward the horizon.
    stars.clear();
    stars.reserve(static_cast<size_t>(starCount));
    const int cellsX = static_cast<int>(width) / P;
    const int cellsY = static_cast<int>(height * 0.62f) / P;
    for (int i = 0; i < starCount; ++i) {
        Star s;
        const float fy = frand();
        s.cell = sf::Vector2i(std::rand() % std::max(1, cellsX),
                              static_cast<int>(fy * fy * static_cast<float>(cellsY)));
        s.brightness = frand(0.35f, 1.0f);
        s.twinkleSpeed = frand(0.6f, 2.6f);
        s.phase = frand(0.f, 2.f * PI);
        s.big = (std::rand() % 14 == 0);
        const int hue = std::rand() % 10;
        s.tint = hue < 6 ? sf::Color(226, 234, 255) : (hue < 8 ? sf::Color(255, 238, 206) : sf::Color(190, 210, 255));
        stars.push_back(s);
    }

    buildCloudAtlas();
    buildBodyAtlas();

    // Scatter clouds over a strip wider than the screen so they wrap unseen.
    cloudMargin = 440.f;
    cloudSpan = skyWidth + cloudMargin * 2.f;
    clouds.clear();

    const int farCount = 12, nearCount = 9;
    auto scatter = [&](int count, bool far) {
        std::vector<float> thresholds;
        for (int i = 0; i < count; ++i)
            thresholds.push_back(0.04f + 0.86f * (static_cast<float>(i) + 0.5f) / static_cast<float>(count));
        for (int i = count - 1; i > 0; --i) std::swap(thresholds[i], thresholds[std::rand() % (i + 1)]);

        const auto& shapes = far ? farShapes : nearShapes;
        for (int i = 0; i < count; ++i) {
            Cloud c;
            c.far = far;
            c.shape = std::rand() % static_cast<int>(shapes.size());
            c.x = cloudSpan * (static_cast<float>(i) + frand(0.1f, 0.9f)) / static_cast<float>(count);
            c.y = far ? frand(height * 0.16f, height * 0.50f) : frand(height * 0.04f, height * 0.36f);
            c.speed = far ? frand(0.28f, 0.40f) : frand(0.60f, 0.85f);
            c.parallax = far ? 0.015f : 0.038f;
            c.threshold = thresholds[static_cast<size_t>(i)];
            clouds.push_back(c);
        }
    };
    scatter(farCount, true);
    scatter(nearCount, false);
}

// ---------------------------------------------------------------------------
// Weather and palette
// ---------------------------------------------------------------------------

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
    t = std::clamp(t, 0.f, 1.f);
    return sf::Color(
        static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
        static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
        static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
        static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
    );
}

SkyPalette SkySystem::lerpPalette(const SkyPalette& a, const SkyPalette& b, float t) const {
    return {
        lerpColor(a.zenith, b.zenith, t),     lerpColor(a.midSky, b.midSky, t),
        lerpColor(a.lowerSky, b.lowerSky, t), lerpColor(a.horizon, b.horizon, t),
        lerpColor(a.sunTint, b.sunTint, t),   lerpColor(a.sunGlow, b.sunGlow, t),
        lerpColor(a.moonTint, b.moonTint, t), lerpColor(a.cloudTop, b.cloudTop, t),
        lerpColor(a.cloudMid, b.cloudMid, t), lerpColor(a.cloudBase, b.cloudBase, t)
    };
}

SkyPalette SkySystem::getBasePalette(float timeOfDay) const {
    // Keyframes over the day (0 = midnight, 0.5 = noon). The sun is up from
    // 0.20 to 0.75, so dawn and sunset colours peak right at those moments and
    // the long middle of the day stays a proper blue.
    struct Key { float t; const SkyPalette* pal; };
    const Key keys[] = {
        {0.00f, &nightPalette},
        {0.15f, &nightPalette},
        {0.215f, &dawnPalette},
        {0.30f, &dayPalette},
        {0.62f, &dayPalette},
        {0.735f, &sunsetPalette},
        {0.82f, &nightPalette},
        {1.00f, &nightPalette},
    };
    const int count = static_cast<int>(sizeof(keys) / sizeof(keys[0]));
    timeOfDay = std::clamp(timeOfDay, 0.f, 1.f);
    for (int i = 0; i < count - 1; ++i) {
        if (timeOfDay <= keys[i + 1].t) {
            const float span = keys[i + 1].t - keys[i].t;
            const float t = span > 0.f ? (timeOfDay - keys[i].t) / span : 0.f;
            return lerpPalette(*keys[i].pal, *keys[i + 1].pal, t);
        }
    }
    return nightPalette;
}

SkyPalette SkySystem::evaluatePalette(float timeOfDay) const {
    SkyPalette base = getBasePalette(timeOfDay);
    if (cloudDensity <= 0.65f) return base;

    // Heavy cloud greys the whole sky, but night stays night.
    const float night = nightAmount(timeOfDay);
    SkyPalette grey = overcastPalette;
    if (night > 0.f) {
        SkyPalette dark = nightPalette;
        dark.cloudTop = sf::Color(50, 56, 78);
        dark.cloudMid = sf::Color(34, 40, 60);
        dark.cloudBase = sf::Color(22, 26, 42);
        grey = lerpPalette(overcastPalette, dark, night);
    }
    float stormBlend = (cloudDensity - 0.65f) / 0.35f;
    return lerpPalette(base, grey, stormBlend);
}

sf::Color SkySystem::skyColorAt(const SkyPalette& pal, float f) const {
    // f: 0 at the top of the screen, 1 at the horizon line.
    if (f < 0.40f) return lerpColor(pal.zenith, pal.midSky, f / 0.40f);
    if (f < 0.78f) return lerpColor(pal.midSky, pal.lowerSky, (f - 0.40f) / 0.38f);
    return lerpColor(pal.lowerSky, pal.horizon, (f - 0.78f) / 0.22f);
}

float SkySystem::nightAmount(float timeOfDay) const {
    float n = 0.f;
    if (timeOfDay > 0.76f) n = (timeOfDay - 0.76f) / 0.07f;
    else if (timeOfDay < 0.20f) n = (0.20f - timeOfDay) / 0.06f;
    return std::clamp(n, 0.f, 1.f);
}

SkySystem::Body SkySystem::sunAt(float timeOfDay) const {
    Body b;
    const float t = (timeOfDay - 0.20f) / 0.55f;
    if (t < -0.08f || t > 1.08f) return b;
    const float rad = std::clamp(t, 0.0f, 1.0f) * PI;
    b.visible = true;
    b.x = 140.f + (skyWidth - 280.f) * t;
    b.y = (skyHeight * 0.86f) - std::sin(rad) * (skyHeight * 0.72f);
    b.height = std::sin(rad);
    return b;
}

SkySystem::Body SkySystem::moonAt(float timeOfDay) const {
    Body b;
    const float t = (timeOfDay >= 0.72f) ? ((timeOfDay - 0.72f) / 0.52f) : ((timeOfDay + 0.28f) / 0.52f);
    if (t < -0.05f || t > 1.05f) return b;
    const float rad = std::clamp(t, 0.0f, 1.0f) * PI;
    b.visible = true;
    b.x = 140.f + (skyWidth - 280.f) * t;
    b.y = (skyHeight * 0.86f) - std::sin(rad) * (skyHeight * 0.72f);
    b.height = std::sin(rad);
    return b;
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void SkySystem::update(float dt, float timeOfDay, float cameraX) {
    (void)timeOfDay; (void)cameraX;
    totalTime += dt;
    updateWeather(dt);

    const float baseWind = 9.5f;
    for (auto& c : clouds) {
        c.x += baseWind * c.speed * dt;
        if (c.x > cloudSpan * 8.f) c.x = std::fmod(c.x, cloudSpan);
    }
}

void SkySystem::drawSky(sf::RenderTarget& target, float timeOfDay, float cameraX) {
    (void)cameraX;
    const SkyPalette pal = evaluatePalette(timeOfDay);
    const sf::Vector2f viewSize = target.getView().getSize();

    const unsigned gw = static_cast<unsigned>(std::ceil(viewSize.x / static_cast<float>(P)));
    const unsigned gh = static_cast<unsigned>(std::ceil(viewSize.y / static_cast<float>(P)));
    if (gw == 0 || gh == 0) return;
    if (gradientSize.x != gw || gradientSize.y != gh) {
        gradientSize = sf::Vector2u(gw, gh);
        gradientPixels.assign(static_cast<size_t>(gw) * gh * 4, 255);
        gradientTexture.create(gw, gh);
        gradientTexture.setSmooth(false);
    }

    // Colour bands from zenith to horizon.
    sf::Color steps[GRADIENT_STEPS];
    for (int i = 0; i < GRADIENT_STEPS; ++i)
        steps[i] = skyColorAt(pal, static_cast<float>(i) / static_cast<float>(GRADIENT_STEPS - 1));

    // Light sources that tint the sky around them.
    struct Glow { float x, y, radius, strength; sf::Color color; };
    Glow glows[2];
    int glowCount = 0;
    const float clear = 1.0f - cloudDensity * 0.6f;

    const Body sun = sunAt(timeOfDay);
    if (sun.visible && pal.sunGlow.a > 4) {
        const float low = 1.0f - sun.height;                       // 1 at the horizon
        glows[glowCount++] = { sun.x / P, sun.y / P, (150.f + 190.f * low) / P,
                               std::min(1.f, pal.sunGlow.a / 255.f * 1.7f) * clear,
                               sf::Color(pal.sunGlow.r, pal.sunGlow.g, pal.sunGlow.b) };
    }
    const Body moon = moonAt(timeOfDay);
    const float night = nightAmount(timeOfDay);
    if (moon.visible && night > 0.05f) {
        glows[glowCount++] = { moon.x / P, moon.y / P, 120.f / P, 0.42f * night * clear,
                               sf::Color(120, 150, 220) };
    }

    const float horizonRow = std::max(1.f, static_cast<float>(gh) * HORIZON_FRACTION);
    sf::Uint8* px = gradientPixels.data();

    for (unsigned y = 0; y < gh; ++y) {
        const float v = std::min(1.f, static_cast<float>(y) / horizonRow) * static_cast<float>(GRADIENT_STEPS - 1);
        const int band = static_cast<int>(v);
        // Flat for the first half of each band, dithered into the next for the second half.
        const float blend = std::max(0.f, (v - static_cast<float>(band)) * 2.f - 1.f);

        for (unsigned x = 0; x < gw; ++x) {
            const float d = bayer(static_cast<int>(x), static_cast<int>(y));
            int idx = band + (blend > d ? 1 : 0);
            if (idx > GRADIENT_STEPS - 1) idx = GRADIENT_STEPS - 1;
            sf::Color c = steps[idx];

            for (int g = 0; g < glowCount; ++g) {
                const float dx = static_cast<float>(x) - glows[g].x;
                const float dy = (static_cast<float>(y) - glows[g].y) * 1.5f;   // wider than tall
                const float r = glows[g].radius;
                if (dx > r || dx < -r || dy > r || dy < -r) continue;
                const float dist = std::sqrt(dx * dx + dy * dy);
                if (dist >= r) continue;
                const float fall = 1.f - dist / r;
                const int level = static_cast<int>(fall * std::sqrt(fall) * glows[g].strength * GLOW_STEPS + d);
                if (level > 0)
                    c = lerpColor(c, glows[g].color, std::min(0.80f, static_cast<float>(level) / GLOW_STEPS));
            }

            px[0] = c.r; px[1] = c.g; px[2] = c.b; px[3] = 255;
            px += 4;
        }
    }

    gradientTexture.update(gradientPixels.data());
    sf::Sprite sprite(gradientTexture);
    sprite.setScale(static_cast<float>(P), static_cast<float>(P));
    target.draw(sprite);
}

void SkySystem::drawStars(sf::RenderTarget& target, float timeOfDay) {
    const float nightFactor = nightAmount(timeOfDay) * (1.0f - cloudDensity * 0.85f);
    if (nightFactor <= 0.02f) return;

    sf::VertexArray va(sf::Quads);
    const float p = static_cast<float>(P);
    auto cell = [&](int cx, int cy, sf::Color c) {
        const float x = static_cast<float>(cx) * p, y = static_cast<float>(cy) * p;
        va.append(sf::Vertex(sf::Vector2f(x, y), c));
        va.append(sf::Vertex(sf::Vector2f(x + p, y), c));
        va.append(sf::Vertex(sf::Vector2f(x + p, y + p), c));
        va.append(sf::Vertex(sf::Vector2f(x, y + p), c));
    };

    for (const auto& s : stars) {
        // Twinkle in three hard steps rather than a smooth fade.
        const float wave = (std::sin(totalTime * s.twinkleSpeed + s.phase) + 1.f) * 0.5f;
        const float step = wave > 0.72f ? 1.0f : (wave > 0.30f ? 0.72f : 0.45f);
        const float a = s.brightness * step * nightFactor;
        if (a < 0.08f) continue;

        sf::Color c = s.tint;
        c.a = static_cast<sf::Uint8>(std::min(255.f, a * 255.f));
        cell(s.cell.x, s.cell.y, c);
        if (s.big && step > 0.5f) {
            sf::Color arm = c;
            arm.a = static_cast<sf::Uint8>(c.a * 0.45f);
            cell(s.cell.x - 1, s.cell.y, arm);
            cell(s.cell.x + 1, s.cell.y, arm);
            cell(s.cell.x, s.cell.y - 1, arm);
            cell(s.cell.x, s.cell.y + 1, arm);
        }
    }
    target.draw(va);
}

void SkySystem::drawCelestials(sf::RenderTarget& target, float timeOfDay, float cameraX) {
    (void)cameraX;
    const SkyPalette pal = evaluatePalette(timeOfDay);
    const float p = static_cast<float>(P);
    const float viewH = target.getView().getSize().y;

    auto place = [&](const sf::IntRect& rect, float cx, float cy, sf::Color tint) {
        sf::Sprite sp(bodyAtlas, rect);
        sp.setScale(p, p);
        sp.setPosition(snap(cx - rect.width * p * 0.5f), snap(cy - rect.height * p * 0.5f));
        sp.setColor(tint);
        target.draw(sp);
    };

    const Body sun = sunAt(timeOfDay);
    if (sun.visible) {
        place(sunDiscRect, sun.x, sun.y, pal.sunTint);
        place(sunCoreRect, sun.x, sun.y, lerpColor(pal.sunTint, sf::Color(255, 255, 246), 0.7f));
    }

    const Body moon = moonAt(timeOfDay);
    if (moon.visible) {
        // Unlit side: the sky colour behind it, lifted slightly, so it hides the stars.
        const float f = std::min(1.f, moon.y / (viewH * HORIZON_FRACTION));
        const sf::Color dark = lerpColor(skyColorAt(pal, f), pal.moonTint, 0.10f);
        const float fade = std::max(nightAmount(timeOfDay), 0.35f);
        sf::Color lit = pal.moonTint;
        lit.a = static_cast<sf::Uint8>(255.f * fade);
        if (fade > 0.9f) place(moonDarkRect, moon.x, moon.y, dark);
        place(moonLitRect, moon.x, moon.y, lit);
    }
}

void SkySystem::drawClouds(sf::RenderTarget& target, float timeOfDay, float cameraX) {
    if (cloudDensity <= 0.02f || clouds.empty()) return;

    const SkyPalette pal = evaluatePalette(timeOfDay);
    const float p = static_cast<float>(P);

    // Far clouds sit in the haze: pull their colours toward the sky behind them.
    const sf::Color haze = pal.lowerSky;
    const sf::Color farBase = lerpColor(pal.cloudBase, haze, 0.55f);
    const sf::Color farMid  = lerpColor(pal.cloudMid,  haze, 0.50f);
    const sf::Color farTop  = lerpColor(pal.cloudTop,  haze, 0.40f);

    auto drawLayer = [&](bool far) {
        for (const auto& c : clouds) {
            if (c.far != far) continue;
            // Each cloud has its own density threshold and fades in over a short range,
            // in four steps so the edge pixels never go mushy.
            float vis = (cloudDensity - c.threshold) / 0.10f;
            if (vis <= 0.f) continue;
            vis = std::ceil(std::min(vis, 1.f) * 4.f) / 4.f;
            const sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * vis);

            const CloudShape& shape = (far ? farShapes : nearShapes)[static_cast<size_t>(c.shape)];
            float x = std::fmod(c.x - cameraX * c.parallax, cloudSpan);
            if (x < 0.f) x += cloudSpan;
            x -= cloudMargin;

            const sf::Vector2f pos(snap(x), snap(c.y));
            auto layer = [&](const sf::IntRect& rect, sf::Color col) {
                col.a = alpha;
                sf::Sprite sp(cloudAtlas, rect);
                sp.setScale(p, p);
                sp.setPosition(pos);
                sp.setColor(col);
                target.draw(sp);
            };
            layer(shape.base, far ? farBase : pal.cloudBase);
            layer(shape.mid,  far ? farMid  : pal.cloudMid);
            layer(shape.top,  far ? farTop  : pal.cloudTop);
        }
    };

    drawLayer(true);
    drawLayer(false);
}