#include "world/WaterPlane.h"
#include <cmath>
#include <algorithm>

WaterPlane::WaterPlane()
    : time(0.f),
      surfaceWorldY(610.0f),      // FLAT_GROUND_Y (500) + a thin 110px bank
      reflectionDepthPx(420.0f),
      reflectionAlpha(0.62f),
      stripHeightPx(3.0f),
      waveAmplitude(3.2f),
      waveSpeed(1.6f),
      waveFrequency(0.045f),
      shallowColor(38, 62, 78),
      deepColor(10, 18, 28) {}

void WaterPlane::update(float dt) {
    time += dt;
    if (time > 10000.f) time -= 10000.f;   // keep sin() argument well-conditioned
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
    return (worldY - viewTop) / vs.y * targetHeight;
}

void WaterPlane::draw(sf::RenderTarget& target,
                      const sf::Texture& sceneTexture,
                      const sf::View& cameraView,
                      sf::Color skyTint) const {
    float w = static_cast<float>(target.getSize().x);
    float h = static_cast<float>(target.getSize().y);
    if (w <= 0.f || h <= 0.f) return;

    float surfaceY = worldToScreenY(surfaceWorldY, cameraView, h);

    // Water entirely below the viewport - nothing to do.
    if (surfaceY >= h) return;
    // Camera is under the waterline. Draw the body only; a mirror would be
    // sampling from off-screen anyway.
    bool submerged = (surfaceY < 0.f);
    if (submerged) surfaceY = 0.f;

    sf::View previousView = target.getView();
    target.setView(target.getDefaultView());

    if (!submerged) {
        drawReflection(target, sceneTexture, surfaceY, w, h, skyTint);
    }
    drawBody(target, surfaceY, w, h, skyTint);
    if (!submerged) {
        drawSurfaceLine(target, surfaceY, w, skyTint);
        drawSparkles(target, surfaceY, w, h, cameraView, skyTint);
    }

    target.setView(previousView);
}

void WaterPlane::drawReflection(sf::RenderTarget& target, const sf::Texture& sceneTex,
                                float surfaceY, float w, float h, sf::Color skyTint) const {
    // Build the whole mirror as one textured VertexArray so it costs a single
    // draw call regardless of how many strips it takes.
    //
    // Row mapping: a destination pixel at (surfaceY + d) samples the source
    // pixel at (surfaceY - d). Each strip therefore has its texture coords
    // flipped vertically, which also handles the flip *within* the strip.
    sf::VertexArray reflection(sf::Quads);
    reflection.resize(0);

    const float strip = std::max(1.0f, stripHeightPx);

    for (float k = 0.f; ; k += strip) {
        float dstTop = surfaceY + k;
        if (dstTop >= h) break;

        float depth01 = k / reflectionDepthPx;
        if (depth01 >= 1.f) break;

        float srcBottom = surfaceY - k;              // maps to dstTop
        float srcTop    = surfaceY - k - strip;      // maps to dstTop + strip
        if (srcTop < 0.f) break;                     // ran out of scene above the water

        // Horizontal ripple. Amplitude grows with depth so the near-surface
        // reflection stays legible and the far end dissolves.
        float wobble = std::sin(k * waveFrequency + time * waveSpeed) * waveAmplitude * (0.35f + depth01 * 1.15f);

        // Fade out with depth, and lean the colour toward the deep water tone.
        float fade = (1.f - depth01);
        fade *= fade;                                 // quadratic reads better than linear
        float a = reflectionAlpha * fade * 255.f;

        float mixToDeep = depth01 * 0.75f;
        sf::Color c(
            static_cast<sf::Uint8>(std::clamp((255.f * (1.f - mixToDeep) + deepColor.r * mixToDeep) * (skyTint.r / 255.f), 0.f, 255.f)),
            static_cast<sf::Uint8>(std::clamp((255.f * (1.f - mixToDeep) + deepColor.g * mixToDeep) * (skyTint.g / 255.f), 0.f, 255.f)),
            static_cast<sf::Uint8>(std::clamp((255.f * (1.f - mixToDeep) + deepColor.b * mixToDeep) * (skyTint.b / 255.f), 0.f, 255.f)),
            static_cast<sf::Uint8>(std::clamp(a, 0.f, 255.f))
        );

        float x0 = wobble;
        float x1 = w + wobble;
        float y0 = dstTop;
        float y1 = std::min(dstTop + strip, h);

        // TL, TR, BR, BL - texture coords inverted top-to-bottom.
        reflection.append(sf::Vertex(sf::Vector2f(x0, y0), c, sf::Vector2f(0.f, srcBottom)));
        reflection.append(sf::Vertex(sf::Vector2f(x1, y0), c, sf::Vector2f(w,   srcBottom)));
        reflection.append(sf::Vertex(sf::Vector2f(x1, y1), c, sf::Vector2f(w,   srcTop)));
        reflection.append(sf::Vertex(sf::Vector2f(x0, y1), c, sf::Vector2f(0.f, srcTop)));
    }

    if (reflection.getVertexCount() == 0) return;

    sf::RenderStates states;
    states.texture = &sceneTex;
    target.draw(reflection, states);
}

