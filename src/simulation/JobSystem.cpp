#include "simulation/JobSystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace sim {

EntityID JobSystem::findNearestNode(SimulationRegistry& registry, float x, float y, ResourceType type) {
    EntityID best = 0;
    float bestDist = 999999.f;
    for (auto& pair : registry.getAllResources()) {
        if (pair.second.type == type && pair.second.amount > 0) {
            float dist = std::abs(pair.second.worldX - x);
            if (dist < bestDist) {
                bestDist = dist;
                best = pair.first;
            }
        }
    }
    return best;
}

void JobSystem::spawnStructure(SimulationRegistry& registry, VillageData& village, StructureType type) {
    StructureData s;
    s.id = IDGenerator::generateStructureID();
    s.type = type;
    s.villageId = village.id;
    
    float offset = ((std::rand() % 1000) - 500.f);
    s.worldX = village.centerX + offset;
    s.worldY = village.centerY;
    
    s.progress = 0.f;
    s.currentBuilder = 0;
    s.curWood = 0;
    s.curStone = 0;
    s.isFinished = false;

    if (type == StructureType::Nest) { s.reqWood = 10; s.reqStone = 0; s.maxProgress = 20.f; }
    else if (type == StructureType::StorageHut) { s.reqWood = 30; s.reqStone = 10; s.maxProgress = 60.f; }
    else if (type == StructureType::WatchPlatform) { s.reqWood = 20; s.reqStone = 0; s.maxProgress = 40.f; }
    else { s.reqWood = 5; s.reqStone = 5; s.maxProgress = 10.f; }

    registry.registerStructure(s);
    village.constructionQueue.push_back(s.id);
}

void JobSystem::villagePlanningAI(SimulationRegistry& registry, VillageData& village, uint64_t ticks) {
    if (ticks % 100 != 0) return;

    int numStorage = 1;
    int numNests = 0;
    for (StructureID sid : village.finishedStructures) {
        StructureData* s = registry.getStructure(sid);
        if (s) {
            if (s->type == StructureType::StorageHut) numStorage++;
            if (s->type == StructureType::Nest) numNests++;
        }
    }

    if (village.members.size() > numNests * 2) {
        spawnStructure(registry, village, StructureType::Nest);
    }
    else if (village.food + village.wood + village.stone > 80 * numStorage) {
        spawnStructure(registry, village, StructureType::StorageHut);
    }
}

void JobSystem::handleLeader(SimulationRegistry& registry, ApeData& leader) {
    leader.currentJob = Job::Guard;
    leader.currentTargetNode = 0;
    leader.currentTargetStructure = 0;
}

