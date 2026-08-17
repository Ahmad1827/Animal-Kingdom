#include "entities/NPCApe.h"
#include "entities/Tree.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <iostream>

NPCApe::NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture) 
    : simId(id), physicalApe(x, y, texture, false), stateTimer(0.f), intendedMoveX(0.f), 
      isDroppingToHang(false), grabbedChunk(0), grabbedVine(-1), grabbedSeg(-1), 
      pauseTimer(0.f), workTimer(0.f), woodcutLogTimer(0.f), fontLoaded(false) {
    baseSpeedMultiplier = 1.0f;
    personalOffset = ((id * 37) % 100) - 50.f; 

    static sf::Font sharedFont;
    static bool sharedFontLoaded = false;
    static bool fontAttempted = false;

    if (!fontAttempted) {
        fontAttempted = true;
        if (sharedFont.loadFromFile("assets/fonts/PressStart2P-Regular.ttf") ||
            sharedFont.loadFromFile("assets/fonts/font.ttf") ||
            sharedFont.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
            sharedFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")) {
            sharedFontLoaded = true;
        }
    }

    if (sharedFontLoaded) {
        fontLoaded = true;
        nameText.setFont(sharedFont);
        nameText.setCharacterSize(10);
        nameText.setFillColor(sf::Color::Yellow);
        nameText.setOutlineColor(sf::Color::Black);
        nameText.setOutlineThickness(1.5f);
    }
}

bool NPCApe::hasTrait(sim::ApeData* data, sim::Trait trait) {
    for (auto t : data->traits) {
        if (t == trait) return true;
    }
    return false;
}

void NPCApe::fireAudioHook(const std::string& soundEvent) {
}

