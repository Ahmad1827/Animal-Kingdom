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
    sim::EntityID actorId;
    float worldX;
    float worldY;

public:
    ToolRackInteractionTarget(sim::EntityID structureId, sim::VillageID villageId,
                              sim::SimulationRegistry& registry, WorldManager* worldManager,
                              sim::EntityID actorId = 0)
        : structureId(structureId), villageId(villageId), registry(registry),
          worldManager(worldManager), actorId(actorId) {
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
        if (!s) return menu;

        sim::ApeData* actor = registry.getApe(registry.getControlledApe());
        int amber = actor ? actor->amberCount : 0;

        if (s->axeCount > 0) {
            menu.push_back({
                "Stone Axe on Rack (Awaiting worker)",
                nullptr
            });
        } else if (s->claimedAxes > 0) {
            menu.push_back({
                "Stone Axe (Worker on the way to collect)",
                nullptr
            });
        } else {
            std::string label = (amber >= 2)
                ? "Buy Woodcutter Axe (Cost: 2 Amber) [Have: " + std::to_string(amber) + "]"
                : "Buy Woodcutter Axe [Need 2 Amber, Have: " + std::to_string(amber) + "]";

            menu.push_back({
                label,
                [this, s]() {
                    if (!s) return;
                    sim::ApeData* a = registry.getApe(registry.getControlledApe());
                    if (!a) return;
                    if (a->amberCount >= 2 && s->axeCount == 0 && s->claimedAxes == 0) {
                        int oldAmber = a->amberCount;
                        a->amberCount -= 2;
                        s->axeCount += 1;
                        std::cout << "[TOOL] Axe purchase requested\n";
                        std::cout << "[ECONOMY] Amber " << oldAmber << " -> " << a->amberCount << "\n";
                        std::cout << "[TOOL] Axe placed on rack\n";
                    }
                }
            });
        }

        return menu;
    }
};