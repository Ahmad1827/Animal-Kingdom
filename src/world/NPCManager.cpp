#include "world/NPCManager.h"

NPCManager::NPCManager(sf::Texture& texture) : apeTexture(texture) {}

void NPCManager::update(float dt, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, 
                        sim::SimulationManager& simManager, WorldManager* worldManager, float timeOfDay) {
    
    auto& allApes = simManager.getRegistry().getAllApes();

    // 1. Spawn NPCs that enter preload bounds
    for (auto& pair : allApes) {
        sim::ApeData& data = pair.second;
        if (!data.alive) continue;

        if (data.worldX >= preloadBounds.left && data.worldX <= preloadBounds.left + preloadBounds.width &&
            data.worldY >= preloadBounds.top && data.worldY <= preloadBounds.top + preloadBounds.height) {
            
            if (activeNPCs.find(data.id) == activeNPCs.end()) {
                activeNPCs[data.id] = std::make_unique<NPCApe>(data.id, data.worldX, data.worldY, apeTexture);
            }
        }
    }

    // 2. Update active NPCs and Despawn those outside unload bounds
    for (auto it = activeNPCs.begin(); it != activeNPCs.end(); ) {
        sim::ApeData* data = simManager.getRegistry().getApe(it->first);
        if (!data || !data->alive) {
            it = activeNPCs.erase(it);
            continue;
        }

        sf::FloatRect bounds = it->second->getBounds();
        if (bounds.left + bounds.width < unloadBounds.left || bounds.left > unloadBounds.left + unloadBounds.width ||
            bounds.top + bounds.height < unloadBounds.top || bounds.top > unloadBounds.top + unloadBounds.height) {
            
            it = activeNPCs.erase(it);
        } else {
            it->second->update(dt, data, worldManager, timeOfDay);
            ++it;
        }
    }
}

void NPCManager::draw(sf::RenderTarget& target) {
    for (const auto& pair : activeNPCs) {
        pair.second->draw(target);
    }
}