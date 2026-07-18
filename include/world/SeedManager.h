#pragma once
#include <cstdint>

class SeedManager {
public:
    static uint32_t getChunkSeed(uint32_t worldSeed, int chunkX);
    static float getRandomFloat(uint32_t& seed, float min, float max);
    static int getRandomInt(uint32_t& seed, int min, int max);
};