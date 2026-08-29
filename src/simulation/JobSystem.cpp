#include "simulation/JobSystem.h"
#include "simulation/WorldEventManager.h"
#include "simulation/KingdomManager.h"
#include "simulation/WarfareManager.h"
#include "world/WorldManager.h"
#include "entities/Tree.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>

namespace sim {

EntityID JobSystem::findNearestNode(SimulationRegistry& registry, float x, float y, ResourceType type, VillageData* village) {
    EntityID best = 0;
    float bestDist = 999999.f;
    for (auto& pair : registry.getAllResources()) {
        if (pair.second.type == type && pair.second.amount > 0) {
            float dist = std::abs(pair.second.worldX - x);
            if (village && dist > village->territoryRadius * 1.5f) continue;
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
    s.isPlanned = false;
    s.isUnderConstruction = true;
    s.isFinished = false;

    if (type == StructureType::Nest) { s.reqWood = 10; s.reqStone = 0; s.maxProgress = 20.f; }
    else if (type == StructureType::StorageHut) { s.reqWood = 30; s.reqStone = 10; s.maxProgress = 60.f; }
    else if (type == StructureType::WatchPlatform) { s.reqWood = 20; s.reqStone = 0; s.maxProgress = 40.f; }
    else { s.reqWood = 5; s.reqStone = 5; s.maxProgress = 10.f; }

    registry.registerStructure(s);
    village.constructionQueue.push_back(s.id);
}

void JobSystem::villagePlanningAI(SimulationRegistry& registry, VillageData& village, uint64_t ticks) {
    if (ticks % 150 != 0) return;

    if (village.food == 0 && village.members.size() > 2) {
        if (std::rand() % 100 < 5 && !village.isMigrating) {
            village.isMigrating = true;
            village.migrationTargetX = village.centerX + ((std::rand() % 2 == 0) ? 5000.f : -5000.f);
            HistoricalRecord rec;
            rec.year = registry.getYear();
            rec.day = registry.getDay();
            rec.description = village.name + " began a desperate migration due to famine.";
            registry.addHistory(rec);
        }
    }

    if (village.isMigrating) {
        village.centerX += (village.migrationTargetX > village.centerX) ? 10.f : -10.f;
        if (std::abs(village.centerX - village.migrationTargetX) < 100.f) {
            village.isMigrating = false;
            HistoricalRecord rec;
            rec.year = registry.getYear();
            rec.day = registry.getDay();
            rec.description = village.name + " settled in a new region.";
            registry.addHistory(rec);
        }
        return;
    }

    if (village.food > 40 && village.wood > 10 && village.toolsBasket < 5) {
        village.food -= 10;
        village.wood -= 5;
        village.toolsBasket++;
    }
    if (village.stone > 10 && village.wood > 10 && village.toolsAxe < 5) {
        village.stone -= 5;
        village.wood -= 5;
        village.toolsAxe++;
    }
    if (village.stone > 15 && village.wood > 5 && village.toolsPick < 5) {
        village.stone -= 10;
        village.wood -= 5;
        village.toolsPick++;
    }

    int numStorage = 1;
    int numNests = 0;
    
    for (StructureID sid : village.finishedStructures) {
        StructureData* s = registry.getStructure(sid);
        if (s) {
            if (s->type == StructureType::StorageHut) numStorage++;
            if (s->type == StructureType::Nest) numNests++;
        }
    }
    for (StructureID sid : village.constructionQueue) {
        StructureData* s = registry.getStructure(sid);
        if (s) {
            if (s->type == StructureType::StorageHut) numStorage++;
            if (s->type == StructureType::Nest) numNests++;
        }
    }

    if (village.members.size() > static_cast<size_t>(numNests * 2)) {
        spawnStructure(registry, village, StructureType::Nest);
    }
    else if (village.food + village.wood + village.stone > 80 * numStorage) {
        spawnStructure(registry, village, StructureType::StorageHut);
    }

    if (village.members.size() > 5) {
        village.territoryRadius += 10.0f;
    }
}

void JobSystem::handleLeader(SimulationRegistry&, ApeData& leader, VillageData&) {
    leader.currentJob = Job::Guard;
    leader.currentTargetNode = 0;
    leader.currentTargetStructure = 0;
    leader.skills.leadership += 0.005f;
}

void JobSystem::updateReputations(SimulationRegistry& registry, uint64_t ticks) {
    if (ticks % 300 != 0) return;
    auto& villages = registry.getAllVillages();
    for (auto it1 = villages.begin(); it1 != villages.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != villages.end(); ++it2) {
            float dist = std::abs(it1->second.centerX - it2->second.centerX);
            if (dist < (it1->second.territoryRadius + it2->second.territoryRadius)) {
                it1->second.knownVillages.insert(it2->first);
                it2->second.knownVillages.insert(it1->first);
                if (it1->second.relations.find(it2->first) == it1->second.relations.end()) {
                    it1->second.relations[it2->first] = Reputation::Suspicious;
                    it2->second.relations[it1->first] = Reputation::Suspicious;
                }
            }
        }
    }
}

void JobSystem::updateJobs(SimulationRegistry& registry, float timeOfDay, uint64_t totalTicks) {
    bool isNight = (timeOfDay > 0.8f || timeOfDay < 0.2f);
    bool isEvening = (timeOfDay >= 0.7f && timeOfDay <= 0.8f);

    WorldEventManager::update(registry, totalTicks);
    updateReputations(registry, totalTicks);
    KingdomManager::update(registry, totalTicks);
    WarfareManager::update(registry, totalTicks);

    for (auto& pair : registry.getAllResources()) {
        if (pair.second.amount < pair.second.maxAmount) {
            float mod = (pair.second.type == ResourceType::Food) ? WorldEventManager::getFoodGrowthModifier(registry, pair.second.worldX) : 1.0f;
            pair.second.regrowTimer += 1.0f * mod;
            if (pair.second.regrowTimer > (pair.second.type == ResourceType::Food ? 300.f : 800.f)) {
                pair.second.amount++;
                pair.second.regrowTimer = 0.f;
            }
        }
    }

    for (auto& pair : registry.getAllVillages()) {
        villagePlanningAI(registry, pair.second, totalTicks);
    }

    EntityID playerControlledId = registry.getControlledApe();

    for (auto& pair : registry.getAllApes()) {
        ApeData& ape = pair.second;
        if (!ape.alive) continue;

        if (ape.id != playerControlledId && ape.equippedTool == ToolType::None && ape.currentTargetStructure == 0 &&
            (ape.currentJob == Job::Idle || ape.currentJob == Job::Wander || ape.currentJob == Job::Socialize)) {
            for (auto& sPair : registry.getAllStructures()) {
                StructureData& s = sPair.second;
                if (s.villageId == ape.villageId && s.type == StructureType::ToolRack && s.isFinished && s.axeCount > 0) {
                    s.axeCount--;
                    s.claimedAxes++;
                    ape.currentTargetStructure = s.id;
                    ape.hasTravelDestination = true;
                    ape.travelDestinationX = s.worldX;
                    std::cout << "[TOOL] Ape " << ape.id << " (" << ape.name << ") claimed Axe\n";
                    std::cout << "[TOOL] Ape " << ape.id << " walking to Tool Rack\n";
                    break;
                }
            }
        }

        if (ape.currentTargetStructure != 0) {
            StructureData* s = registry.getStructure(ape.currentTargetStructure);
            if (s && s->type == StructureType::ToolRack) {
                if (std::abs(ape.worldX - s->worldX) <= 35.0f) {
                    s->claimedAxes = std::max(0, s->claimedAxes - 1);
                    ape.equippedTool = ToolType::StoneAxe;
                    ape.currentJob = Job::Woodcutter;
                    ape.currentOccupation = Occupation::Woodcutter;
                    ape.currentGoal = Goal::GatherWood;
                    ape.currentTargetStructure = 0;
                    ape.hasTravelDestination = false;
                    std::cout << "[TOOL] Ape " << ape.id << " (" << ape.name << ") acquired Axe\n";
                    std::cout << "[JOB] Ape " << ape.id << " -> WOODCUTTER\n";
                } else {
                    ape.hasTravelDestination = true;
                    ape.travelDestinationX = s->worldX;
                }
            }
        }

        if (ape.currentJob == Job::Woodcutter) {
            WorldManager* wm = registry.getWorldManager();
            if (wm) {
                if (!ape.hasTravelDestination || ape.currentTargetNode == 0) {
                    std::vector<Tree*> nearby = wm->getNearbyTrees(ape.worldX, 2500.f);
                    Tree* targetTree = nullptr;
                    float bestDist = 99999.f;

                    for (Tree* t : nearby) {
                        if (!t || t->getHarvestState() == TreeHarvestState::Harvested) continue;
                        float dist = std::abs(ape.worldX - t->getTrunkCenter());
                        if (dist < bestDist) {
                            bestDist = dist;
                            targetTree = t;
                        }
                    }

                    if (targetTree) {
                        ape.currentTargetNode = static_cast<EntityID>(targetTree->getId());
                        ape.travelDestinationX = targetTree->getTrunkCenter();
                        ape.hasTravelDestination = true;
                    }
                } else {
                    if (std::abs(ape.worldX - ape.travelDestinationX) <= 25.0f) {
                        int treeId = static_cast<int>(ape.currentTargetNode);
                        wm->harvestTree(treeId);
                        ape.hasTravelDestination = false;
                        ape.currentTargetNode = 0;
                        ape.skills.woodcutting += 0.05f;
                    }
                }
            }
            continue;
        }

        if (ape.hasTravelDestination && ape.currentJob != Job::Builder && ape.currentJob != Job::CarryResource) {
            ape.currentJob = Job::March;
            float dist = ape.travelDestinationX - ape.worldX;
            
            if (std::abs(dist) > 10.0f) {
                ape.worldX += (dist > 0 ? 1.0f : -1.0f) * 6.0f; 
            } else {
                ape.worldX = ape.travelDestinationX;
                if (std::abs(ape.worldX - ape.homeX) < 50.f) {
                    ape.hasTravelDestination = false;
                    ape.currentJob = Job::Idle;
                }
            }
            continue;
        }

        ape.hunger -= 0.05f;
        if (ape.hunger < 0.f) ape.hunger = 0.f;

        VillageData* village = registry.getVillage(ape.villageId);

        if (ape.currentArmyId != 0) {
            ArmyData* army = registry.getArmy(ape.currentArmyId);
            if (army) {
                if (army->objective == ArmyObjective::Muster) {
                    ape.currentJob = Job::Muster;
                    ape.currentTargetNode = 0;
                    if (std::abs(ape.worldX - army->worldX) > 50.0f) {
                        ape.worldX += (army->worldX > ape.worldX) ? 2.0f : -2.0f;
                    }
                } else if (army->objective == ArmyObjective::March) {
                    ape.currentJob = Job::March;
                    ape.worldX = army->worldX; 
                } else if (army->objective == ArmyObjective::Attack) {
                    ape.currentJob = Job::Combat;
                }
                continue;
            } else {
                ape.currentArmyId = 0;
                ape.currentJob = Job::Idle;
            }
        }
        
        if (totalTicks % 60 == 0) {
            for (auto& otherPair : registry.getAllApes()) {
                if (otherPair.first != ape.id && otherPair.second.alive) {
                    if (std::abs(ape.worldX - otherPair.second.worldX) < 150.0f) {
                        if (ape.currentKingdom != 0 && otherPair.second.currentKingdom != 0 && ape.currentKingdom != otherPair.second.currentKingdom) {
                            KingdomData* myK = registry.getKingdom(ape.currentKingdom);
                            if (myK) {
                                DiplomacyStatus status = myK->relations[otherPair.second.currentKingdom];
                                if (status == DiplomacyStatus::War) {
                                    ape.currentJob = Job::Combat;
                                    ape.currentCombatTarget = otherPair.first;
                                } else if (status == sim::DiplomacyStatus::Rival) {
                                    ape.currentJob = Job::Intimidate;
                                } else if (status == sim::DiplomacyStatus::Neutral) {
                                    ape.currentJob = Job::Observe;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if (ape.currentJob == Job::Observe || ape.currentJob == Job::Intimidate) {
            if (totalTicks % 300 == 0) ape.currentJob = Job::Idle;
            continue;
        }

        if (village && ape.equippedTool == ToolType::None) {
            if (village->toolsBasket > 0 && ape.currentOccupation == Occupation::Unemployed) {
                ape.equippedTool = ToolType::Basket;
                village->toolsBasket--;
            } else if (village->toolsAxe > 0 && ape.skills.woodcutting > 1.2f) {
                ape.equippedTool = ToolType::StoneAxe;
                village->toolsAxe--;
            } else if (village->toolsPick > 0 && ape.skills.gathering > 1.2f) {
                ape.equippedTool = ToolType::StonePick;
                village->toolsPick--;
            } else if (village->toolsSpear > 0) {
                ape.equippedTool = ToolType::WoodenSpear;
                village->toolsSpear--;
            }
        }
        
        bool danger = false;
        for (auto& animPair : registry.getAllAnimals()) {
            if (animPair.second.state == AnimalState::Hunting && animPair.second.targetId == ape.id) {
                ape.currentJob = Job::Flee;
                ape.currentTargetNode = 0;
                ape.currentTargetStructure = 0;
                danger = true;
                break;
            }
        }
        if (danger) continue;

        if (village && ape.id == village->leaderId) {
            handleLeader(registry, ape, *village);
            continue;
        }

        if (isNight && ape.currentJob != Job::Builder && ape.currentJob != Job::CarryResource) {
            ape.currentJob = Job::Sleep;
            ape.currentTargetNode = 0;
            ape.currentTargetStructure = 0;
            continue;
        }

        if (isEvening && ape.currentJob != Job::Builder && ape.currentJob != Job::CarryResource) {
            if (ape.carriedAmount > 0) {
                ape.currentJob = Job::CarryResource;
            } else {
                ape.currentJob = Job::Socialize;
            }
            ape.currentTargetNode = 0;
            ape.currentTargetStructure = 0;
            continue;
        }

        if (ape.carriedAmount > 0 && ape.currentJob != Job::Builder) {
            ape.currentJob = Job::CarryResource;
            if (ape.currentTargetStructure != 0) {
                StructureData* s = registry.getStructure(ape.currentTargetStructure);
                if (s && std::abs(ape.worldX - s->worldX) < 100.f) {
                    if (ape.carriedType == ResourceType::Wood) s->curWood += ape.carriedAmount;
                    if (ape.carriedType == ResourceType::Stone) s->curStone += ape.carriedAmount;
                    ape.carriedAmount = 0;
                    ape.carriedType = ResourceType::None;
                    ape.currentJob = Job::Builder;
                    ape.skills.building += 0.02f;
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

        if (ape.hunger < 40.f && ape.currentJob != Job::Builder && ape.currentJob != Job::CarryResource) {
            if (village && village->food > 0 && std::abs(ape.worldX - village->centerX) < 150.f) {
                ape.currentJob = Job::Eat;
                village->food--;
                ape.hunger = 100.f;
            } else {
                ape.currentJob = Job::Forage;
                if (ape.currentTargetNode == 0) ape.currentTargetNode = findNearestNode(registry, ape.worldX, ape.worldY, ResourceType::Food, village);
            }
            continue;
        }

        if (!ape.hasTravelDestination && ape.currentJob != Job::Builder && (ape.currentJob == Job::Idle || ape.currentJob == Job::Wander || ape.currentJob == Job::Socialize || ape.currentJob == Job::Eat)) {
            if (village) {
                if (ape.equippedTool == ToolType::WoodenSpear && village->kingdomId != 0) {
                    ape.currentJob = Job::Patrol;
                }
                else if (village->food < static_cast<int>(village->members.size() * 3) || registry.getSeason() == Season::Autumn) {
                    ape.currentJob = Job::Forage;
                    ape.currentTargetNode = findNearestNode(registry, ape.worldX, ape.worldY, ResourceType::Food, village);
                }
                else if (!village->constructionQueue.empty() && !village->isMigrating) {
                    ape.currentJob = Job::Builder;
                    ape.currentTargetStructure = village->constructionQueue.front();
                }
                else {
                    ape.currentJob = (std::rand() % 100 < 15) ? Job::Scout : Job::Wander;
                }
            }
        }

        if (ape.currentJob == Job::Forage || ape.currentJob == Job::StoneGatherer) {
            if (ape.currentTargetNode != 0) {
                ResourceNode* node = registry.getResource(ape.currentTargetNode);
                if (node && node->amount > 0) {
                    if (std::abs(ape.worldX - node->worldX) < 50.f) {
                        node->amount--;
                        ape.carriedType = node->type;
                        int capacity = (ape.equippedTool == ToolType::Basket) ? 3 : 1;
                        if (ape.equippedTool == ToolType::StonePick && node->type == ResourceType::Stone) capacity = 2;
                        ape.carriedAmount = capacity;
                        ape.currentTargetNode = 0;
                        if (node->type == ResourceType::Stone) ape.skills.gathering += 0.01f;
                        if (node->type == ResourceType::Food) ape.skills.gathering += 0.01f;
                    }
                } else {
                    ape.currentJob = Job::Idle;
                }
            }
        }
        else if (ape.currentJob == Job::Builder) {
            if (ape.currentTargetStructure != 0) {
                StructureData* s = registry.getStructure(ape.currentTargetStructure);
                if (s && s->isUnderConstruction && !s->isFinished) {
                    s->currentBuilder = ape.id;
                    float dist = std::abs(ape.worldX - s->worldX);

                    if (dist <= 65.f) {
                        ape.hasTravelDestination = false;
                        s->progress += 1.5f * ape.skills.building;

                        if (s->progress >= s->maxProgress) {
                            s->progress = s->maxProgress;
                            s->isUnderConstruction = false;
                            s->isFinished = true;
                            s->currentBuilder = 0;

                            if (village) {
                                village->finishedStructures.push_back(s->id);
                                village->constructionQueue.erase(
                                    std::remove(village->constructionQueue.begin(), village->constructionQueue.end(), s->id),
                                    village->constructionQueue.end()
                                );
                            }

                            ape.currentJob = Job::Idle;
                            ape.currentTargetStructure = 0;
                            ape.hasTravelDestination = false;
                            ape.skills.building += 0.05f;
                        }
                    } else {
                        ape.hasTravelDestination = true;
                        ape.travelDestinationX = s->worldX;
                    }
                } else {
                    ape.currentJob = Job::Idle;
                    ape.currentTargetStructure = 0;
                    ape.hasTravelDestination = false;
                }
            }
        }
        else if (ape.currentJob == Job::Scout) {
            ape.skills.scouting += 0.005f;
            if (village) {
                float targetEdge = (std::rand() % 2 == 0) ? (village->centerX + village->territoryRadius * 1.5f) : (village->centerX - village->territoryRadius * 1.5f);
                if (std::abs(ape.worldX - targetEdge) < 100.f) {
                    ape.currentJob = Job::Idle;
                }
                
                for (const auto& vPair : registry.getAllVillages()) {
                    if (vPair.first != village->id && std::abs(ape.worldX - vPair.second.centerX) < vPair.second.territoryRadius) {
                        if (village->kingdomId != 0 && vPair.second.kingdomId != 0 && village->kingdomId != vPair.second.kingdomId) {
                            KingdomManager::handleFirstContact(registry, village->kingdomId, vPair.second.kingdomId);
                        }
                    }
                }
            }
        }
        else if (ape.currentJob == Job::Patrol) {
            if (ape.currentKingdom != 0) {
                KingdomData* kd = registry.getKingdom(ape.currentKingdom);
                if (kd) {
                    float targetEdge = (std::rand() % 2 == 0) ? kd->territoryMaxX : kd->territoryMinX;
                    if (std::abs(ape.worldX - targetEdge) < 200.f) {
                        ape.currentJob = Job::Idle;
                    }
                }
            }
        }
    }
}

}