void WaterPlane::drawBody(sf::RenderTarget& target, float surfaceY, float w, float h, sf::Color skyTint) const {
    // Tinted gradient laid over the mirror. This is what makes it read as
    // water rather than as an upside-down copy of the world.
    auto tinted = [&](sf::Color base, float alpha) {
        return sf::Color(
            static_cast<sf::Uint8>(base.r * (skyTint.r / 255.f)),
            static_cast<sf::Uint8>(base.g * (skyTint.g / 255.f)),
            static_cast<sf::Uint8>(base.b * (skyTint.b / 255.f)),
            static_cast<sf::Uint8>(std::clamp(alpha, 0.f, 255.f))
        );
    };

    sf::Color top = tinted(shallowColor, 110.f);
    sf::Color bottom = tinted(deepColor, 232.f);

    sf::VertexArray body(sf::Quads, 4);
    body[0] = sf::Vertex(sf::Vector2f(0.f, surfaceY), top);
    body[1] = sf::Vertex(sf::Vector2f(w,   surfaceY), top);
    body[2] = sf::Vertex(sf::Vector2f(w,   h),        bottom);
    body[3] = sf::Vertex(sf::Vector2f(0.f, h),        bottom);
    target.draw(body);
}

void WaterPlane::drawSurfaceLine(sf::RenderTarget& target, float surfaceY, float w, sf::Color skyTint) const {
    // Two-pixel bright lip along the waterline, plus a darker shadow just
    // under it. Cheap, and it's most of what makes the edge read as a surface.
    sf::VertexArray lip(sf::Quads, 8);

    sf::Color bright(
        static_cast<sf::Uint8>(190 * (skyTint.r / 255.f)),
        static_cast<sf::Uint8>(215 * (skyTint.g / 255.f)),
        static_cast<sf::Uint8>(230 * (skyTint.b / 255.f)),
        130);
    sf::Color shadow(6, 12, 20, 150);

    lip[0] = sf::Vertex(sf::Vector2f(0.f, surfaceY - 1.f), bright);
    lip[1] = sf::Vertex(sf::Vector2f(w,   surfaceY - 1.f), bright);
    lip[2] = sf::Vertex(sf::Vector2f(w,   surfaceY + 1.f), bright);
    lip[3] = sf::Vertex(sf::Vector2f(0.f, surfaceY + 1.f), bright);

    lip[4] = sf::Vertex(sf::Vector2f(0.f, surfaceY + 1.f), shadow);
    lip[5] = sf::Vertex(sf::Vector2f(w,   surfaceY + 1.f), shadow);
    lip[6] = sf::Vertex(sf::Vector2f(w,   surfaceY + 5.f), shadow);
    lip[7] = sf::Vertex(sf::Vector2f(0.f, surfaceY + 5.f), shadow);

    target.draw(lip);
}

void WaterPlane::drawSparkles(sf::RenderTarget& target, float surfaceY, float w, float h,
                              const sf::View& cameraView, sf::Color skyTint) const {
    // Short horizontal glints, like the white flecks on Kingdom's water.
    // Anchored to world space (via the camera centre) so they scroll with the
    // world instead of sliding around on the camera.
    sf::VertexArray glints(sf::Quads);

    const int count = 34;
    float camX = cameraView.getCenter().x;

    for (int i = 0; i < count; ++i) {
        // Deterministic pseudo-random layout, stable frame to frame.
        float fi = static_cast<float>(i);
        float baseX = std::fmod(fi * 197.31f - camX * 0.85f, w + 200.f);
        if (baseX < 0.f) baseX += (w + 200.f);
        baseX -= 100.f;

        float rowT = std::fmod(fi * 0.3719f, 1.0f);
        float y = surfaceY + 8.f + rowT * rowT * std::min(reflectionDepthPx, h - surfaceY);
        if (y >= h - 1.f) continue;

        float phase = time * 1.1f + fi * 2.17f;
        float twinkle = 0.5f + 0.5f * std::sin(phase);
        float len = 5.f + 11.f * std::fmod(fi * 0.611f, 1.0f);

        float alpha = 60.f * twinkle * (1.f - rowT * 0.8f);
        if (alpha < 4.f) continue;

        float drift = std::sin(time * 0.7f + fi) * 2.5f;
        float x0 = baseX + drift;
        float x1 = x0 + len;

        sf::Color c(
            static_cast<sf::Uint8>(210 * (skyTint.r / 255.f)),
            static_cast<sf::Uint8>(228 * (skyTint.g / 255.f)),
            static_cast<sf::Uint8>(240 * (skyTint.b / 255.f)),
            static_cast<sf::Uint8>(std::clamp(alpha, 0.f, 255.f)));

        glints.append(sf::Vertex(sf::Vector2f(x0, y),        c));
        glints.append(sf::Vertex(sf::Vector2f(x1, y),        c));
        glints.append(sf::Vertex(sf::Vector2f(x1, y + 1.5f), c));
        glints.append(sf::Vertex(sf::Vector2f(x0, y + 1.5f), c));
    }

    if (glints.getVertexCount() > 0) target.draw(glints);
}