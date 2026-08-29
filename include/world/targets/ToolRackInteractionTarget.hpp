#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

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

    std::string getInteractionType() const override {
        return "ToolRack";
    }

    sf::Vector2f getInteractionPosition() const override {
        return sf::Vector2f(worldX, worldY);
    }

    bool canInteract() const override {
        sim::StructureData* s = registry.getStructure(structureId);
        if (!s || !s->isFinished) return false;
        return true;
    }

    std::string getInteractionTitle() const override {
        return "TOOL RACK";
    }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> menu;
        sim::StructureData* s = registry.getStructure(structureId);
        sim::ApeData* actor = registry.getApe(registry.getControlledApe());
        if (!s || !actor) return menu;

        if (s->axeCount > 0) {
            menu.push_back({
                "Stone Axe on Rack (Awaiting worker)",
                []() {}
            });
        } else if (s->claimedAxes > 0) {
            menu.push_back({
                "Stone Axe (Claimed by worker)",
                []() {}
            });
        } else {
            bool canAfford = (actor->amberCount >= 2);
            std::string label = canAfford ? "Buy Woodcutter Axe (Cost: 2 Amber)" : "Buy Woodcutter Axe (Need 2 Amber)";

            menu.push_back({
                label,
                [this, actor, s, canAfford]() {
                    if (!canAfford) return;
                    if (actor->amberCount >= 2 && s->axeCount == 0 && s->claimedAxes == 0) {
                        int oldAmber = actor->amberCount;
                        actor->amberCount -= 2;
                        s->axeCount += 1;
                        std::cout << "[TOOL] Axe purchase requested\n";
                        std::cout << "[ECONOMY] Amber " << oldAmber << " -> " << actor->amberCount << "\n";
                        std::cout << "[TOOL] Axe placed on rack\n";
                    }
                }
            });
        }

        return menu;
    }
};