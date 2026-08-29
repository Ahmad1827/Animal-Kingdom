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
    float currentX;
    float currentY;

public:
    BorderTotemInteractionTarget(sim::EntityID targetId, sim::SimulationRegistry& registry,
                                 WorldManager* worldManager, bool isMin)
        : targetId(targetId), registry(registry), worldManager(worldManager), isMin(isMin) {
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
        return isMin ? "WEST PERIMETER GATE" : "EAST PERIMETER GATE";
    }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> menu;
        sim::VillageData* v = registry.getVillage(targetId);
        sim::ApeData* actor = registry.getApe(registry.getControlledApe());
        if (!actor) return menu;

        if (!v) {
            sim::KingdomData* k = registry.getKingdom(targetId);
            if (k) {
                menu.push_back({
                    "Kingdom Frontier Marker (" + k->name + ")",
                    []() {}
                });
            }
            return menu;
        }

        if (v->isExpandingBorder) {
            menu.push_back({
                "Workers relocating perimeter...",
                []() {}
            });
            return menu;
        }

        const float EXPANSION_STEP = 500.0f;
        float proposedTargetX = isMin ? (v->borderMinX - EXPANSION_STEP) : (v->borderMaxX + EXPANSION_STEP);

        float limitX = isMin ? -999999.0f : 999999.0f;
        auto clearanceZones = WorldGenerator::getClearanceZones(0);

        for (const auto& z : clearanceZones) {
            if (z.type == ClearanceType::MeetingGround) {
                float mid = (z.minX + z.maxX) * 0.5f;
                if (isMin && mid < v->centerX && mid > limitX) {
                    limitX = z.maxX;
                } else if (!isMin && mid > v->centerX && mid < limitX) {
                    limitX = z.minX;
                }
            }
        }

        bool reachedLimit = false;
        if (isMin && proposedTargetX <= limitX) {
            proposedTargetX = limitX + 60.0f;
            if (std::abs(v->borderMinX - proposedTargetX) < 80.0f) reachedLimit = true;
        } else if (!isMin && proposedTargetX >= limitX) {
            proposedTargetX = limitX - 60.0f;
            if (std::abs(v->borderMaxX - proposedTargetX) < 80.0f) reachedLimit = true;
        }

        if (reachedLimit) {
            menu.push_back({
                "Border reached neutral meeting ground",
                []() {}
            });
            return menu;
        }

        const int AMBER_COST = 3;
        bool canAfford = (actor->amberCount >= AMBER_COST);
        std::string label = canAfford ? "Expand Perimeter (Cost: 3 Amber)" : "Expand Perimeter (Need 3 Amber)";

        menu.push_back({
            label,
            [this, actor, v, canAfford, proposedTargetX]() {
                if (!canAfford || v->isExpandingBorder) return;
                if (actor->amberCount >= 3) {
                    int oldAmber = actor->amberCount;
                    actor->amberCount -= 3;
                    v->isExpandingBorder = true;
                    v->expandingSideRight = !isMin;
                    v->targetBorderX = proposedTargetX;
                    v->borderMoverApe = 0;

                    for (sim::EntityID sId : v->finishedStructures) {
                        sim::StructureData* s = registry.getStructure(sId);
                        if (s && s->type == sim::StructureType::SimpleBarrier) {
                            if (isMin && s->worldX < v->centerX) {
                                v->borderStructureId = s->id;
                                break;
                            } else if (!isMin && s->worldX > v->centerX) {
                                v->borderStructureId = s->id;
                                break;
                            }
                        }
                    }

                    std::cout << "[BORDER] Expansion ordered. Amber " << oldAmber << " -> " << actor->amberCount << "\n";
                    std::cout << "[BORDER] New Target X = " << proposedTargetX << "\n";
                }
            }
        });

        return menu;
    }
};