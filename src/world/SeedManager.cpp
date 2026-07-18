#include "world/SeedManager.h"

uint32_t SeedManager::getChunkSeed(uint32_t worldSeed, int chunkX) {
    uint32_t hash = worldSeed;
    hash ^= (uint32_t)chunkX + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}

float SeedManager::getRandomFloat(uint32_t& seed, float min, float max) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    float normalized = static_cast<float>(seed) / static_cast<float>(0xFFFFFFFF);
    return min + normalized * (max - min);
}

int SeedManager::getRandomInt(uint32_t& seed, int min, int max) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return min + (seed % (max - min + 1));
}