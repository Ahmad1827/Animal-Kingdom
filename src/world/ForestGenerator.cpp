#include "world/ForestGenerator.h"
#include "world/TerrainGenerator.h"
#include <cstdlib>

std::vector<Tree> ForestGenerator::generateTrees(float startX, float width) {
    std::vector<Tree> trees;
    std::srand(static_cast<unsigned>(startX) * 19283); 

    int numTrees = 8 + (std::rand() % 7);
    float sectionWidth = width / numTrees;

    for (int i = 0; i < numTrees; ++i) {
        float x = startX + (i * sectionWidth) + (std::rand() % static_cast<int>(sectionWidth * 0.5f));
        float y = TerrainGenerator::getTerrainHeight(x);
        
        float height = 400.f + (std::rand() % 400);
        float trunkWidth = 30.f + (std::rand() % 30);

        Tree tree(x, y, trunkWidth, height);
        
        int numBranches = 4 + (std::rand() % 6);
        for (int j = 0; j < numBranches; ++j) {
            float yOff = 100.f + (j * 100.f) + (std::rand() % 60);
            if (yOff > height - 50.f) break;
            
            float bWidth = 100.f + (std::rand() % 200);
            bool right = (std::rand() % 2) == 0;
            tree.addBranch(yOff, bWidth, right);
        }
        trees.push_back(tree);
    }
    return trees;
}