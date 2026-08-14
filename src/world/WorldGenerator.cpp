#include "world/WorldGenerator.h"
#include <random>
#include <cmath>

static constexpr float FLAT_GROUND_Y = 500.0f;

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t seed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Tree> result;
    float currentX = startX + 50.f;
    float endX = startX + width - 50.f;

    std::mt19937 rng(seed);

    while (currentX < endX) {
        float spacing = 90.f + (rng() % 140); 
        currentX += spacing;
        if (currentX >= endX) break;

        float scale = 0.75f + (rng() % 65) / 100.f;
        float treeWidth = 80.f * scale;
        float treeHeight = 280.f * scale;
        sf::Color trunkColor(101, 67, 33);

        // Subtly bury the tree trunks exactly into the grass line
        float yOffset = (rng() % 8) - 2.f; 

        result.emplace_back(currentX, FLAT_GROUND_Y + yOffset, treeWidth, treeHeight, trunkColor, decorTex);
    }

    return result;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Decoration> decors;
    std::mt19937 rng(chunkSeed);

    int count = 15 + (rng() % 15);
    float step = width / static_cast<float>(count);

    for (int i = 0; i < count; ++i) {
        float x = startX + (i * step) + (rng() % static_cast<int>(step * 0.5f + 1.f));
        float y = FLAT_GROUND_Y; 

        int type = rng() % 6;
        float scale = 0.8f + (rng() % 50) / 100.f;

        decors.emplace_back(x, y, type, scale, decorTex);
    }

    return decors;
}