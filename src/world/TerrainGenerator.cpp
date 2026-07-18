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

sf::VertexArray TerrainGenerator::generateTerrainMesh(float startX, float width, float resolution, uint32_t worldSeed, sf::Color topColor, sf::Color bottomColor) {
    int points = static_cast<int>(width / resolution) + 1;
    sf::VertexArray mesh(sf::TriangleStrip, points * 2);

    for (int i = 0; i < points; ++i) {
        float currentX = startX + (i * resolution);
        float currentY = getTerrainHeight(currentX, worldSeed);

        mesh[i * 2].position = sf::Vector2f(currentX, currentY);
        mesh[i * 2].color = topColor;

        mesh[i * 2 + 1].position = sf::Vector2f(currentX, currentY + 1500.f);
        mesh[i * 2 + 1].color = bottomColor;
    }
    return mesh;
}