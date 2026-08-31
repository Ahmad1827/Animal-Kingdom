#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include "world/WorldGenerator.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

class BorderTotemInteractionTarget : public InteractionTarget {
private:
    sim::EntityID targetId;
    sim::SimulationRegistry& registry;
    WorldManager* worldManager;
    bool isMin;
    sim::EntityID actorId;
    float currentX;
    float currentY;

public:
    BorderTotemInteractionTarget(sim::EntityID targetId, sim::SimulationRegistry& registry,
                                 WorldManager* worldManager, bool isMin, sim::EntityID actorId = 0)
        : targetId(targetId), registry(registry), worldManager(worldManager), isMin(isMin), actorId(actorId) {
        sim::VillageData* v = registry.getVillage(targetId);
        sim::KingdomData* k = registry.getKingdom(targetId);
        if (v) {
            currentX = isMin ? v->borderMinX : v->borderMaxX;
        } else if (k) {
            currentX = isMin ? k->territoryMinX : k->territoryMaxX;
        } else {
            currentX = 0.0f;
        }
        currentY = worldManager ? worldManager->getTerrainHeight(currentX) : 500.0f;
    }

    std::string getInteractionType() const override {
        return "BorderPalisade";
    }

    sf::Vector2f getInteractionPosition() const override {
        return sf::Vector2f(currentX, currentY);
    }

    bool canInteract() const override {
        sim::VillageData* v = registry.getVillage(targetId);
        if (v) {
            return !v->isExpandingBorder;
        }
        return true;
    }

    std::string getInteractionTitle() const override {
        return isMin ? "WEST BORDER TOTEM" : "EAST BORDER TOTEM";
    }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> menu;
        sim::VillageData* v = registry.getVillage(targetId);
        sim::ApeData* actor = registry.getApe(registry.getControlledApe());

        if (!v) {
            sim::KingdomData* k = registry.getKingdom(targetId);
            if (k) {
                menu.push_back({
                    "Kingdom Frontier Marker (" + k->name + ")",
                    nullptr
                });
            }
            return menu;
        }

        if (v->isExpandingBorder) {
            menu.push_back({
                "Workers relocating border marker...",
                nullptr
            });
            return menu;
        }

        float currentBorderX = isMin ? v->borderMinX : v->borderMaxX;
        const float EXPANSION_STEP = 500.0f;
        float proposedTargetBorderX = isMin ? (currentBorderX - EXPANSION_STEP) : (currentBorderX + EXPANSION_STEP);

        float limitX = isMin ? -999999.0f : 999999.0f;
        auto clearanceZones = WorldGenerator::getClearanceZones(0);

        for (const auto& z : clearanceZones) {
            if (z.type == ClearanceType::MeetingGround) {
                float mid = (z.minX + z.maxX) * 0.5f;
                if (isMin && mid < v->centerX && mid > limitX) {
                    limitX = z.maxX + 50.0f;
                } else if (!isMin && mid > v->centerX && mid < limitX) {
                    limitX = z.minX - 50.0f;
                }
            }
        }

        bool reachedLimit = false;
        if (isMin && proposedTargetBorderX <= limitX) {
            proposedTargetBorderX = limitX;
            if (std::abs(currentBorderX - proposedTargetBorderX) < 80.0f) reachedLimit = true;
        } else if (!isMin && proposedTargetBorderX >= limitX) {
            proposedTargetBorderX = limitX;
            if (std::abs(currentBorderX - proposedTargetBorderX) < 80.0f) reachedLimit = true;
        }

        if (reachedLimit) {
            menu.push_back({
                "Border reached neutral meeting ground limit",
                nullptr
            });
            return menu;
        }

        int amber = actor ? actor->amberCount : 0;
        const int AMBER_COST = 3;
        std::string label = (amber >= AMBER_COST)
            ? "Expand Border (Cost: 3 Amber) [Have: " + std::to_string(amber) + "]"
            : "Expand Border [Need 3 Amber, Have: " + std::to_string(amber) + "]";

        menu.push_back({
            label,
            [this, v, proposedTargetBorderX]() {
                if (!v || v->isExpandingBorder) return;
                sim::ApeData* a = registry.getApe(registry.getControlledApe());
                if (!a) return;
                if (a->amberCount >= 3) {
                    int oldAmber = a->amberCount;
                    a->amberCount -= 3;
                    v->isExpandingBorder = true;
                    v->expandingSideRight = !isMin;
                    v->targetBorderX = proposedTargetBorderX;
                    v->borderMoverApe = 0;

                    std::cout << "[BORDER] Expansion ordered. Amber " << oldAmber << " -> " << a->amberCount << "\n";
                    std::cout << "[BORDER] New Target Border X = " << proposedTargetBorderX << "\n";
                } else {
                    std::cout << "[BORDER] Expansion failed: Need 3 amber, have " << a->amberCount << "\n";
                }
            }
        });

        return menu;
    }
};