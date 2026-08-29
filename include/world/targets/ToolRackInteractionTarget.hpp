#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include <cmath>
#include <iostream>

class ToolRackInteractionTarget : public InteractionTarget {
private:
    sim::EntityID structureId;
    sim::VillageID villageId;
    sim::SimulationRegistry& registry;
    WorldManager* worldManager;
    float worldX;
    float worldY;

public:
    ToolRackInteractionTarget(sim::EntityID structureId, sim::VillageID villageId,
                              sim::SimulationRegistry& registry, WorldManager* worldManager)
        : structureId(structureId), villageId(villageId), registry(registry), worldManager(worldManager) {
        sim::StructureData* s = registry.getStructure(structureId);
        if (s) {
            worldX = s->worldX;
            worldY = s->worldY;
        } else {
            worldX = 0.f;
            worldY = 500.f;
        }
    }

    sf::Vector2f getPosition() const override {
        return sf::Vector2f(worldX, worldY);
    }

    sf::FloatRect getBounds() const override {
        return sf::FloatRect(worldX - 45.f, worldY - 70.f, 90.f, 70.f);
    }

    bool canInteract(sim::EntityID actorId) const override {
        sim::StructureData* s = registry.getStructure(structureId);
        if (!s || !s->isFinished) return false;

        sim::ApeData* actor = registry.getApe(actorId);
        if (!actor) return false;

        if (actor->villageId != villageId && actor->currentKingdom == 0) return false;

        return true;
    }

    void onInteract(sim::EntityID actorId) override {
        sim::ApeData* actor = registry.getApe(actorId);
        sim::StructureData* s = registry.getStructure(structureId);
        if (!actor || !s) return;

        if (s->axeCount > 0 || s->claimedAxes > 0) return;

        const int AXE_COST = 2;
        if (actor->amberCount >= AXE_COST) {
            int oldAmber = actor->amberCount;
            actor->amberCount -= AXE_COST;
            s->axeCount += 1;

            std::cout << "[TOOL] Axe purchase requested\n";
            std::cout << "[ECONOMY] Amber " << oldAmber << " -> " << actor->amberCount << "\n";
            std::cout << "[TOOL] Axe placed on rack\n";
        }
    }

    std::string getPromptText() const override {
        sim::StructureData* s = registry.getStructure(structureId);
        if (!s) return "";

        sim::ApeData* actor = registry.getApe(registry.getControlledApe());
        if (s->axeCount > 0) {
            return "Tool Rack (Axe Available)";
        }
        if (s->claimedAxes > 0) {
            return "Tool Rack (Axe Claimed)";
        }
        if (actor && actor->amberCount < 2) {
            return "Woodcutter Axe (Need 2 Amber)";
        }
        return "Woodcutter Axe (Cost: 2 Amber)";
    }

    std::string getActionText() const override {
        sim::StructureData* s = registry.getStructure(structureId);
        sim::ApeData* actor = registry.getApe(registry.getControlledApe());
        if (s && (s->axeCount > 0 || s->claimedAxes > 0)) {
            return "Axe on Rack";
        }
        if (actor && actor->amberCount < 2) {
            return "Not Enough Amber";
        }
        return "Hold to Buy Axe";
    }

    float getHoldDuration() const override {
        return 0.35f;
    }
};