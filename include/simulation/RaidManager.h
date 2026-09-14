#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include "entities/Ape.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"

enum class RaiderState {
    Marching,
    Looting,
    Retreating,
    Escaped
};

struct RaiderUnit {
    std::unique_ptr<Ape> ape;
    RaiderState state = RaiderState::Marching;
    float targetX = 0.f;
    float retreatBorderX = 0.f;
    float speed = 240.f;
    float lootTimer = 0.f;
    int direction = 1;
    bool carryingLoot = false;
    int stolenFood = 0;
    int stolenWood = 0;
};

struct ActiveRaid {
    sim::VillageID targetVillageId = 0;
    sim::VillageID originVillageId = 0;
    std::string originRealmName;
    std::string targetVillageName;
    float targetX = 0.f;
    float retreatBorderX = 0.f;
    bool isAgainstPlayer = false;
    std::vector<RaiderUnit> squad;
    bool alertActive = false;
    float alertTimer = 0.f;
    std::string alertMessage;
};

class RaidManager {
private:
    sf::Texture* apeTexture = nullptr;
    const sf::Font* font = nullptr;
    std::vector<ActiveRaid> activeRaids;
    float tensionRaidTimer = 0.f;
    float pulseTime = 0.f;

public:
    RaidManager();

    void init(sf::Texture& texture, const sf::Font& font);
    void spawnRaid(sim::VillageID targetVId, sim::VillageID originVId,
                   const std::string& originName, const std::string& targetName,
                   float targetX, float spawnBorderX, float retreatBorderX,
                   bool againstPlayer, int squadSize = 5);
    void update(float dt, float playerX, WorldManager* worldManager, sim::SimulationRegistry& registry);
    void draw(sf::RenderTarget& rt);
    void drawUI(sf::RenderWindow& window, const sf::View& letterboxView);
    void triggerDebugRaidOnPlayer(sim::SimulationRegistry& registry, float playerX);
    bool hasActiveRaids() const { return !activeRaids.empty(); }
};