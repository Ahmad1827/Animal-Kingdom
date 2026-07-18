#include "core/SaveManager.h"
#include <fstream>

bool SaveManager::saveGame(const std::string& filename, const SaveData& data) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(&data), sizeof(SaveData));
    return true;
}

bool SaveManager::loadGame(const std::string& filename, SaveData& data) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) return false;
    in.read(reinterpret_cast<char*>(&data), sizeof(SaveData));
    return true;
}