#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "entities/Tree.h"
#include "world/Decoration.h"
#include "world/Biome.h"

enum class ClearanceType {
    Base,
    MeetingGround,
    Structure
};

struct WorldClearanceZone {
    float minX;
    float maxX;
    ClearanceType type;
    std::string identifier;
};

class WorldGenerator {
public:
    static std::vector<WorldClearanceZone> getClearanceZones(uint32_t worldSeed);
    static bool isPositionClear(float worldX, uint32_t worldSeed);

    static std::vector<Tree> generateTrees(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome, sf::Texture& decorTex);
    static std::vector<Decoration> generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome, sf::Texture& decorTex);
};