void NPCApe::determineNextAction(sim::ApeData* data, float timeOfDay, sim::SimulationRegistry& registry, sim::EntityID playerId) {
    sim::VillageData* v = registry.getVillage(data->villageId);

    if (data->carriedType == sim::ResourceType::Food) physicalApe.setCarriedItem(1);
    else if (data->carriedType == sim::ResourceType::Wood) physicalApe.setCarriedItem(2);
    else if (data->carriedType == sim::ResourceType::Stone) physicalApe.setCarriedItem(3);
    else physicalApe.setCarriedItem(0);

    if (data->currentJob == sim::Job::Woodcutter || data->currentTargetNode != 0) {
        return;
    }

    if (data->currentJob == sim::Job::Builder || data->currentJob == sim::Job::CarryResource) {
        float targetX = physicalApe.getPosition().x;
        float stopDistance = 60.f;
        bool shouldWorkAtTarget = false;

        if (data->currentJob == sim::Job::Builder) {
            if (data->currentTargetStructure != 0) {
                sim::StructureData* s = registry.getStructure(data->currentTargetStructure);
                if (s) {
                    targetX = s->worldX;
                    stopDistance = 60.f;
                    shouldWorkAtTarget = true;
                }
            }
        }
        else if (data->currentJob == sim::Job::CarryResource) {
            if (data->currentTargetStructure != 0) {
                sim::StructureData* s = registry.getStructure(data->currentTargetStructure);
                if (s) targetX = s->worldX;
            } else if (v) {
                targetX = v->centerX;
            }
            stopDistance = 40.f;
        }

        float dist = std::abs(physicalApe.getPosition().x - targetX);
        if (dist > stopDistance) {
            intendedMoveX = (physicalApe.getPosition().x < targetX) ? 1.f : -1.f;
            physicalApe.setState(ApeState::Grounded);
        } else {
            intendedMoveX = 0.f;
            if (shouldWorkAtTarget) {
                physicalApe.setState(ApeState::Working);
            } else {
                physicalApe.setState(ApeState::Grounded);
            }
        }
        return;
    }

    if (playerId != 0 && playerId != data->id) {
        sim::ApeData* player = registry.getApe(playerId);
        if (player && player->alive) {
            float playerX = player->worldX;
            float myX = physicalApe.getPosition().x;

            sim::EntityID hostId = player->scheduledAudienceHost;
            if (hostId != 0 && player->summonedRepId != hostId) {
                sim::ApeData* hostApe = registry.getApe(hostId);
                if (hostApe) {
                    float throneX = hostApe->worldX; 
                    bool isHostVillage = false;
                    
                    sim::VillageData* hostV = registry.getVillage(hostApe->villageId);
                    sim::KingdomData* hostK = (hostApe->currentKingdom != 0) ? registry.getKingdom(hostApe->currentKingdom) : nullptr;
                    
                    if (hostK && hostK->currentKingId == hostId) {
                        sim::VillageData* capV = registry.getVillage(hostK->capitalVillageId);
                        if (capV) { throneX = capV->centerX; isHostVillage = (data->villageId == capV->id); }
                    } else if (hostV && hostV->leaderId == hostId) {
                        throneX = hostV->centerX;
                        isHostVillage = (data->villageId == hostV->id);
                    }

                    if (std::abs(playerX - throneX) < 1500.f) {
                        if (data->id == hostId) {
                            if (std::abs(myX - throneX) > 20.f) {
                                intendedMoveX = (myX < throneX) ? 1.f : -1.f;
                                physicalApe.setState(ApeState::Grounded);
                            } else {
                                intendedMoveX = (myX < playerX) ? 0.001f : -0.001f; 
                                physicalApe.setState(ApeState::Grounded);
                            }
                            return;
                        } 
                        else if (isHostVillage) {
                            bool isLeft = (data->id % 2 == 0);
                            int posIdx = (data->id % 20);
                            float offset = 180.f + (posIdx * 45.f);
                            float targetX = throneX + (isLeft ? -offset : offset);
                            
                            if (std::abs(myX - targetX) > 20.f) {
                                intendedMoveX = (myX < targetX) ? 1.f : -1.f;
                                physicalApe.setState(ApeState::Grounded);
                            } else {
                                intendedMoveX = (myX < throneX) ? 0.001f : -0.001f;
                                physicalApe.setState(ApeState::Grounded);
                            }
                            return;
                        }
                    }
                }
            }

            float dist = std::abs(playerX - myX);

            if (dist < 500.0f) { 
                bool isVillageLeader = (v && v->leaderId == data->id);
                sim::KingdomData* kData = (data->currentKingdom != 0) ? registry.getKingdom(data->currentKingdom) : nullptr;
                bool isKing = (kData && kData->currentKingId == data->id);

                if (isVillageLeader || isKing) {
                    bool isFriendly = false, isSuspicious = false, isHostile = false;
                    
                    if (isKing) {
                        sim::KingdomData* pK = (player->currentKingdom != 0) ? registry.getKingdom(player->currentKingdom) : nullptr;
                        if (pK && kData->relations.count(pK->id)) {
                            sim::DiplomacyStatus status = kData->relations[pK->id];
                            if (status == sim::DiplomacyStatus::War) isHostile = true;
                            else if (status == sim::DiplomacyStatus::Rival) isSuspicious = true;
                            else if (status == sim::DiplomacyStatus::Friendly || status == sim::DiplomacyStatus::Alliance) isFriendly = true;
                        } else if (pK && kData->borderTension.count(pK->id)) {
                            if (kData->borderTension[pK->id] >= 50.f) isSuspicious = true; 
                        }
                    } else if (isVillageLeader) {
                        if (v->personalOpinions.count(playerId)) {
                            int op = v->personalOpinions[playerId];
                            if (op >= 30) isFriendly = true;
                            else if (op <= -30) isHostile = true;
                            else if (op <= -10) isSuspicious = true;
                        }
                    }

                    if (isFriendly || isSuspicious || isHostile) {
                        float hostileSafeDist = 300.f;
                        float suspiciousSafeDist = 180.f;
                        float friendlyGreetDist = 150.f;
                        
                        if (isFriendly) {
                            if (dist < friendlyGreetDist) {
                                intendedMoveX = (myX < playerX) ? 0.001f : -0.001f;
                                physicalApe.setState(ApeState::Grounded);
                                return;
                            }
                        } 
                        else {
                            float preferredDist = isHostile ? hostileSafeDist : suspiciousSafeDist;
                            
                            if (dist < preferredDist) {
                                intendedMoveX = (myX < playerX) ? -1.f : 1.f;
                            } else {
                                intendedMoveX = (myX < playerX) ? 0.001f : -0.001f; 
                            }
                            physicalApe.setState(ApeState::Grounded);
                            return;
                        }
                    }
                }
            }
        }
    }

    if (std::rand() % 100 < 5 && pauseTimer <= 0.f) {
        pauseTimer = 0.5f + (std::rand() % 15) / 10.f;
        intendedMoveX = 0.f;
        if (physicalApe.getState() != ApeState::Working) {
            physicalApe.setState(ApeState::Grounded);
        }
        return;
    }

    float targetX = physicalApe.getPosition().x;
    float stopDistance = 15.f;
    bool shouldWorkAtTarget = false;

    if (data->hasTravelDestination) {
        targetX = data->travelDestinationX;
        stopDistance = 10.f;
    } 
    else if (data->currentJob == sim::Job::Forage || data->currentJob == sim::Job::StoneGatherer) {
        if (data->currentTargetNode != 0) {
            sim::ResourceNode* node = registry.getResource(data->currentTargetNode);
            if (node) {
                targetX = node->worldX;
                shouldWorkAtTarget = true;
            }
        }
    } 
    else if (data->currentJob == sim::Job::Scout && v) {
        float scoutTarget = (data->id % 2 == 0) ? (v->centerX + v->territoryRadius * 1.5f) : (v->centerX - v->territoryRadius * 1.5f);
        targetX = scoutTarget;
        stopDistance = 100.f;
    } 
    else if (data->currentJob == sim::Job::Sleep || data->currentJob == sim::Job::ReturnHome) {
        targetX = data->homeX;
        stopDistance = 10.f;
    } 
    else if (data->currentJob == sim::Job::Guard && v) {
        float leftBorder = v->centerX - v->territoryRadius + 50.f;
        float rightBorder = v->centerX + v->territoryRadius - 50.f;
        
        bool goRight = (static_cast<int>(timeOfDay * 10.f + data->id) % 2 == 0);
        targetX = goRight ? rightBorder : leftBorder;
        
        if (std::abs(physicalApe.getPosition().x - targetX) < 100.f) {
            if (std::rand() % 100 < 10 && pauseTimer <= 0.f) {
                pauseTimer = 3.f + (std::rand() % 30) / 10.f; 
            }
        }
    } 
    else if (data->currentJob == sim::Job::Socialize || data->currentJob == sim::Job::Idle || data->currentJob == sim::Job::Wander) {
        if (v) {
            bool isKing = false;
            if (data->currentKingdom != 0) {
                sim::KingdomData* kd = registry.getKingdom(data->currentKingdom);
                if (kd && kd->currentKingId == data->id) isKing = true;
            }

            if (isKing) {
                if (std::rand() % 100 < 20) targetX = v->centerX + (std::rand() % 400 - 200); 
                else targetX = v->centerX;
                stopDistance = 40.f;
            } else {
                targetX = v->centerX + personalOffset; 
                stopDistance = 15.f;
            }
        }
    }

    if (std::abs(physicalApe.getPosition().x - targetX) > stopDistance) {
        intendedMoveX = (physicalApe.getPosition().x < targetX) ? 1.f : -1.f;
        physicalApe.setState(ApeState::Grounded);
    } else {
        intendedMoveX = 0.f;
        if (shouldWorkAtTarget) {
            physicalApe.setState(ApeState::Working);
        } else {
            physicalApe.setState(ApeState::Grounded);
            if (pauseTimer <= 0.f && std::rand() % 100 < 20) {
                pauseTimer = 1.0f + (std::rand() % 20) / 10.f;
            }
        }
    }
}