void JobSystem::updateJobs(SimulationRegistry& registry, float timeOfDay, uint64_t totalTicks) {
    bool isNight = (timeOfDay > 0.8f || timeOfDay < 0.2f);
    bool isEvening = (timeOfDay >= 0.7f && timeOfDay <= 0.8f);

    for (auto& pair : registry.getAllResources()) {
        if (pair.second.amount < pair.second.maxAmount) {
            pair.second.regrowTimer += 1.0f;
            if (pair.second.regrowTimer > (pair.second.type == ResourceType::Food ? 300.f : 800.f)) {
                pair.second.amount++;
                pair.second.regrowTimer = 0.f;
            }
        }
    }

    for (auto& pair : registry.getAllVillages()) {
        villagePlanningAI(registry, pair.second, totalTicks);
    }

    for (auto& pair : registry.getAllApes()) {
        ApeData& ape = pair.second;
        if (!ape.alive) continue;

        ape.hunger -= 0.05f;
        if (ape.hunger < 0.f) ape.hunger = 0.f;

        VillageData* village = registry.getVillage(ape.villageId);
        
        if (village && ape.id == village->leaderId) {
            handleLeader(registry, ape);
            continue;
        }

        if (isNight) {
            ape.currentJob = Job::Sleep;
            ape.currentTargetNode = 0;
            ape.currentTargetStructure = 0;
            continue;
        }

        if (isEvening) {
            if (ape.carriedAmount > 0) {
                ape.currentJob = Job::CarryResource;
            } else {
                ape.currentJob = Job::Socialize;
            }
            ape.currentTargetNode = 0;
            ape.currentTargetStructure = 0;
            continue;
        }

        if (ape.carriedAmount > 0) {
            ape.currentJob = Job::CarryResource;
            
            if (ape.currentTargetStructure != 0) {
                StructureData* s = registry.getStructure(ape.currentTargetStructure);
                if (s && std::abs(ape.worldX - s->worldX) < 100.f) {
                    if (ape.carriedType == ResourceType::Wood) s->curWood += ape.carriedAmount;
                    if (ape.carriedType == ResourceType::Stone) s->curStone += ape.carriedAmount;
                    ape.carriedAmount = 0;
                    ape.carriedType = ResourceType::None;
                    ape.currentJob = Job::Builder;
                }
                continue;
            }
            
            if (village && std::abs(ape.worldX - village->centerX) < 100.f) {
                if (ape.carriedType == ResourceType::Food) village->food += ape.carriedAmount;
                else if (ape.carriedType == ResourceType::Wood) village->wood += ape.carriedAmount;
                else if (ape.carriedType == ResourceType::Stone) village->stone += ape.carriedAmount;
                
                ape.carriedAmount = 0;
                ape.carriedType = ResourceType::None;
                ape.currentJob = Job::Idle;
            }
            continue;
        }

        if (ape.hunger < 40.f) {
            if (village && village->food > 0 && std::abs(ape.worldX - village->centerX) < 150.f) {
                ape.currentJob = Job::Eat;
                village->food--;
                ape.hunger = 100.f;
            } else {
                ape.currentJob = Job::Forage;
                if (ape.currentTargetNode == 0) ape.currentTargetNode = findNearestNode(registry, ape.worldX, ape.worldY, ResourceType::Food);
            }
            continue;
        }

        if (ape.currentJob == Job::Idle || ape.currentJob == Job::Wander || ape.currentJob == Job::Socialize || ape.currentJob == Job::Eat) {
            if (village) {
                if (village->food < static_cast<int>(village->members.size() * 2)) {
                    ape.currentJob = Job::Forage;
                    ape.currentTargetNode = findNearestNode(registry, ape.worldX, ape.worldY, ResourceType::Food);
                }
                else if (!village->constructionQueue.empty()) {
                    ape.currentJob = Job::Builder;
                    ape.currentTargetStructure = village->constructionQueue.front();
                }
                else if (village->wood < 30) {
                    ape.currentJob = Job::Woodcutter;
                    ape.currentTargetNode = findNearestNode(registry, ape.worldX, ape.worldY, ResourceType::Wood);
                }
                else if (village->stone < 20) {
                    ape.currentJob = Job::StoneGatherer;
                    ape.currentTargetNode = findNearestNode(registry, ape.worldX, ape.worldY, ResourceType::Stone);
                }
                else {
                    ape.currentJob = Job::Wander;
                }
            }
        }

        if (ape.currentJob == Job::Forage || ape.currentJob == Job::Woodcutter || ape.currentJob == Job::StoneGatherer) {
            if (ape.currentTargetNode != 0) {
                ResourceNode* node = registry.getResource(ape.currentTargetNode);
                if (node && node->amount > 0) {
                    if (std::abs(ape.worldX - node->worldX) < 50.f) {
                        node->amount--;
                        ape.carriedType = node->type;
                        ape.carriedAmount = (ape.equippedTool == ToolType::StoneAxe && node->type == ResourceType::Wood) ? 2 : 1;
                        ape.currentTargetNode = 0;
                    }
                } else {
                    ape.currentJob = Job::Idle;
                }
            }
        }
        else if (ape.currentJob == Job::Builder) {
            if (ape.currentTargetStructure != 0) {
                StructureData* s = registry.getStructure(ape.currentTargetStructure);
                if (s && !s->isFinished) {
                    s->currentBuilder = ape.id;
                    if (s->curWood < s->reqWood) {
                        if (village && village->wood > 0) {
                            if (std::abs(ape.worldX - village->centerX) < 100.f) {
                                village->wood--;
                                ape.carriedType = ResourceType::Wood;
                                ape.carriedAmount = 1;
                            }
                        }
                    } else if (s->curStone < s->reqStone) {
                        if (village && village->stone > 0) {
                            if (std::abs(ape.worldX - village->centerX) < 100.f) {
                                village->stone--;
                                ape.carriedType = ResourceType::Stone;
                                ape.carriedAmount = 1;
                            }
                        }
                    } else {
                        if (std::abs(ape.worldX - s->worldX) < 100.f) {
                            s->progress += 1.0f;
                            if (s->progress >= s->maxProgress) {
                                s->isFinished = true;
                                village->finishedStructures.push_back(s->id);
                                village->constructionQueue.erase(
                                    std::remove(village->constructionQueue.begin(), village->constructionQueue.end(), s->id),
                                    village->constructionQueue.end()
                                );
                                ape.currentJob = Job::Idle;
                                ape.currentTargetStructure = 0;
                            }
                        }
                    }
                } else {
                    ape.currentJob = Job::Idle;
                    ape.currentTargetStructure = 0;
                }
            }
        }
    }
}

}