#include "simulation/RaidManager.h"
#include <cmath>
#include <algorithm>

RaidManager::RaidManager() {}

void RaidManager::init(sf::Texture& texture, const sf::Font& f) {
    apeTexture = &texture;
    font = &f;
}

void RaidManager::spawnRaid(sim::VillageID targetVId, sim::VillageID originVId,
                            const std::string& originName, const std::string& targetName,
                            float targetX, float spawnBorderX, float retreatBorderX,
                            bool againstPlayer, int squadSize) {
    if (!apeTexture) return;

    ActiveRaid raid;
    raid.targetVillageId = targetVId;
    raid.originVillageId = originVId;
    raid.originRealmName = originName;
    raid.targetVillageName = targetName;
    raid.targetX = targetX;
    raid.retreatBorderX = retreatBorderX;
    raid.isAgainstPlayer = againstPlayer;
    raid.alertActive = againstPlayer;
    raid.alertTimer = againstPlayer ? 6.0f : 0.0f;
    raid.alertMessage = "WARBAND INCURSION: " + originName + " raiders breached " + targetName + " boundary!";

    int dir = (targetX > spawnBorderX) ? 1 : -1;

    for (int i = 0; i < squadSize; ++i) {
        RaiderUnit u;
        float startX = spawnBorderX - dir * (30.f + i * 38.f);
        float startY = 480.f;

        u.ape = std::make_unique<Ape>(startX, startY, *apeTexture, false);
        u.ape->setDepthLane(sim::DepthLane::Foreground);
        u.state = RaiderState::Marching;
        u.targetX = targetX + (i % 2 == 0 ? 15.f : -15.f) * i;
        u.retreatBorderX = retreatBorderX;
        u.direction = dir;
        u.speed = 225.f + (i % 3) * 22.f;
        u.lootTimer = 2.4f + i * 0.25f;

        raid.squad.push_back(std::move(u));
    }

    activeRaids.push_back(std::move(raid));
}

