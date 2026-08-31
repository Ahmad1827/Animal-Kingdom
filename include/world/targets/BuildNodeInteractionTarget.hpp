#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

class BuildNodeInteractionTarget : public InteractionTarget {
private:
    sim::EntityID structureId;
    sim::VillageID villageId;
    sim::SimulationRegistry& registry;
    WorldManager* worldManager;
    float worldX;
    float worldY;

public:
    BuildNodeInteractionTarget(sim::EntityID structureId, sim::VillageID villageId,
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
        return "BuildNode";
    }

    sf::Vector2f getInteractionPosition() const override {
        return sf::Vector2f(worldX, worldY);
    }

    bool canInteract() const override {
        sim::StructureData* s = registry.getStructure(structureId);
        if (!s || s->isFinished) return false;
        return true;
    }

    std::string getInteractionTitle() const override {
        sim::StructureData* s = registry.getStructure(structureId);
        return s ? s->name : "CONSTRUCTION SITE";
    }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> menu;
        sim::StructureData* s = registry.getStructure(structureId);
        sim::VillageData* v = registry.getVillage(villageId);
        sim::ApeData* actor = registry.getApe(registry.getControlledApe());

        if (!s || !v || !actor) return menu;

        if (s->isUnderConstruction) {
            menu.push_back({
                "Under Construction (Progress: " + std::to_string(static_cast<int>((s->progress / std::max(1.f, s->maxProgress)) * 100.f)) + "%)",
                nullptr
            });
            return menu;
        }

        std::string costStr = "Cost: " + std::to_string(s->reqWood) + " Wood, " + std::to_string(s->reqStone) + " Stone";
        menu.push_back({
            costStr,
            nullptr
        });

        bool canBuild = (v->wood >= s->reqWood && v->stone >= s->reqStone);
        std::string buildLabel = canBuild ? "Begin Construction" : "Insufficient Tribal Materials";

        menu.push_back({
            buildLabel,
            [this, s, v, canBuild]() {
                if (!canBuild || !s || !v || s->isUnderConstruction) return;
                v->wood -= s->reqWood;
                v->stone -= s->reqStone;
                s->isPlanned = false;
                s->isUnderConstruction = true;
                v->constructionQueue.push_back(s->id);
                std::cout << "[BUILD] Construction queued for " << s->name << "\n";
            }
        });

        return menu;
    }
};