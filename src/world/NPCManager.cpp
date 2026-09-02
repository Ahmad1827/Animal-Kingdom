#include "world/NPCManager.h"
#include "simulation/ApeBehaviorSystem.h"
#include <cmath>
#include <algorithm>

NPCManager::NPCManager(sf::Texture& texture) : apeTexture(texture) {}

void NPCManager::update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, sim::SimulationManager& simManager, WorldManager* worldManager, float timeOfDay) {
    sim::SimulationRegistry& registry = simManager.getRegistry();
    sim::EntityID controlledId = simManager.getControlledApe();
    auto& allApes = registry.getAllApes();

    for (auto& pair : allApes) {
        sim::ApeData& data = pair.second;
        if (!data.alive || data.id == controlledId) continue;

        sim::VillageData* village = registry.getVillage(data.villageId);
        if (village) {
            sim::ApeBehaviorSystem::updateApeRoleRoutine(data, *village, registry, dt, timeOfDay);
        }

        if (activeNPCs.find(data.id) == activeNPCs.end()) {
            activeNPCs[data.id] = std::make_unique<NPCApe>(data.id, data.worldX, data.worldY, apeTexture);
        }
    }

    for (auto it = activeNPCs.begin(); it != activeNPCs.end(); ) {
        sim::ApeData* data = registry.getApe(it->first);
        if (!data || !data->alive || data->id == controlledId) {
            it = activeNPCs.erase(it);
            continue;
        }

        bool isKing = false;
        if (data->currentKingdom != 0) {
            sim::KingdomData* kd = registry.getKingdom(data->currentKingdom);
            if (kd && kd->currentKingId == data->id) isKing = true;
        }

        it->second->setVisualEquipment(data->equippedTool, data->carriedType, data->carriedAmount, isKing);
        it->second->setDepthLane(data->depthLane);
        it->second->update(dt, data, worldManager, timeOfDay, registry, controlledId);
        ++it;
    }
}

void NPCManager::removeNPC(sim::EntityID id) { 
    activeNPCs.erase(id); 
}

void NPCManager::draw(sf::RenderTarget& target) { 
    for (const auto& pair : activeNPCs) {
        pair.second->draw(target); 
    }
}

void NPCManager::drawLane(sf::RenderTarget& target, sim::DepthLane lane, const sim::SimulationRegistry& registry) {
    for (const auto& pair : activeNPCs) {
        const sim::ApeData* d = const_cast<sim::SimulationRegistry&>(registry).getApe(pair.first);
        if (d && d->depthLane == lane) {
            pair.second->draw(target);
        }
    }
}