void RaidManager::update(float dt, float playerX, WorldManager* worldManager, sim::SimulationRegistry& registry) {
    pulseTime += dt;
    tensionRaidTimer += dt;

    sim::EntityID playerApeId = registry.getControlledApe();
    sim::ApeData* playerApe = registry.getApe(playerApeId);
    sim::VillageData* playerVillage = (playerApe && playerApe->villageId != 0) ? registry.getVillage(playerApe->villageId) : nullptr;
    sim::KingdomData* playerKingdom = (playerApe && playerApe->currentKingdom != 0) ? registry.getKingdom(playerApe->currentKingdom) : nullptr;

    if (tensionRaidTimer >= 55.f && playerVillage && activeRaids.empty()) {
        tensionRaidTimer = 0.f;
        if (playerKingdom) {
            for (const auto& relPair : playerKingdom->relations) {
                if (relPair.second == sim::DiplomacyStatus::War) {
                    sim::KingdomData* enemyKingdom = registry.getKingdom(relPair.first);
                    if (enemyKingdom && !enemyKingdom->controlledVillages.empty()) {
                        sim::VillageData* enemyVillage = registry.getVillage(*enemyKingdom->controlledVillages.begin());
                        if (enemyVillage) {
                            float spawnX = (enemyVillage->centerX < playerVillage->centerX) ? playerVillage->borderMinX - 80.f : playerVillage->borderMaxX + 80.f;
                            float retreatX = spawnX;
                            spawnRaid(playerVillage->id, enemyVillage->id, enemyKingdom->name, playerVillage->name,
                                      playerVillage->centerX, spawnX, retreatX, true, 5);
                            break;
                        }
                    }
                }
            }
        }
    }

    for (auto it = activeRaids.begin(); it != activeRaids.end();) {
        ActiveRaid& raid = *it;

        if (raid.alertTimer > 0.f) {
            raid.alertTimer -= dt;
            if (raid.alertTimer <= 0.f) raid.alertActive = false;
        }

        bool allFinished = true;

        for (auto& r : raid.squad) {
            if (r.state == RaiderState::Escaped) continue;

            allFinished = false;

            float curX = r.ape->getPosition().x;
            float gY = worldManager ? worldManager->getTerrainHeight(curX + 24.f) : 500.f;
            r.ape->setGroundY(gY);

            if (r.state == RaiderState::Marching) {
                r.ape->setVelocity(static_cast<float>(r.direction) * r.speed, 0.f);
                r.ape->update(dt);
                r.ape->setPosition(r.ape->getPosition().x, gY - r.ape->getBounds().height);

                if (std::abs(r.ape->getPosition().x - r.targetX) < 42.f) {
                    r.state = RaiderState::Looting;
                    r.ape->setVelocity(0.f, 0.f);
                }
            } else if (r.state == RaiderState::Looting) {
                r.ape->setVelocity(0.f, 0.f);
                r.ape->update(dt);
                r.lootTimer -= dt;

                if (r.lootTimer <= 0.f) {
                    sim::VillageData* v = registry.getVillage(raid.targetVillageId);
                    if (v) {
                        int fLoot = std::min(v->food, 8);
                        int wLoot = std::min(v->wood, 5);
                        v->food -= fLoot;
                        v->wood -= wLoot;
                        r.stolenFood = fLoot;
                        r.stolenWood = wLoot;
                    }

                    r.state = RaiderState::Retreating;
                    r.direction = (r.retreatBorderX > r.ape->getPosition().x) ? 1 : -1;
                    r.speed *= 1.25f;
                    r.carryingLoot = true;
                }
            } else if (r.state == RaiderState::Retreating) {
                r.ape->setVelocity(static_cast<float>(r.direction) * r.speed, 0.f);
                r.ape->update(dt);
                r.ape->setPosition(r.ape->getPosition().x, gY - r.ape->getBounds().height);

                bool pastBorder = (r.direction == 1 && r.ape->getPosition().x >= r.retreatBorderX) ||
                                  (r.direction == -1 && r.ape->getPosition().x <= r.retreatBorderX);
                if (pastBorder) {
                    r.state = RaiderState::Escaped;

                    if (!raid.isAgainstPlayer && playerVillage && raid.originVillageId == playerVillage->id) {
                        playerVillage->food += r.stolenFood;
                        playerVillage->wood += r.stolenWood;
                    }
                }
            }
        }

        if (allFinished) {
            it = activeRaids.erase(it);
        } else {
            ++it;
        }
    }
}

