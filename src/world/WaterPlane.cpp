#include "world/WaterPlane.h"
#include <cmath>
#include <algorithm>

namespace {

// Every pixel value in this class (strip height, wobble, glint size, reflection
// depth) was tuned for a render target 720 pixels high. The scene is now drawn
// at the window's real resolution, so those values are multiplied by this factor
// to keep the water looking the same at any size.
float pixelScale(const sf::RenderTarget& target) {
    return std::max(0.5f, static_cast<float>(target.getSize().y) / 720.f);
}

} // namespace

WaterPlane::WaterPlane()
    : time(0.f),
      surfaceWorldY(536.0f),
      reflectionDepthPx(480.0f),
      reflectionAlpha(0.82f),
      stripHeightPx(2.0f),
      waveAmplitude(3.8f),
      waveSpeed(2.4f),
      waveFrequency(0.055f),
      shallowColor(44, 92, 118),
      deepColor(20, 42, 60) {}

void WaterPlane::update(float dt) {
    time += dt;
    if (time > 10000.f) time -= 10000.f;
}

void WaterPlane::setSurfaceWorldY(float y) { surfaceWorldY = y; }
float WaterPlane::getSurfaceWorldY() const { return surfaceWorldY; }
void WaterPlane::setReflectionDepth(float px) { reflectionDepthPx = std::max(1.f, px); }
void WaterPlane::setReflectionAlpha(float a) { reflectionAlpha = std::clamp(a, 0.f, 1.f); }
void WaterPlane::setWaveAmplitude(float px) { waveAmplitude = px; }
void WaterPlane::setWaveSpeed(float s) { waveSpeed = s; }
void WaterPlane::setDeepColor(const sf::Color& c) { deepColor = c; }
void WaterPlane::setShallowColor(const sf::Color& c) { shallowColor = c; }

float WaterPlane::worldToScreenY(float worldY, const sf::View& view, float targetHeight) {
    sf::Vector2f vs = view.getSize();
    sf::Vector2f vc = view.getCenter();
    if (vs.y <= 0.f) return 0.f;
    float viewTop = vc.y - vs.y * 0.5f;
    return std::floor((worldY - viewTop) / vs.y * targetHeight);
}

void WaterPlane::draw(sf::RenderTarget& target,
                      const sf::Texture& sceneTexture,
                      const sf::View& cameraView,
                      sf::Color skyTint) const {
    float w = static_cast<float>(target.getSize().x);
    float h = static_cast<float>(target.getSize().y);
    if (w <= 0.f || h <= 0.f) return;

    float surfaceY = worldToScreenY(surfaceWorldY, cameraView, h);
    if (surfaceY >= h) return;

    bool submerged = (surfaceY < 0.f);
    if (submerged) surfaceY = 0.f;

    sf::View previousView = target.getView();
    target.setView(target.getDefaultView());

    drawBody(target, surfaceY, w, h, skyTint);

    if (!submerged) {
        drawReflection(target, sceneTexture, surfaceY, w, h);
        //drawSurfaceLine(target, surfaceY, w, skyTint);
        drawSparkles(target, surfaceY, w, h, cameraView, skyTint);
    }

    target.setView(previousView);
}

void WaterPlane::drawBody(sf::RenderTarget& target, float surfaceY, float w, float h, sf::Color skyTint) const {
    auto tinted = [&](sf::Color base, float alpha) {
        return sf::Color(
            static_cast<sf::Uint8>(base.r * (skyTint.r / 255.f)),
            static_cast<sf::Uint8>(base.g * (skyTint.g / 255.f)),
            static_cast<sf::Uint8>(base.b * (skyTint.b / 255.f)),
            static_cast<sf::Uint8>(std::clamp(alpha, 0.f, 255.f))
        );
    };

    sf::Color top = tinted(shallowColor, 220.f);
    sf::Color bottom = tinted(deepColor, 250.f);

    sf::VertexArray body(sf::Quads, 4);
    body[0] = sf::Vertex(sf::Vector2f(0.f, surfaceY), top);
    body[1] = sf::Vertex(sf::Vector2f(w, surfaceY), top);
    body[2] = sf::Vertex(sf::Vector2f(w, h), bottom);
    body[3] = sf::Vertex(sf::Vector2f(0.f, h), bottom);
    target.draw(body);
}

