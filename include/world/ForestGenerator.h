#pragma once
#include <vector>
#include "entities/Tree.h"

class ForestGenerator {
public:
    static std::vector<Tree> generateTrees(float startX, float width);
};