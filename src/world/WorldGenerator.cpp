#include "world/WorldGenerator.h"
#include <random>
#include <cmath>

static constexpr float FLAT_GROUND_Y = 500.0f;

static bool isInsideSettlementClearing(float x) {
    if (std::abs(x - 1000.0f) < 480.0f) {
        return true;
    }
    for (int i = 1; i <= 6; ++i) {
        float rivalPos1 = (i * 3 * 2000.f) + 1000.f;
        float rivalPos2 = (-i * 3 * 2000.f) + 1000.f;
        if (std::abs(x - rivalPos1) < 480.0f || std::abs(x - rivalPos2) < 480.0f) {
            return true;
        }
    }
    return false;
}

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t seed, uint32_t worldSeed, const BiomeProperties& props, sf::Texture& decorTex) {
    std::vector<Tree> result;
    float currentX = startX + 50.f;
    float endX = startX + width - 50.f;

    std::mt19937 rng(seed);
    int treeCounter = 1;

    while (currentX < endX) {
        float spacing = 90.f + (rng() % 140);
        currentX += spacing;
        if (currentX >= endX) break;

        if (isInsideSettlementClearing(currentX)) {
            continue;
        }

        float scale = 0.75f + (rng() % 65) / 100.f;
        float treeWidth = 80.f * scale;
        float treeHeight = 280.f * scale;
        sf::Color trunkColor(101, 67, 33);

        float yOffset = (rng() % 8) - 2.f;
        int uniqueTreeId = static_cast<int>(std::abs(std::round(currentX))) * 100 + (treeCounter++);
        result.emplace_back(currentX, FLAT_GROUND_Y + yOffset, treeWidth, treeHeight, trunkColor, decorTex, uniqueTreeId);
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

        if (isInsideSettlementClearing(x)) {
            continue;
        }

        int type = rng() % 6;
        float scale = 0.8f + (rng() % 50) / 100.f;

        decors.emplace_back(x, y, type, scale, decorTex);
    }

    return decors;
}