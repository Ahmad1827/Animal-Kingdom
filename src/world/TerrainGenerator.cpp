#include "world/TerrainGenerator.h"
#include <cmath>

float TerrainGenerator::getTerrainHeight(float x, uint32_t worldSeed) {
    float seedOffset = static_cast<float>(worldSeed % 10000);
    float base = 500.f;
    float macro = std::sin((x + seedOffset) * 0.0005f) * 300.f;
    float detail = std::sin((x - seedOffset) * 0.002f) * 80.f;
    float micro = std::sin((x + seedOffset * 2.f) * 0.01f) * 15.f;
    return base + macro + detail + micro;
}

sf::VertexArray TerrainGenerator::generateTerrainMesh(const sf::FloatRect& chunkBounds, float resolution, uint32_t worldSeed, sf::Color topColor, sf::Color bottomColor) {
    sf::VertexArray mesh(sf::TriangleStrip);

    int points = static_cast<int>(chunkBounds.width / resolution) + 1;
    bool hasVisibleTerrain = false;

    for (int i = 0; i < points; ++i) {
        float currentX = chunkBounds.left + (i * resolution);
        float surfaceY = getTerrainHeight(currentX, worldSeed);

        // If the entire chunk is in the sky above the surface, draw nothing.
        if (chunkBounds.top + chunkBounds.height < surfaceY) continue;

        hasVisibleTerrain = true;
        
        // Clamp top to surface, or chunk top if underground
        float topY = std::max(surfaceY, chunkBounds.top);
        float bottomY = chunkBounds.top + chunkBounds.height;

        mesh.append(sf::Vertex(sf::Vector2f(currentX, topY), topColor));
        mesh.append(sf::Vertex(sf::Vector2f(currentX, bottomY), bottomColor));
    }

    if (!hasVisibleTerrain) mesh.clear();
    return mesh;
}