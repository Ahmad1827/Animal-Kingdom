#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "entities/Tree.h"
#include "world/Decoration.h"
#include "world/Biome.h"

enum class ClearanceType {
    Base,
    Border,
    MeetingGround
};

struct WorldClearanceZone {
    float minX;
    float maxX;
    ClearanceType type;
    std::string identifier;
};

namespace DecorType {
    constexpr int GenericBush       = 0;
    constexpr int SmallBush         = 1;
    constexpr int JungleUndergrowth = 2;
    constexpr int Rock              = 3;
    constexpr int SmallRock         = 4;
    constexpr int Flower            = 5;
    constexpr int LeafLitter        = 6;
    constexpr int GrassTuft         = 7;
}

class WorldGenerator {
public:
    static std::vector<WorldClearanceZone> getClearanceZones(uint32_t worldSeed);
    static bool isPositionClear(float worldX, uint32_t worldSeed);

    static std::vector<Tree> generateTrees(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome, sf::Texture& decorTex);
    static std::vector<Decoration> generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome, sf::Texture& decorTex);
};