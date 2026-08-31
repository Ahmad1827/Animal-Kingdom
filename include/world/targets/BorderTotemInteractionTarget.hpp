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

    static constexpr float WALL_HALF_WIDTH = 104.0f;
    static constexpr float WALL_TO_BORDER_OFFSET = WALL_HALF_WIDTH * 2.0f;

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
        return isMin ? "WEST PERIMETER GATE" : "EAST PERIMETER GATE";
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

        float currentWallX = isMin ? (v->borderMinX + WALL_TO_BORDER_OFFSET) : (v->borderMaxX - WALL_TO_BORDER_OFFSET);
        for (sim::EntityID sId : v->finishedStructures) {
            sim::StructureData* s = registry.getStructure(sId);
            if (s && s->type == sim::StructureType::SimpleBarrier) {
                if (isMin && s->worldX < v->centerX) {
                    currentWallX = s->worldX;
                    break;
                } else if (!isMin && s->worldX > v->centerX) {
                    currentWallX = s->worldX;
                    break;
                }
            }
        }

        const float EXPANSION_STEP = 500.0f;
        float proposedTargetWallX = isMin ? (currentWallX - EXPANSION_STEP) : (currentWallX + EXPANSION_STEP);

        float limitX = isMin ? -999999.0f : 999999.0f;
        auto clearanceZones = WorldGenerator::getClearanceZones(0);

        for (const auto& z : clearanceZones) {
            if (z.type == ClearanceType::MeetingGround) {
                float mid = (z.minX + z.maxX) * 0.5f;
                if (isMin && mid < v->centerX && mid > limitX) {
                    limitX = z.maxX + WALL_TO_BORDER_OFFSET + 50.0f;
                } else if (!isMin && mid > v->centerX && mid < limitX) {
                    limitX = z.minX - WALL_TO_BORDER_OFFSET - 50.0f;
                }
            }
        }

        bool reachedLimit = false;
        if (isMin && proposedTargetWallX <= limitX) {
            proposedTargetWallX = limitX;
            if (std::abs(currentWallX - proposedTargetWallX) < 80.0f) reachedLimit = true;
        } else if (!isMin && proposedTargetWallX >= limitX) {
            proposedTargetWallX = limitX;
            if (std::abs(currentWallX - proposedTargetWallX) < 80.0f) reachedLimit = true;
        }

        if (reachedLimit) {
            menu.push_back({
                "Perimeter reached neutral meeting ground limit",
                []() {}
            });
            return menu;
        }

        int amber = actor ? actor->amberCount : 0;
        const int AMBER_COST = 3;
        std::string label = (amber >= AMBER_COST)
            ? "Expand Perimeter (Cost: 3 Amber) [Have: " + std::to_string(amber) + "]"
            : "Expand Perimeter [Need 3 Amber, Have: " + std::to_string(amber) + "]";

        menu.push_back({
            label,
            [this, v, proposedTargetWallX]() {
                if (!v || v->isExpandingBorder) return;
                sim::ApeData* a = registry.getApe(registry.getControlledApe());
                if (!a) return;
                if (a->amberCount >= 3) {
                    int oldAmber = a->amberCount;
                    a->amberCount -= 3;
                    v->isExpandingBorder = true;
                    v->expandingSideRight = !isMin;
                    v->targetBorderX = proposedTargetWallX;
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

                    std::cout << "[BORDER] Expansion ordered. Amber " << oldAmber << " -> " << a->amberCount << "\n";
                    std::cout << "[BORDER] New Target Wall X = " << proposedTargetWallX << "\n";
                } else {
                    std::cout << "[BORDER] Expansion failed: Need 3 amber, have " << a->amberCount << "\n";
                }
            }
        });

        return menu;
    }
};