void NPCApe::applyPhysics(float dt, WorldManager* worldManager) {
    sf::FloatRect playerBounds = physicalApe.getBounds();
    sf::FloatRect platformBounds;
    bool wasGrounded = (physicalApe.getState() == ApeState::Grounded || physicalApe.getState() == ApeState::Working);

    if (physicalApe.getState() != ApeState::ClimbingTrunk && physicalApe.getState() != ApeState::HangingBranch && physicalApe.getState() != ApeState::ClimbingVine && physicalApe.getState() != ApeState::Working) {
        physicalApe.setState(ApeState::Airborne);
    }

    float groundHeight = worldManager->getTerrainHeight(playerBounds.left + playerBounds.width / 2.f);
    float bottomY = playerBounds.top + playerBounds.height;
    float distanceToGround = groundHeight - bottomY;
    
    if (physicalApe.getVelocity().y >= 0.f && bottomY >= groundHeight) {
        physicalApe.setPosition(physicalApe.getPosition().x, groundHeight - playerBounds.height);
        physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
        if (physicalApe.getState() != ApeState::Working) {
            physicalApe.setState(ApeState::Grounded);
        }
        physicalApe.setDroppingThrough(false);
        isDroppingToHang = false;
    }
    else if (wasGrounded && physicalApe.getVelocity().y >= 0.f && distanceToGround > 0.f && distanceToGround < 25.f) {
        physicalApe.setPosition(physicalApe.getPosition().x, groundHeight - playerBounds.height);
        physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
        if (physicalApe.getState() != ApeState::Working) {
            physicalApe.setState(ApeState::Grounded);
        }
        physicalApe.setDroppingThrough(false);
        isDroppingToHang = false;
    }

    sf::FloatRect checkBounds = playerBounds;
    sf::Vector2f checkVel = physicalApe.getVelocity();
    if (wasGrounded) { checkBounds.top += 2.f; if (checkVel.y == 0.f) checkVel.y = 10.f; }

    if (physicalApe.getState() == ApeState::Airborne && !physicalApe.isDroppingThrough()) {
        if (worldManager->checkOneWayCollision(checkBounds, checkVel, dt, platformBounds)) {
            physicalApe.setPosition(physicalApe.getPosition().x, platformBounds.top - playerBounds.height);
            physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
            if (physicalApe.getState() != ApeState::Working) {
                physicalApe.setState(ApeState::Grounded);
            }
            isDroppingToHang = false;
        }
    }
}

