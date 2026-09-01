#include "entities/NPCApe.h"
#include "entities/Tree.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>

NPCApe::NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture) 
    : simId(id), physicalApe(x, y, texture, false), stateTimer(0.f), intendedMoveX(0.f), 
      isDroppingToHang(false), grabbedChunk(0), grabbedVine(-1), grabbedSeg(-1), 
      pauseTimer(0.f), workTimer(0.f), woodcutLogTimer(0.f), fontLoaded(false) {
    baseSpeedMultiplier = 1.0f;
    personalOffset = static_cast<float>(((id * 37) % 120) - 60);

    static sf::Font sharedFont;
    static bool sharedFontLoaded = false;
    static bool fontAttempted = false;

    if (!fontAttempted) {
        fontAttempted = true;
        if (sharedFont.loadFromFile("assets/fonts/PressStart2P-Regular.ttf") ||
            sharedFont.loadFromFile("assets/fonts/font.ttf") ||
            sharedFont.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
            sharedFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf") ||
            sharedFont.loadFromFile("font.ttf")) {
            sharedFontLoaded = true;
        }
    }

    if (sharedFontLoaded) {
        fontLoaded = true;
        nameText.setFont(sharedFont);
        nameText.setCharacterSize(12);
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

void NPCApe::fireAudioHook(const std::string&) {}

void NPCApe::determineNextAction(sim::ApeData* data, float timeOfDay, sim::SimulationRegistry& registry, sim::EntityID playerId) {
    if (!data) return;

    if (data->carriedType == sim::ResourceType::Food) physicalApe.setCarriedItem(1);
    else if (data->carriedType == sim::ResourceType::Wood) physicalApe.setCarriedItem(2);
    else if (data->carriedType == sim::ResourceType::Stone) physicalApe.setCarriedItem(3);
    else physicalApe.setCarriedItem(0);

    if (data->hasTravelDestination) {
        float myX = physicalApe.getPosition().x;
        float targetX = data->travelDestinationX;
        float dist = std::abs(myX - targetX);
        float stopDistance = (data->currentJob == sim::Job::Builder || data->currentJob == sim::Job::Forage) ? 35.f : 20.f;

        if (dist > stopDistance) {
            intendedMoveX = (myX < targetX) ? 1.f : -1.f;
            physicalApe.setState(ApeState::Grounded);
        } else {
            intendedMoveX = 0.f;
            if (data->currentJob == sim::Job::Builder || data->currentJob == sim::Job::Forage || data->currentJob == sim::Job::Woodcutter) {
                physicalApe.setState(ApeState::Working);
            } else {
                physicalApe.setState(ApeState::Grounded);
            }
        }
        return;
    }

    sim::VillageData* v = registry.getVillage(data->villageId);
    if (!v) return;

    if (data->currentJob == sim::Job::Sleep) {
        intendedMoveX = 0.f;
        physicalApe.setState(ApeState::Grounded);
        return;
    }

    if (data->currentJob == sim::Job::Idle || data->currentJob == sim::Job::Socialize) {
        intendedMoveX = 0.f;
        physicalApe.setState(ApeState::Grounded);
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
        nameText.setPosition(bounds.left + bounds.width / 2.f, bounds.top - 14.f);
    }

    if (!data) return;

    if (data->currentJob == sim::Job::Woodcutter && data->currentTargetNode != 0 && worldManager) {
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
                float fallDir = (myX < destX) ? 1.0f : -1.0f;
                int targetTreeId = static_cast<int>(data->currentTargetNode);

                std::vector<Tree*> nearby = worldManager->getNearbyTrees(destX, 300.f);
                Tree* targetTree = nullptr;
                float minDist = std::numeric_limits<float>::max();

                for (Tree* t : nearby) {
                    if (!t || t->getHarvestState() == TreeHarvestState::Harvested || t->getHarvestState() == TreeHarvestState::Falling) {
                        continue;
                    }
                    if (targetTreeId != 0 && t->getId() == targetTreeId) {
                        targetTree = t;
                        break;
                    }
                    float d = std::abs(t->getTrunkCenter() - destX);
                    if (d < minDist) {
                        minDist = d;
                        targetTree = t;
                    }
                }

                if (targetTree) {
                    targetTree->startFalling(fallDir);
                }

                data->currentJob = sim::Job::Idle;
                data->currentTargetNode = 0;
                data->hasTravelDestination = false;
                workTimer = 0.f;
                physicalApe.setState(ApeState::Grounded);
                return;
            }
        }
    } else {
        determineNextAction(data, timeOfDay, registry, playerId);
    }

    if (physicalApe.getState() == ApeState::Grounded || physicalApe.getState() == ApeState::Airborne || physicalApe.getState() == ApeState::Working) {
        float speed = (data->councilRole == sim::CouncilRole::WarChief) ? 170.f : 120.f;
        
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