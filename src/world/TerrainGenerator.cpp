#include "world/TerrainGenerator.h"
#include <cmath>

float TerrainGenerator::getTerrainHeight(float x) {
    // Composite sine waves for smooth, deterministic rolling hills
    float base = 500.f;
    float macro = std::sin(x * 0.001f) * 150.f;
    float detail = std::sin(x * 0.004f) * 40.f;
    float micro = std::sin(x * 0.01f) * 10.f;
    return base + macro + detail + micro;
}

sf::VertexArray TerrainGenerator::generateTerrainMesh(float startX, float width, float resolution) {
    int points = static_cast<int>(width / resolution) + 1;
    sf::VertexArray mesh(sf::TriangleStrip, points * 2);

    for (int i = 0; i < points; ++i) {
        float currentX = startX + (i * resolution);
        float currentY = getTerrainHeight(currentX);

        mesh[i * 2].position = sf::Vector2f(currentX, currentY);
        mesh[i * 2].color = sf::Color(34, 100, 34);

        mesh[i * 2 + 1].position = sf::Vector2f(currentX, currentY + 1000.f); // Deep underground
        mesh[i * 2 + 1].color = sf::Color(20, 50, 20);
    }
    return mesh;
}