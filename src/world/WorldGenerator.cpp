#include "world/WorldGenerator.h"
#include "world/TerrainGenerator.h"
#include "world/SeedManager.h"
#include <cmath>

sf::Color interpolateColor(sf::Color c1, sf::Color c2, float t) {
    return sf::Color(
        c1.r + static_cast<sf::Uint8>((c2.r - c1.r) * t),
        c1.g + static_cast<sf::Uint8>((c2.g - c1.g) * t),
        c1.b + static_cast<sf::Uint8>((c2.b - c1.b) * t)
    );
}

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome) {
    std::vector<Tree> trees;
    int numTrees = SeedManager::getRandomInt(chunkSeed, biome.minTrees, biome.maxTrees);
    if (numTrees == 0) return trees;

    std::vector<float> existingX;

    for (int i = 0; i < numTrees; ++i) {
        float x = 0;
        bool validPos = false;
        
        for (int attempts = 0; attempts < 10; ++attempts) {
            x = startX + SeedManager::getRandomFloat(chunkSeed, 100.f, width - 100.f);
            validPos = true;
            for (float ex : existingX) {
                if (std::abs(ex - x) < (150.f * biome.treeSizeMultiplier)) {
                    validPos = false;
                    break;
                }
            }
            if (validPos) break;
        }
        
        if (!validPos) continue; 
        existingX.push_back(x);

        float y = TerrainGenerator::getTerrainHeight(x, worldSeed);
        
        float heightBase = SeedManager::getRandomFloat(chunkSeed, 200.f, 600.f) * biome.treeSizeMultiplier;
        float trunkWidth = SeedManager::getRandomFloat(chunkSeed, 15.f, 40.f) * biome.treeSizeMultiplier;

        float tTrunk = SeedManager::getRandomFloat(chunkSeed, 0.f, 1.f);
        sf::Color trunkColor = interpolateColor(biome.trunkColorMin, biome.trunkColorMax, tTrunk);
        
        float tLeaf = SeedManager::getRandomFloat(chunkSeed, 0.f, 1.f);
        sf::Color leafColor = interpolateColor(biome.leafColorMin, biome.leafColorMax, tLeaf);

        Tree tree(x, y, trunkWidth, heightBase, trunkColor);
        
        int baseBranches = 2;
        if (heightBase > 400.f) baseBranches = 4;
        if (heightBase > 800.f) baseBranches = 6;
        
        int numBranches = std::max(1, baseBranches + biome.branchCountModifier);
        
        // Strict branch pacing to ensure reachable platforming routes
        float lastBranchY = 100.f; // Starting branch height
        for (int j = 0; j < numBranches; ++j) {
            float yOff = lastBranchY + SeedManager::getRandomFloat(chunkSeed, 120.f, 180.f);
            
            if (yOff > heightBase - 50.f) break; // Don't grow above trunk
            lastBranchY = yOff;

            float bWidth = SeedManager::getRandomFloat(chunkSeed, 120.f, 280.f) * biome.treeSizeMultiplier;
            bool right = SeedManager::getRandomInt(chunkSeed, 0, 1) == 0;
            tree.addBranch(yOff, bWidth, right, trunkColor);
            
            // Procedurally add vines hanging from branches
            if (SeedManager::getRandomInt(chunkSeed, 0, 100) < 40) {
                float vLen = SeedManager::getRandomFloat(chunkSeed, 100.f, 400.f);
                float vXOff = right ? trunkWidth + (bWidth * SeedManager::getRandomFloat(chunkSeed, 0.2f, 0.8f)) : -(bWidth * SeedManager::getRandomFloat(chunkSeed, 0.2f, 0.8f));
                tree.addVine(vXOff, yOff, vLen);
            }
        }

        // Smaller, tighter canopies to prevent overwhelming the screen and blocking jump visibility
        float radius = SeedManager::getRandomFloat(chunkSeed, 60.f, 130.f) * biome.treeSizeMultiplier * biome.canopyDensity;
        float cOff = SeedManager::getRandomFloat(chunkSeed, heightBase - 50.f, heightBase + 20.f);
        int clusters = static_cast<int>(2 * biome.canopyDensity) + SeedManager::getRandomInt(chunkSeed, 1, 3);
        tree.buildCanopy(chunkSeed, radius, cOff, leafColor, clusters);

        tree.initDynamicMesh(); // Lock geometry array allocations on generation thread
        trees.push_back(tree);
    }
    return trees;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome) {
    std::vector<Decoration> decs;
    
    int numClusters = SeedManager::getRandomInt(chunkSeed, biome.minDecorations / 4, biome.maxDecorations / 4);
    
    for (int i = 0; i < numClusters; ++i) {
        float clusterCenter = startX + SeedManager::getRandomFloat(chunkSeed, 0.f, width);
        int type = SeedManager::getRandomInt(chunkSeed, 0, 2);
        int itemsInCluster = SeedManager::getRandomInt(chunkSeed, 3, 8);
        
        for(int j = 0; j < itemsInCluster; ++j) {
            float x = clusterCenter + SeedManager::getRandomFloat(chunkSeed, -100.f, 100.f);
            float y = TerrainGenerator::getTerrainHeight(x, worldSeed);
            float scale = SeedManager::getRandomFloat(chunkSeed, 0.5f, 1.5f);
            
            sf::Color color;
            if (type == 0) color = sf::Color(80, 85, 90); // Rocks
            else if (type == 1) color = interpolateColor(biome.leafColorMin, biome.leafColorMax, SeedManager::getRandomFloat(chunkSeed, 0.f, 1.f)); // Bushes
            else color = sf::Color(60, 40, 20); // Logs

            decs.emplace_back(x, y, type, scale, color);
        }
    }
    return decs;
}