void RaidManager::draw(sf::RenderTarget& rt) {
    if (!font) return;

    for (const auto& raid : activeRaids) {
        for (const auto& r : raid.squad) {
            if (r.state == RaiderState::Escaped) continue;

            r.ape->draw(rt);

            sf::Vector2f headPos = r.ape->getPosition();
            headPos.x += r.ape->getBounds().width * 0.5f;

            if (r.state == RaiderState::Looting) {
                float pulse = 0.5f + 0.5f * std::sin(pulseTime * 12.f);
                sf::RectangleShape badge(sf::Vector2f(56.f, 15.f));
                badge.setOrigin(28.f, 15.f);
                badge.setPosition(headPos.x, headPos.y - 10.f);
                badge.setFillColor(sf::Color(220, 45, 30, static_cast<sf::Uint8>(180 + pulse * 75)));
                badge.setOutlineColor(sf::Color(255, 220, 100));
                badge.setOutlineThickness(1.f);
                rt.draw(badge);

                sf::Text pTxt("PILLAGING", *font, 9);
                pTxt.setStyle(sf::Text::Bold);
                pTxt.setFillColor(sf::Color::White);
                sf::FloatRect pb = pTxt.getLocalBounds();
                pTxt.setOrigin(pb.left + pb.width * 0.5f, pb.top + pb.height * 0.5f);
                pTxt.setPosition(headPos.x, headPos.y - 17.f);
                rt.draw(pTxt);
            } else if (r.state == RaiderState::Retreating && r.carryingLoot) {
                sf::CircleShape sack(6.f);
                sack.setOrigin(6.f, 6.f);
                sack.setPosition(headPos.x, headPos.y - 12.f);
                sack.setFillColor(sf::Color(170, 125, 60));
                sack.setOutlineColor(sf::Color(255, 220, 60));
                sack.setOutlineThickness(1.f);
                rt.draw(sack);

                sf::Text lTxt("+" + std::to_string(r.stolenFood), *font, 8);
                lTxt.setStyle(sf::Text::Bold);
                lTxt.setFillColor(sf::Color(115, 245, 115));
                sf::FloatRect lb = lTxt.getLocalBounds();
                lTxt.setOrigin(lb.left + lb.width * 0.5f, lb.top + lb.height * 0.5f);
                lTxt.setPosition(headPos.x, headPos.y - 24.f);
                rt.draw(lTxt);
            } else {
                sf::Text rLabel("RAIDER", *font, 8);
                rLabel.setStyle(sf::Text::Bold);
                rLabel.setFillColor(sf::Color(245, 75, 65));
                rLabel.setOutlineColor(sf::Color::Black);
                rLabel.setOutlineThickness(1.f);
                sf::FloatRect rb = rLabel.getLocalBounds();
                rLabel.setOrigin(rb.left + rb.width * 0.5f, rb.top + rb.height * 0.5f);
                rLabel.setPosition(headPos.x, headPos.y - 8.f);
                rt.draw(rLabel);
            }
        }
    }
}

void RaidManager::drawUI(sf::RenderWindow& window, const sf::View& letterboxView) {
    if (!font) return;

    for (const auto& raid : activeRaids) {
        if (!raid.alertActive) continue;

        window.setView(letterboxView);

        float ribbonW = 540.f;
        float ribbonH = 34.f;
        float ribbonX = (1280.f - ribbonW) * 0.5f;
        float ribbonY = 18.f;

        float pulse = 0.5f + 0.5f * std::sin(pulseTime * 8.f);

        sf::RectangleShape banner(sf::Vector2f(ribbonW, ribbonH));
        banner.setPosition(ribbonX, ribbonY);
        banner.setFillColor(sf::Color(135, 22, 16, static_cast<sf::Uint8>(215 + pulse * 40)));
        banner.setOutlineColor(sf::Color(245, 205, 75));
        banner.setOutlineThickness(1.5f);
        window.draw(banner);

        sf::Text alertTxt(raid.alertMessage, *font, 12);
        alertTxt.setStyle(sf::Text::Bold);
        alertTxt.setFillColor(sf::Color(255, 245, 215));
        alertTxt.setOutlineColor(sf::Color::Black);
        alertTxt.setOutlineThickness(1.2f);
        sf::FloatRect ab = alertTxt.getLocalBounds();
        alertTxt.setOrigin(ab.left + ab.width * 0.5f, ab.top + ab.height * 0.5f);
        alertTxt.setPosition(640.f, ribbonY + ribbonH * 0.5f);
        window.draw(alertTxt);

        break;
    }
}

void RaidManager::triggerDebugRaidOnPlayer(sim::SimulationRegistry& registry, float playerX) {
    sim::EntityID playerApeId = registry.getControlledApe();
    sim::ApeData* playerApe = registry.getApe(playerApeId);
    sim::VillageData* playerVillage = playerApe ? registry.getVillage(playerApe->villageId) : nullptr;

    if (!playerVillage) return;

    float spawnX = (playerX > playerVillage->centerX) ? playerVillage->borderMinX - 80.f : playerVillage->borderMaxX + 80.f;
    float retreatX = spawnX;
    spawnRaid(playerVillage->id, 0, "Hostile Warband", playerVillage->name,
              playerVillage->centerX, spawnX, retreatX, true, 5);
}