#include "entities/NPCApe.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

NPCApe::NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture) 
    : simId(id), physicalApe(x, y, texture, false), stateTimer(0.f), intendedMoveX(0.f), 
      isDroppingToHang(false), grabbedChunk(0), grabbedVine(-1), grabbedSeg(-1) {}

bool NPCApe::hasTrait(sim::ApeData* data, sim::Trait trait) {
    for (auto t : data->traits) {
        if (t == trait) return true;
    }
    return false;
}

void NPCApe::determineNextAction(sim::ApeData* data, float timeOfDay, sim::SimulationRegistry& registry) {
    sim::VillageData* v = registry.getVillage(data->villageId);

    // Sync visual carried item
    if (data->carriedType == sim::ResourceType::Food) physicalApe.setCarriedItem(1);
    else if (data->carriedType == sim::ResourceType::Wood) physicalApe.setCarriedItem(2);
    else if (data->carriedType == sim::ResourceType::Stone) physicalApe.setCarriedItem(3);
    else physicalApe.setCarriedItem(0);

    // 1. Returning resources to village center
    if (data->currentJob == sim::Job::CarryResource) {
        if (data->currentTargetStructure != 0) {
            sim::StructureData* s = registry.getStructure(data->currentTargetStructure);
            if (s && std::abs(physicalApe.getPosition().x - s->worldX) > 50.f) {
                intendedMoveX = (physicalApe.getPosition().x < s->worldX) ? 1.f : -1.f;
                return;
            }
        } else if (v && std::abs(physicalApe.getPosition().x - v->centerX) > 50.f) {
            intendedMoveX = (physicalApe.getPosition().x < v->centerX) ? 1.f : -1.f;
            return;
        }
        intendedMoveX = 0.f;
        return;
    }

    // 2. Resource Gathering / Building
    if (data->currentJob == sim::Job::Forage || data->currentJob == sim::Job::Woodcutter || data->currentJob == sim::Job::StoneGatherer) {
        if (data->currentTargetNode != 0) {
            sim::ResourceNode* node = registry.getResource(data->currentTargetNode);
            if (node && std::abs(physicalApe.getPosition().x - node->worldX) > 20.f) {
                intendedMoveX = (physicalApe.getPosition().x < node->worldX) ? 1.f : -1.f;
                return;
            }
        }
    } else if (data->currentJob == sim::Job::Builder) {
        if (data->currentTargetStructure != 0) {
            sim::StructureData* s = registry.getStructure(data->currentTargetStructure);
            if (s) {
                if (s->curWood < s->reqWood || s->curStone < s->reqStone) {
                    // Fetching materials from center
                    if (v && std::abs(physicalApe.getPosition().x - v->centerX) > 50.f) {
                        intendedMoveX = (physicalApe.getPosition().x < v->centerX) ? 1.f : -1.f;
                        return;
                    }
                } else {
                    // Building
                    if (std::abs(physicalApe.getPosition().x - s->worldX) > 50.f) {
                        intendedMoveX = (physicalApe.getPosition().x < s->worldX) ? 1.f : -1.f;
                        return;
                    }
                }
            }
        }
    }

    // 3. Sleeping / General wandering
    if (data->currentJob == sim::Job::Sleep || data->currentJob == sim::Job::ReturnHome) {
        if (std::abs(physicalApe.getPosition().x - data->homeX) > 50.f) intendedMoveX = (physicalApe.getPosition().x < data->homeX) ? 1.f : -1.f;
        else intendedMoveX = 0.f;
        return;
    }
    if (data->currentJob == sim::Job::Guard && v) {
        if (std::abs(physicalApe.getPosition().x - v->centerX) > 300.f) intendedMoveX = (physicalApe.getPosition().x < v->centerX) ? 1.f : -1.f;
        else { if (std::rand() % 100 < 5) intendedMoveX = (std::rand() % 2 == 0) ? 1.f : -1.f; else intendedMoveX = 0.f; }
        return;
    }
    if ((data->currentJob == sim::Job::Socialize || data->currentJob == sim::Job::Wander) && v) {
        if (std::abs(physicalApe.getPosition().x - v->centerX) > v->territoryRadius * 0.5f) intendedMoveX = (physicalApe.getPosition().x < v->centerX) ? 1.f : -1.f;
        else { if (std::rand() % 100 < 10) intendedMoveX = (std::rand() % 2 == 0) ? 1.f : -1.f; else intendedMoveX = 0.f; }
        return;
    }
    intendedMoveX = 0.f;
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
    stateTimer -= dt;
    if (stateTimer <= 0.f) {
        determineNextAction(data, timeOfDay, registry);
        stateTimer = 0.2f; // Faster AI updates for construction logic
    }

    if (physicalApe.getState() == ApeState::Grounded || physicalApe.getState() == ApeState::Airborne) {
        float speed = 150.f;
        if (data->currentJob == sim::Job::Guard || data->currentJob == sim::Job::Forage || data->currentJob == sim::Job::Woodcutter || data->currentJob == sim::Job::StoneGatherer) speed = 250.f;
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