void NPCApe::update(float dt, sim::ApeData* data, WorldManager* worldManager, float timeOfDay, sim::SimulationRegistry& registry, sim::EntityID playerId) {
    if (fontLoaded && data) {
        nameText.setString(data->name);
        sf::FloatRect bounds = physicalApe.getBounds();
        sf::FloatRect textBounds = nameText.getLocalBounds();
        nameText.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height);
        nameText.setPosition(bounds.left + bounds.width / 2.f, bounds.top - 8.f);
    }

    bool isHarvestingMission = (data->currentJob == sim::Job::Woodcutter || (data->currentTargetNode != 0 && data->hasTravelDestination));

    if (isHarvestingMission && worldManager) {
        float myX = physicalApe.getPosition().x;
        float destX = (data->travelDestinationX != 0.f) ? data->travelDestinationX : myX;
        float distToDest = std::abs(myX - destX);

        if (distToDest > 45.f) {
            physicalApe.setState(ApeState::Grounded);
            intendedMoveX = (myX < destX) ? 1.f : -1.f;
            workTimer = 0.f;
        } else {
            intendedMoveX = 0.f;
            physicalApe.setVelocity(0.f, physicalApe.getVelocity().y);
            physicalApe.setState(ApeState::Working);

            workTimer += dt;
            if (workTimer >= 5.0f) {
                int targetTreeId = static_cast<int>(data->currentTargetNode);

                if (targetTreeId != 0) {
                    worldManager->harvestTree(targetTreeId);
                }
                worldManager->harvestTreeNear(destX, 200.f);
                worldManager->harvestTreeNear(myX, 200.f);

                data->currentJob = sim::Job::Idle;
                data->currentTargetNode = 0;
                data->hasTravelDestination = false;
                workTimer = 0.f;
                physicalApe.setState(ApeState::Grounded);

                std::cout << "[WOODCUT SUCCESS] " << data->name << " finished chopping after 5s! Tree removed." << std::endl << std::flush;
                return;
            }
        }
    }
    else if (data->currentJob == sim::Job::Builder && worldManager) {
        float myX = physicalApe.getPosition().x;
        sim::StructureData* s = registry.getStructure(data->currentTargetStructure);
        if (s && s->isUnderConstruction && !s->isFinished) {
            float dist = std::abs(myX - s->worldX);
            if (dist <= 65.f) {
                physicalApe.setState(ApeState::Working);
                intendedMoveX = 0.f;
                physicalApe.setVelocity(0.f, physicalApe.getVelocity().y);
                
                workTimer += dt;
                if (workTimer >= 0.8f) {
                    workTimer -= 0.8f;
                }
                s->progress += dt * 3.0f * data->skills.building;
                if (s->progress >= s->maxProgress) {
                    s->progress = s->maxProgress;
                    s->isUnderConstruction = false;
                    s->isFinished = true;
                    s->currentBuilder = 0;
                    sim::VillageData* v = registry.getVillage(data->villageId);
                    if (v) {
                        v->finishedStructures.push_back(s->id);
                        v->constructionQueue.erase(
                            std::remove(v->constructionQueue.begin(), v->constructionQueue.end(), s->id),
                            v->constructionQueue.end()
                        );
                    }
                    data->currentJob = sim::Job::Idle;
                    data->currentTargetStructure = 0;
                    data->hasTravelDestination = false;
                    workTimer = 0.f;
                    physicalApe.setState(ApeState::Grounded);
                }
            } else {
                physicalApe.setState(ApeState::Grounded);
                intendedMoveX = (myX < s->worldX) ? 1.f : -1.f;
            }
        } else {
            data->currentJob = sim::Job::Idle;
            data->currentTargetStructure = 0;
            data->hasTravelDestination = false;
            workTimer = 0.f;
            physicalApe.setState(ApeState::Grounded);
        }
    } else {
        if (pauseTimer > 0.f) {
            pauseTimer -= dt;
            intendedMoveX = 0.f; 
        } else {
            stateTimer -= dt;
            if (stateTimer <= 0.f) {
                determineNextAction(data, timeOfDay, registry, playerId);
                stateTimer = 0.2f + (std::rand() % 10) / 100.f; 
            }
        }
    }

    if (physicalApe.getState() == ApeState::Grounded || physicalApe.getState() == ApeState::Airborne || physicalApe.getState() == ApeState::Working) {
        float speed = 140.f * baseSpeedMultiplier;
        
        if (data->currentJob == sim::Job::Guard || data->currentJob == sim::Job::Scout) {
            speed = 220.f * baseSpeedMultiplier;
        }
        
        if (intendedMoveX != 0.f && physicalApe.getState() != ApeState::Working) {
            physicalApe.setVelocity(intendedMoveX * speed, physicalApe.getVelocity().y);
        } else {
            physicalApe.setVelocity(0.f, physicalApe.getVelocity().y);
        }
    }

    applyPhysics(dt, worldManager);
    physicalApe.update(dt);

    data->worldX = physicalApe.getPosition().x;
    data->worldY = physicalApe.getPosition().y;
    data->currentChunkX = static_cast<int>(std::floor(data->worldX / 2000.f));
}

void NPCApe::draw(sf::RenderTarget& target) {
    physicalApe.draw(target);
    if (fontLoaded) {
        target.draw(nameText);
    }
}