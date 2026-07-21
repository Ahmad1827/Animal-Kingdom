#pragma once
#include <string>

struct Animation {
    std::string name;
    int startX;
    int startY;
    int frameWidth;
    int frameHeight;
    int frameCount;
    float frameDuration;
    bool looping;
    float offsetX; // Rendering alignment X
    float offsetY; // Rendering alignment Y
};