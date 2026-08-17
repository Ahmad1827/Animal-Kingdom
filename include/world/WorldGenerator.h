#pragma once
#include <vector>
#include <cstdint>
#include "entities/Tree.h"
#include "world/Decoration.h"
#include "world/Biome.h"

class WorldGenerator {
public:
    static bool isSettlementCore(float worldX, uint32_t worldSeed);
    static bool isMeetingGroundClearing(float worldX, uint32_t worldSeed);

    static std::vector<Tree> generateTrees(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome, sf::Texture& decorTex);
    static std::vector<Decoration> generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome, sf::Texture& decorTex);
};