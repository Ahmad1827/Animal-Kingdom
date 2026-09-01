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

    float toolRackX = village.centerX - 700.0f;
    float sparringGroundX = village.centerX + 500.0f;

    if (ape.councilRole == CouncilRole::WarChief) {
        ape.equippedTool = ToolType::WoodenSpear;

        if (isDay) {
            if (hour >= 5.5f && hour < 9.0f) {
                if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 60.f) {
                    ape.travelDestinationX = (ape.worldX < village.centerX) ? (maxX - 120.f) : (minX + 120.f);
                    ape.hasTravelDestination = true;
                    ape.currentJob = Job::Patrol;
                }
                return;
            }

            if (hour >= 9.0f && hour < 10.5f) {
                ape.travelDestinationX = toolRackX;
                ape.hasTravelDestination = true;
                ape.currentJob = Job::Builder;
                return;
            }

            if (hour >= 10.5f && hour < 13.0f) {
                float marchLeft = village.centerX - 350.0f;
                float marchRight = village.centerX + 350.0f;

                if (!ape.hasTravelDestination || std::abs(ape.worldX - ape.travelDestinationX) < 40.f) {
                    ape.travelDestinationX = (ape.worldX < village.centerX) ? marchRight : marchLeft;
                    ape.hasTravelDestination = true;
                    ape.currentJob = Job::Patrol;
                }

                float marchDir = (ape.travelDestinationX > ape.worldX) ? -1.0f : 1.0f;
                int traineeSlot = 0;
                for (EntityID memberId : village.members) {
                    if (memberId == ape.id || memberId == village.leaderId) continue;
                    ApeData* trainee = registry.getApe(memberId);
                    if (!trainee || !trainee->alive || trainee->councilRole != CouncilRole::None) continue;

                    float followDistance = 45.0f + static_cast<float>(traineeSlot * 40.0f);
                    trainee->travelDestinationX = ape.worldX + (marchDir * followDistance);
                    trainee->hasTravelDestination = true;
                    trainee->equippedTool = ToolType::WoodenSpear;
                    trainee->currentJob = Job::March;

                    traineeSlot++;
                    if (traineeSlot >= 3) break;
                }
                return;
            }

            if (hour >= 13.0f && hour < 16.0f) {
                ape.travelDestinationX = sparringGroundX;
                ape.hasTravelDestination = true;
                ape.currentJob = Job::Combat;

                if (std::abs(ape.worldX - sparringGroundX) < 50.f) {
                    ape.skills.combat = std::min(10.0f, ape.skills.combat + dt * 0.04f);
                    ape.skills.leadership = std::min(10.0f, ape.skills.leadership + dt * 0.03f);

                    int traineeSlot = 0;
                    for (EntityID memberId : village.members) {
                        if (memberId == ape.id || memberId == village.leaderId) continue;
                        ApeData* trainee = registry.getApe(memberId);
                        if (!trainee || !trainee->alive || trainee->councilRole != CouncilRole::None) continue;

                        float slotTargetX = sparringGroundX + (traineeSlot == 0 ? 55.0f : (90.0f + static_cast<float>(traineeSlot * 45.0f)));
                        trainee->travelDestinationX = slotTargetX;
                        trainee->hasTravelDestination = true;
                        trainee->equippedTool = ToolType::WoodenSpear;
                        trainee->currentJob = Job::Combat;

                        if (std::abs(trainee->worldX - slotTargetX) < 50.f) {
                            trainee->skills.combat = std::min(10.0f, trainee->skills.combat + dt * (0.06f + ape.skills.leadership * 0.02f));
                        }

                        traineeSlot++;
                        if (traineeSlot >= 3) break;
                    }
                }
                return;
            }

            if (hour >= 16.0f && hour <= 19.5f) {
                ape.travelDestinationX = village.throneX + 75.0f;
                ape.hasTravelDestination = true;
                ape.currentJob = Job::Guard;
                return;
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

    if (ape.councilRole == CouncilRole::None && isDay && hour >= 10.5f && hour < 16.0f) {
        if (ape.currentJob == Job::Combat || ape.currentJob == Job::March) {
            return;
        }
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
            ape.equippedTool = ToolType::None;
        } else {
            ape.travelDestinationX = village.centerX + static_cast<float>((static_cast<int>(ape.id) % 7) * 40 - 120);
            ape.hasTravelDestination = true;
            ape.currentJob = Job::Sleep;
            ape.equippedTool = ToolType::None;
        }
    }
}

}