void WaterPlane::drawReflection(sf::RenderTarget& target, const sf::Texture& sceneTex,
                                float surfaceY, float w, float h) const {
    const float s = pixelScale(target);
    sf::VertexArray reflection(sf::Quads);
    const float strip = std::max(1.0f, std::round(stripHeightPx * s));
    const float depthPx = reflectionDepthPx * s;

    for (float k = 0.f; ; k += strip) {
        float dstTop = surfaceY + k;
        if (dstTop >= h) break;

        float depth01 = k / depthPx;
        if (depth01 >= 1.f) break;

        float srcBottom = surfaceY - k;
        float srcTop = surfaceY - k - strip;
        if (srcTop < 0.f) break;

        float rawWobble = std::sin((k / s) * waveFrequency + time * waveSpeed) * waveAmplitude * s * (0.5f + depth01 * 1.3f);
        float wobble = std::floor(rawWobble);

        float fade = (1.0f - depth01 * 0.75f);
        float a = reflectionAlpha * fade * 255.f;

        float mixToWater = depth01 * 0.45f;
        sf::Color c(
            static_cast<sf::Uint8>(255 * (1.0f - mixToWater) + shallowColor.r * mixToWater),
            static_cast<sf::Uint8>(255 * (1.0f - mixToWater) + shallowColor.g * mixToWater),
            static_cast<sf::Uint8>(255 * (1.0f - mixToWater) + shallowColor.b * mixToWater),
            static_cast<sf::Uint8>(std::clamp(a, 0.f, 255.f))
        );

        float x0 = wobble;
        float x1 = w + wobble;
        float y0 = dstTop;
        float y1 = std::min(dstTop + strip, h);

        reflection.append(sf::Vertex(sf::Vector2f(x0, y0), c, sf::Vector2f(0.f, srcBottom)));
        reflection.append(sf::Vertex(sf::Vector2f(x1, y0), c, sf::Vector2f(w, srcBottom)));
        reflection.append(sf::Vertex(sf::Vector2f(x1, y1), c, sf::Vector2f(w, srcTop)));
        reflection.append(sf::Vertex(sf::Vector2f(x0, y1), c, sf::Vector2f(0.f, srcTop)));
    }

    if (reflection.getVertexCount() == 0) return;

    sf::RenderStates states;
    states.texture = &sceneTex;
    target.draw(reflection, states);
}

void WaterPlane::drawSurfaceLine(sf::RenderTarget& target, float surfaceY, float w, sf::Color skyTint) const {
    const float u = std::max(1.f, std::round(pixelScale(target)));   // one 720p pixel
    sf::VertexArray lip(sf::Quads, 8);

    sf::Color bright(
        static_cast<sf::Uint8>(230 * (skyTint.r / 255.f)),
        static_cast<sf::Uint8>(248 * (skyTint.g / 255.f)),
        static_cast<sf::Uint8>(255 * (skyTint.b / 255.f)),
        220);
    sf::Color shadow(14, 32, 44, 160);

    lip[0] = sf::Vertex(sf::Vector2f(0.f, surfaceY - u), bright);
    lip[1] = sf::Vertex(sf::Vector2f(w, surfaceY - u), bright);
    lip[2] = sf::Vertex(sf::Vector2f(w, surfaceY + u), bright);
    lip[3] = sf::Vertex(sf::Vector2f(0.f, surfaceY + u), bright);

    lip[4] = sf::Vertex(sf::Vector2f(0.f, surfaceY + u), shadow);
    lip[5] = sf::Vertex(sf::Vector2f(w, surfaceY + u), shadow);
    lip[6] = sf::Vertex(sf::Vector2f(w, surfaceY + 3.f * u), shadow);
    lip[7] = sf::Vertex(sf::Vector2f(0.f, surfaceY + 3.f * u), shadow);

    target.draw(lip);
}

void WaterPlane::drawSparkles(sf::RenderTarget& target, float surfaceY, float w, float h,
                              const sf::View& cameraView, sf::Color skyTint) const {
    const float s = pixelScale(target);
    const float thick = std::max(1.f, std::round(2.f * s));
    sf::VertexArray glints(sf::Quads);
    const int count = 48;
    const float span = w + 200.f * s;
    float camX = cameraView.getCenter().x;

    for (int i = 0; i < count; ++i) {
        float fi = static_cast<float>(i);
        float baseX = std::fmod((fi * 191.13f - camX * 0.95f) * s, span);
        if (baseX < 0.f) baseX += span;
        baseX -= 100.f * s;

        float rowT = std::fmod(fi * 0.3411f, 1.0f);
        float y = std::floor(surfaceY + 3.f * s + rowT * rowT * std::min(reflectionDepthPx * s, h - surfaceY));
        if (y >= h - thick) continue;

        float phase = time * 2.0f + fi * 2.41f;
        float twinkle = std::max(0.0f, std::sin(phase));
        float len = std::floor((5.f + 14.f * std::fmod(fi * 0.57f, 1.0f)) * s);

        float alpha = 160.f * twinkle * (1.f - rowT * 0.65f);
        if (alpha < 8.f) continue;

        float drift = std::floor(std::sin(time * 1.1f + fi) * 3.0f * s);
        float x0 = std::floor(baseX + drift);
        float x1 = x0 + len;

        sf::Color c(
            static_cast<sf::Uint8>(235 * (skyTint.r / 255.f)),
            static_cast<sf::Uint8>(248 * (skyTint.g / 255.f)),
            static_cast<sf::Uint8>(255 * (skyTint.b / 255.f)),
            static_cast<sf::Uint8>(std::clamp(alpha, 0.f, 255.f)));

        glints.append(sf::Vertex(sf::Vector2f(x0, y), c));
        glints.append(sf::Vertex(sf::Vector2f(x1, y), c));
        glints.append(sf::Vertex(sf::Vector2f(x1, y + thick), c));
        glints.append(sf::Vertex(sf::Vector2f(x0, y + thick), c));
    }

    if (glints.getVertexCount() > 0) target.draw(glints);
}