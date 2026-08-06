#include "entities/NPCApe.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

NPCApe::NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture) 
    : simId(id), physicalApe(x, y, texture, false), stateTimer(0.f), intendedMoveX(0.f), 
      isDroppingToHang(false), grabbedChunk(0), grabbedVine(-1), grabbedSeg(-1), pauseTimer(0.f) {
    
    // Give each ape a slightly different walking speed (90% to 110%)
    baseSpeedMultiplier = 0.9f + (std::rand() % 20) / 100.f;
    // Give each ape a personal spacing offset so they don't stand inside each other
    personalOffset = ((id * 37) % 100) - 50.f; 
}

bool NPCApe::hasTrait(sim::ApeData* data, sim::Trait trait) {
    for (auto t : data->traits) {
        if (t == trait) return true;
    }
    return false;
}

void NPCApe::determineNextAction(sim::ApeData* data, float timeOfDay, sim::SimulationRegistry& registry) {
    sim::VillageData* v = registry.getVillage(data->villageId);

    if (data->carriedType == sim::ResourceType::Food) physicalApe.setCarriedItem(1);
    else if (data->carriedType == sim::ResourceType::Wood) physicalApe.setCarriedItem(2);
    else if (data->carriedType == sim::ResourceType::Stone) physicalApe.setCarriedItem(3);
    else physicalApe.setCarriedItem(0);

    // Random micro-pauses while thinking
    if (std::rand() % 100 < 5 && pauseTimer <= 0.f) {
        pauseTimer = 0.5f + (std::rand() % 15) / 10.f; 
        intendedMoveX = 0.f;
        return;
    }

    float targetX = physicalApe.getPosition().x;
    float stopDistance = 15.f;

    if (data->currentJob == sim::Job::CarryResource) {
        if (data->currentTargetStructure != 0) {
            sim::StructureData* s = registry.getStructure(data->currentTargetStructure);
            if (s) targetX = s->worldX;
        } else if (v) {
            targetX = v->centerX;
        }
    } 
    else if (data->currentJob == sim::Job::Forage || data->currentJob == sim::Job::Woodcutter || data->currentJob == sim::Job::StoneGatherer) {
        if (data->currentTargetNode != 0) {
            sim::ResourceNode* node = registry.getResource(data->currentTargetNode);
            if (node) targetX = node->worldX;
        }
    } 
    else if (data->currentJob == sim::Job::Builder) {
        if (data->currentTargetStructure != 0) {
            sim::StructureData* s = registry.getStructure(data->currentTargetStructure);
            if (s) {
                if (s->curWood < s->reqWood || s->curStone < s->reqStone) {
                    targetX = v ? v->centerX : physicalApe.getPosition().x; 
                } else {
                    targetX = s->worldX;
                }
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
                pauseTimer = 3.f + (std::rand() % 30) / 10.f; // Guards pause to look around
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
                else targetX = v->centerX; // Supervise near bonfire
                stopDistance = 40.f;
            } else {
                targetX = v->centerX + personalOffset; // Gather near bonfire organically
                stopDistance = 15.f;
            }
        }
    }

    if (std::abs(physicalApe.getPosition().x - targetX) > stopDistance) {
        intendedMoveX = (physicalApe.getPosition().x < targetX) ? 1.f : -1.f;
    } else {
        intendedMoveX = 0.f;
        if (pauseTimer <= 0.f && std::rand() % 100 < 20) {
            pauseTimer = 1.0f + (std::rand() % 20) / 10.f; // Pause upon arriving
        }
    }
}

void NPCApe::applyPhysics(float dt, WorldManager* worldManager) {
    sf::FloatRect playerBounds = physicalApe.getBounds();
    sf::FloatRect platformBounds;
    bool wasGrounded = (physicalApe.getState() == ApeState::Grounded);

    if (physicalApe.getState() != ApeState::ClimbingTrunk && physicalApe.getState() != ApeState::HangingBranch && physicalApe.getState() != ApeState::ClimbingVine) physicalApe.setState(ApeState::Airborne);

    float groundHeight = worldManager->getTerrainHeight(playerBounds.left + playerBounds.width / 2.f);
    float bottomY = playerBounds.top + playerBounds.height;
    
    if (physicalApe.getVelocity().y >= 0.f && bottomY >= groundHeight) {
        physicalApe.setPosition(physicalApe.getPosition().x, groundHeight - playerBounds.height);
        physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
        physicalApe.setState(ApeState::Grounded);
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
            physicalApe.setState(ApeState::Grounded);
            isDroppingToHang = false;
        }
    }
}

void NPCApe::update(float dt, sim::ApeData* data, WorldManager* worldManager, float timeOfDay, sim::SimulationRegistry& registry) {
    if (pauseTimer > 0.f) {
        pauseTimer -= dt;
        intendedMoveX = 0.f; 
    } else {
        stateTimer -= dt;
        if (stateTimer <= 0.f) {
            determineNextAction(data, timeOfDay, registry);
            stateTimer = 0.2f + (std::rand() % 10) / 100.f; // Organic decision variance
        }
    }

    if (physicalApe.getState() == ApeState::Grounded || physicalApe.getState() == ApeState::Airborne) {
        float speed = 140.f * baseSpeedMultiplier;
        
        if (data->currentJob == sim::Job::Guard || data->currentJob == sim::Job::Scout) {
            speed = 220.f * baseSpeedMultiplier;
        }
        
        if (intendedMoveX != 0.f) physicalApe.setVelocity(intendedMoveX * speed, physicalApe.getVelocity().y);
        else physicalApe.setVelocity(0.f, physicalApe.getVelocity().y);
    }

    applyPhysics(dt, worldManager);
    physicalApe.update(dt);

    data->worldX = physicalApe.getPosition().x;
    data->worldY = physicalApe.getPosition().y;
    data->currentChunkX = static_cast<int>(std::floor(data->worldX / 2000.f));
}

void NPCApe::draw(sf::RenderTarget& target) {
    physicalApe.draw(target);
}