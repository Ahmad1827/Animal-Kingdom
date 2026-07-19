#include "world/TerrainGenerator.h"
#include "core/VisualConfig.h"
#include <algorithm>
#include <cmath>

float TerrainGenerator::getTerrainHeight(float x, uint32_t worldSeed) {
    float seedOffset = static_cast<float>(worldSeed % 10000);
    float base = 500.f;
    float macro = std::sin((x + seedOffset) * 0.0005f) * 300.f;
    float detail = std::sin((x - seedOffset) * 0.002f) * 80.f;
    float micro = std::sin((x + seedOffset * 2.f) * 0.01f) * 15.f;
    return base + macro + detail + micro;
}

sf::VertexArray TerrainGenerator::generateSurfaceMesh(const sf::FloatRect& bounds, float resolution, uint32_t seed) {
    sf::VertexArray mesh(sf::Quads);
    int points = static_cast<int>(bounds.width / resolution);
    float texScale = VisualConfig::TERRAIN_TEXTURE_SCALE;
    sf::IntRect sTile = VisualConfig::TILE_SURFACE;

    for (int i = 0; i < points; ++i) {
        float x1 = bounds.left + (i * resolution);
        float x2 = x1 + resolution;
        float y1 = getTerrainHeight(x1, seed);
        float y2 = getTerrainHeight(x2, seed);

        float uvX1 = std::fmod(std::abs(x1), texScale);
        float uvX2 = uvX1 + (x2 - x1);

        mesh.append(sf::Vertex(sf::Vector2f(x1, y1), sf::Color::White, sf::Vector2f(sTile.left + uvX1, sTile.top)));
        mesh.append(sf::Vertex(sf::Vector2f(x2, y2), sf::Color::White, sf::Vector2f(sTile.left + uvX2, sTile.top)));
        mesh.append(sf::Vertex(sf::Vector2f(x2, y2 + texScale), sf::Color::White, sf::Vector2f(sTile.left + uvX2, sTile.top + sTile.height)));
        mesh.append(sf::Vertex(sf::Vector2f(x1, y1 + texScale), sf::Color::White, sf::Vector2f(sTile.left + uvX1, sTile.top + sTile.height)));
    }
    return mesh;
}

sf::VertexArray TerrainGenerator::generateUndergroundMesh(const sf::FloatRect& bounds, float resolution, uint32_t seed, sf::Color undergroundColor) {
    sf::VertexArray mesh(sf::Quads);
    int points = static_cast<int>(bounds.width / resolution);
    float texScale = VisualConfig::TERRAIN_TEXTURE_SCALE;
    float bottom = bounds.top + bounds.height;

    for (int i = 0; i < points; ++i) {
        float x1 = bounds.left + (i * resolution);
        float x2 = x1 + resolution;
        float y1 = getTerrainHeight(x1, seed);
        float y2 = getTerrainHeight(x2, seed);

        // Follow the SAME slope as the surface mesh's bottom edge - no flattening
        float top1 = y1 + texScale;
        float top2 = y2 + texScale;

        if (top1 >= bottom && top2 >= bottom) continue;

        mesh.append(sf::Vertex(sf::Vector2f(x1, top1), undergroundColor));
        mesh.append(sf::Vertex(sf::Vector2f(x2, top2), undergroundColor));
        mesh.append(sf::Vertex(sf::Vector2f(x2, bottom), undergroundColor));
        mesh.append(sf::Vertex(sf::Vector2f(x1, bottom), undergroundColor));
    }
    return mesh;
}