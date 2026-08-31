#pragma once
#include <cstdint>
#include <vector>

namespace SettlementLayout {

float getPlayerCenterX();
float getPlayerTerritoryRadius();
float getVillageTerritoryRadius();
int getVillageCount(uint32_t worldSeed);
std::vector<float> getVillageCenters(uint32_t worldSeed);
bool isSettlementArea(float worldX, uint32_t worldSeed, float buffer = 800.0f);

}