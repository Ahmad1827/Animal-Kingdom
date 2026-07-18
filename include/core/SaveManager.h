#pragma once
#include <string>
#include <SFML/System/Vector2.hpp>
#include <cstdint>

struct SaveData {
    uint32_t worldSeed;
    float worldTime;
    sf::Vector2f playerPosition;
};

class SaveManager {
public:
    static bool saveGame(const std::string& filename, const SaveData& data);
    static bool loadGame(const std::string& filename, SaveData& data);
};