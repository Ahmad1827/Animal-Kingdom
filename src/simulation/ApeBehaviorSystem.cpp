#include "simulation/ApeBehaviorSystem.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace sim {

void ApeBehaviorSystem::updateApeRoleRoutine(ApeData& ape, VillageData& village, SimulationRegistry& registry, float dt, float timeOfDay) {
    if (!ape.alive) return;

    if (village.isGatheringActive) {
        ape.equippedTool = ToolType::None;
        ape.hasTravelDestination = true;
        ape.travelDestinationX = village.throneX + static_cast<float>((static_cast<int>(ape.id) % 5) * 40 - 80);
        ape.currentJob = Job::Gathering;
        return;
    }

    float hour = (timeOfDay <= 1.0f) ? (timeOfDay * 24.0f) : timeOfDay;
    bool isDay = (hour >= 5.5f && hour <= 19.5f);

    float minX = village.borderMinX;
    float maxX = village.borderMaxX;
    if (maxX <= minX || (maxX - minX < 800.f)) {
        minX = village.centerX - 1600.f;
        maxX = village.centerX + 1600.f;
    }

    if (ape.councilRole == CouncilRole::WarChief) {
        ape.equippedTool = ToolType::WoodenSpear;
        if (isDay) {
            if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 60.f) {
                if (ape.worldX < village.centerX) {
                    ape.travelDestinationX = maxX - 120.f;
                } else {
                    ape.travelDestinationX = minX + 120.f;
                }
                ape.hasTravelDestination = true;
                ape.currentJob = Job::Patrol;
            }
        } else {
            if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 60.f) {
                ape.travelDestinationX = village.centerX + static_cast<float>((static_cast<int>(ape.id) % 2 == 0 ? 250.f : -250.f));
                ape.hasTravelDestination = true;
                ape.currentJob = Job::Guard;
            }
        }
        return;
    }

    if (ape.councilRole == CouncilRole::ChiefBuilder) {
        ape.equippedTool = ToolType::StonePick;
        EntityID activeConstructionId = 0;
        float targetBuildingX = village.centerX;

        for (auto& sPair : registry.getAllStructures()) {
            if (sPair.second.villageId == village.id && !sPair.second.isFinished) {
                activeConstructionId = sPair.first;
                targetBuildingX = sPair.second.worldX;
                break;
            }
        }

        if (activeConstructionId != 0) {
            ape.currentTargetStructure = activeConstructionId;
            ape.travelDestinationX = targetBuildingX;
            ape.hasTravelDestination = true;
            ape.currentJob = Job::Builder;

            StructureData* s = registry.getStructure(activeConstructionId);
            if (s && std::abs(ape.worldX - targetBuildingX) < 60.f) {
                s->progress += dt * (1.0f + ape.skills.building * 0.5f);
                if (s->progress >= s->maxProgress) {
                    s->progress = s->maxProgress;
                    s->isFinished = true;
                    s->isUnderConstruction = false;
                    village.finishedStructures.push_back(s->id);
                }
            }
        } else {
            if (isDay) {
                if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 50.f) {
                    float plotOffset = static_cast<float>(((static_cast<int>(ape.id) % 3) - 1) * 350.f);
                    ape.travelDestinationX = village.centerX + plotOffset;
                    ape.hasTravelDestination = true;
                    ape.currentJob = Job::Idle;
                }
            } else {
                ape.travelDestinationX = village.centerX;
                ape.hasTravelDestination = true;
                ape.currentJob = Job::Sleep;
            }
        }
        return;
    }

    if (ape.councilRole == CouncilRole::LeadForager) {
        ape.equippedTool = ToolType::Basket;
        if (isDay) {
            if (ape.amberCount >= ape.maxAmber || ape.carriedAmount >= 20) {
                ape.travelDestinationX = village.centerX;
                ape.hasTravelDestination = true;
                ape.currentJob = Job::CarryResource;

                if (std::abs(ape.worldX - village.centerX) < 60.f) {
                    village.food += ape.carriedAmount;
                    ape.carriedAmount = 0;
                    ape.carriedType = ResourceType::None;
                    ape.amberCount = 0;
                }
            } else {
                if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 50.f) {
                    if (ape.worldX < village.centerX) {
                        ape.travelDestinationX = maxX + 450.f;
                    } else {
                        ape.travelDestinationX = minX - 450.f;
                    }
                    ape.hasTravelDestination = true;
                    ape.currentJob = Job::Forage;
                }

                if (std::abs(ape.worldX - ape.travelDestinationX) < 60.f) {
                    ape.amberCount = std::min(ape.maxAmber, ape.amberCount + 1);
                    ape.carriedAmount = std::min(20, ape.carriedAmount + 2);
                    ape.carriedType = ResourceType::Food;
                }
            }
        } else {
            ape.travelDestinationX = village.centerX + static_cast<float>((static_cast<int>(ape.id) % 5) * 40 - 80);
            ape.hasTravelDestination = true;
            ape.currentJob = Job::Sleep;
        }
        return;
    }

    if (ape.councilRole == CouncilRole::Shaman) {
        ape.equippedTool = ToolType::Torch;
        float meetingGroundX = village.centerX;
        if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 40.f) {
            float wanderSpot = meetingGroundX + static_cast<float>((static_cast<int>(ape.id * 31) % 300) - 150);
            ape.travelDestinationX = wanderSpot;
            ape.hasTravelDestination = true;
            ape.currentJob = Job::Socialize;
        }
        return;
    }

    if (ape.currentJob == Job::Guard) {
        ape.equippedTool = ToolType::WoodenSpear;
        if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 50.f) {
            float guardSpot = (static_cast<int>(ape.id) % 2 == 0) ? (minX + 80.f) : (maxX - 80.f);
            ape.travelDestinationX = guardSpot;
            ape.hasTravelDestination = true;
        }
        return;
    }

    if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 40.f) {
        if (isDay) {
            float span = std::max(400.f, maxX - minX - 300.f);
            float roamX = minX + 150.f + static_cast<float>(static_cast<int>(ape.id * 73) % static_cast<int>(span));
            ape.travelDestinationX = roamX;
            ape.hasTravelDestination = true;
            ape.currentJob = Job::Wander;
        } else {
            ape.travelDestinationX = village.centerX + static_cast<float>((static_cast<int>(ape.id) % 7) * 40 - 120);
            ape.hasTravelDestination = true;
            ape.currentJob = Job::Sleep;
        }
    }
}

}