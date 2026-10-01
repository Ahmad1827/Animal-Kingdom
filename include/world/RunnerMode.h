#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "simulation/SimulationRegistry.h"
#include "simulation/ApeData.h"
#include "world/WorldManager.h"
#include "entities/Ape.h"

enum class HighwayObstacleType {
    LogHurdle,
    RoadsideRock,
    HangingVine,
    AmberCrystal
};

struct HighwayObstacle {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float w = 50.f;
    float h = 40.f;
    HighwayObstacleType type = HighwayObstacleType::LogHurdle;
    bool collected = false;
};

class RunnerMode {
public:
    void init(const sf::Texture& apeTexture, const sf::Font& fontRef) {
        playerTex = &apeTexture;
        font = &fontRef;
        perspectiveBlend = 0.f;
        targetPerspective = 0.f;
        transitionTimer = 0.f;
        laneZ = 0.f;
        targetLaneZ = 0.f;
        jumpY = 0.f;
        jumpVelY = 0.f;
        isJumping = false;
        isSliding = false;
        slideTimer = 0.f;
        runSpeed = 440.f;
        strideAnim = 0.f;
        lootedAmber = 0;
        stumbleTimer = 0.f;
        highwayMilestoneX = 1450.f;
        initObstacles();
    }

    void initObstacles() {
        obstacles.clear();
        for (int i = 0; i < 40; ++i) {
            float obsX = highwayMilestoneX + 280.f + static_cast<float>(i) * 260.f;
            HighwayObstacle obs;
            obs.x = obsX;
            int r = i % 4;
            if (r == 0) {
                obs.type = HighwayObstacleType::LogHurdle;
                obs.z = 0.f;
                obs.y = 0.f;
                obs.w = 140.f;
                obs.h = 28.f;
            } else if (r == 1) {
                obs.type = HighwayObstacleType::RoadsideRock;
                obs.z = (i % 2 == 0) ? -75.f : 75.f;
                obs.y = 0.f;
                obs.w = 55.f;
                obs.h = 45.f;
            } else if (r == 2) {
                obs.type = HighwayObstacleType::HangingVine;
                obs.z = 0.f;
                obs.y = 42.f;
                obs.w = 180.f;
                obs.h = 32.f;
            } else {
                obs.type = HighwayObstacleType::AmberCrystal;
                obs.z = (i % 2 == 0) ? -60.f : 60.f;
                obs.y = 10.f;
                obs.w = 26.f;
                obs.h = 26.f;
            }
            obstacles.push_back(obs);
        }
    }

    void togglePerspective(float currentWorldX) {
        if (targetPerspective < 0.5f) {
            targetPerspective = 1.0f;
            laneZ = 0.f;
            targetLaneZ = 0.f;
            jumpY = 0.f;
            jumpVelY = 0.f;
            isJumping = false;
            isSliding = false;
        } else {
            targetPerspective = 0.0f;
        }
    }

    bool isPerspectiveActive() const {
        return (perspectiveBlend > 0.001f || targetPerspective > 0.5f);
    }

    bool isNearJunction(float playerWorldX) const {
        return std::abs(playerWorldX - highwayMilestoneX) < 140.f;
    }

    float getHighwayMilestoneX() const {
        return highwayMilestoneX;
    }

    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::KeyPressed) {
            if ((event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) && !isJumping && !isSliding) {
                isJumping = true;
                jumpVelY = 520.f;
            }
            if ((event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) && !isJumping && !isSliding) {
                isSliding = true;
                slideTimer = 0.55f;
            }
        }
    }

    void update(float dt, sim::SimulationRegistry& reg, sim::EntityID playerApeId, WorldManager* wm, float timeOfDay, Ape* playerWrapper) {
        if (perspectiveBlend < targetPerspective) {
            perspectiveBlend = std::min(targetPerspective, perspectiveBlend + dt * 1.35f);
        } else if (perspectiveBlend > targetPerspective) {
            perspectiveBlend = std::max(targetPerspective, perspectiveBlend - dt * 1.35f);
        }

        if (perspectiveBlend <= 0.001f && targetPerspective <= 0.001f) {
            return;
        }

        sim::ApeData* ape = reg.getApe(playerApeId);
        if (!ape) return;

        if (stumbleTimer > 0.f) {
            stumbleTimer = std::max(0.f, stumbleTimer - dt);
        }

        float forwardPush = 0.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || perspectiveBlend > 0.85f) {
            forwardPush = 1.0f;
        }

        float currentSpeed = (stumbleTimer > 0.f) ? (runSpeed * 0.35f) : runSpeed;
        float actualSpeed = forwardPush * currentSpeed;

        ape->worldX += actualSpeed * dt;
        if (playerWrapper) {
            float gY = wm ? wm->getTerrainHeight(ape->worldX) : ape->worldY;
            playerWrapper->setPosition(ape->worldX, gY - 60.f);
        }

        strideAnim += actualSpeed * dt * 0.035f;

        float steer = 0.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) steer -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) steer += 1.f;

        targetLaneZ += steer * 340.f * dt;
        targetLaneZ = std::clamp(targetLaneZ, -110.f, 110.f);
        laneZ += (targetLaneZ - laneZ) * std::min(1.0f, dt * 14.f);

        if (isJumping) {
            jumpY += jumpVelY * dt;
            jumpVelY -= 1350.f * dt;
            if (jumpY <= 0.f) {
                jumpY = 0.f;
                jumpVelY = 0.f;
                isJumping = false;
            }
        }

        if (isSliding) {
            slideTimer -= dt;
            if (slideTimer <= 0.f) isSliding = false;
        }

        for (auto& obs : obstacles) {
            if (obs.collected) continue;
            float dx = std::abs(obs.x - ape->worldX);
            if (dx < 25.f) {
                float dz = std::abs(obs.z - laneZ);
                if (obs.type == HighwayObstacleType::AmberCrystal) {
                    if (dz < 35.f && jumpY < 35.f) {
                        obs.collected = true;
                        lootedAmber += 5;
                        ape->amberCount = std::min(99999, ape->amberCount + 5);
                    }
                } else if (obs.type == HighwayObstacleType::LogHurdle) {
                    if (dz < obs.w * 0.45f && jumpY < 26.f) {
                        stumbleTimer = 0.5f;
                    }
                } else if (obs.type == HighwayObstacleType::HangingVine) {
                    if (dz < obs.w * 0.45f && !isSliding) {
                        stumbleTimer = 0.5f;
                    }
                } else if (obs.type == HighwayObstacleType::RoadsideRock) {
                    if (dz < obs.w * 0.45f && jumpY < 22.f) {
                        stumbleTimer = 0.5f;
                    }
                }
            }
        }
    }

    void draw(sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID playerApeId, WorldManager* wm, float timeOfDay) {
        window.setView(letterboxView);

        sim::ApeData* ape = reg.getApe(playerApeId);
        float pX = ape ? ape->worldX : 1000.f;
        float pGroundY = wm ? wm->getTerrainHeight(pX) : 500.f;

        float t = perspectiveBlend * perspectiveBlend * (3.0f - 2.0f * perspectiveBlend);

        float yaw = t * (3.14159265f * 0.5f);
        float pitch = -t * 0.14f;

        float distHoriz = (1.0f - t) * 640.f + t * 240.f;
        float camEyeX = pX - t * 230.f;
        float camEyeY = pGroundY - ((1.0f - t) * 30.f + t * 90.f);
        float camEyeZ = -((1.0f - t) * 640.f) + t * laneZ;

        sf::Vector3f forward(std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch));
        sf::Vector3f right(std::cos(yaw), 0.f, -std::sin(yaw));
        sf::Vector3f up(
            right.y * forward.z - right.z * forward.y,
            right.z * forward.x - right.x * forward.z,
            right.x * forward.y - right.y * forward.x
        );

        auto projectPoint = [&](float wx, float wy, float wz, sf::Vector2f& outScreen, float& outScale) -> bool {
            sf::Vector3f d(wx - camEyeX, wy - camEyeY, wz - camEyeZ);
            float zCam = d.x * forward.x + d.y * forward.y + d.z * forward.z;
            if (zCam <= 1.0f) return false;
            float xCam = d.x * right.x + d.y * right.y + d.z * right.z;
            float yCam = d.x * up.x + d.y * up.y + d.z * up.z;
            outScale = 640.f / zCam;
            outScreen.x = 640.f + xCam * outScale;
            outScreen.y = 360.f - yCam * outScale;
            return true;
        };

        float horizonY = 360.f - (-std::sin(pitch) * 640.f);

        sf::Vertex skyQuads[] = {
            sf::Vertex(sf::Vector2f(0.f, 0.f), sf::Color(32, 24, 40)),
            sf::Vertex(sf::Vector2f(1280.f, 0.f), sf::Color(32, 24, 40)),
            sf::Vertex(sf::Vector2f(1280.f, horizonY), sf::Color(195, 135, 75)),
            sf::Vertex(sf::Vector2f(0.f, horizonY), sf::Color(195, 135, 75))
        };
        window.draw(skyQuads, 4, sf::Quads);

        sf::RectangleShape earthBack(sf::Vector2f(1280.f, 720.f - horizonY));
        earthBack.setPosition(0.f, horizonY);
        earthBack.setFillColor(sf::Color(44, 56, 30));
        window.draw(earthBack);

        float roadHalfW = 125.f;
        int stepCount = 55;
        float stepLen = 32.f;
        float startX = pX - 90.f;

        for (int i = stepCount - 1; i >= 0; --i) {
            float x1 = startX + static_cast<float>(i) * stepLen;
            float x2 = startX + static_cast<float>(i + 1) * stepLen;
            float y1 = wm ? wm->getTerrainHeight(x1) : 500.f;
            float y2 = wm ? wm->getTerrainHeight(x2) : 500.f;

            sf::Vector2f sL1, sR1, sL2, sR2, sOutL1, sOutR1, sOutL2, sOutR2;
            float sc1, sc2;

            if (!projectPoint(x1, y1, -roadHalfW, sL1, sc1) ||
                !projectPoint(x1, y1, roadHalfW, sR1, sc1) ||
                !projectPoint(x2, y2, -roadHalfW, sL2, sc2) ||
                !projectPoint(x2, y2, roadHalfW, sR2, sc2)) {
                continue;
            }

            projectPoint(x1, y1, -roadHalfW * 3.f, sOutL1, sc1);
            projectPoint(x1, y1, roadHalfW * 3.f, sOutR1, sc1);
            projectPoint(x2, y2, -roadHalfW * 3.f, sOutL2, sc2);
            projectPoint(x2, y2, roadHalfW * 3.f, sOutR2, sc2);

            bool isStripe = ((i / 2) % 2 == 0);
            sf::Color grassC = isStripe ? sf::Color(52, 70, 32) : sf::Color(42, 58, 26);
            sf::Color roadC = isStripe ? sf::Color(140, 112, 78) : sf::Color(128, 102, 70);
            sf::Color curbC = isStripe ? sf::Color(215, 190, 130) : sf::Color(165, 45, 35);
            sf::Color lineC = isStripe ? sf::Color(220, 195, 120) : sf::Color(135, 108, 75);

            sf::Vertex gL[] = {
                sf::Vertex(sOutL1, grassC), sf::Vertex(sL1, grassC),
                sf::Vertex(sL2, grassC), sf::Vertex(sOutL2, grassC)
            };
            window.draw(gL, 4, sf::Quads);

            sf::Vertex gR[] = {
                sf::Vertex(sR1, grassC), sf::Vertex(sOutR1, grassC),
                sf::Vertex(sOutR2, grassC), sf::Vertex(sR2, grassC)
            };
            window.draw(gR, 4, sf::Quads);

            sf::Vertex rQuad[] = {
                sf::Vertex(sL1, roadC), sf::Vertex(sR1, roadC),
                sf::Vertex(sR2, roadC), sf::Vertex(sL2, roadC)
            };
            window.draw(rQuad, 4, sf::Quads);

            float bW1 = (sR1.x - sL1.x) * 0.05f;
            float bW2 = (sR2.x - sL2.x) * 0.05f;
            sf::Vertex cL[] = {
                sf::Vertex(sf::Vector2f(sL1.x - bW1, sL1.y), curbC), sf::Vertex(sL1, curbC),
                sf::Vertex(sL2, curbC), sf::Vertex(sf::Vector2f(sL2.x - bW2, sL2.y), curbC)
            };
            window.draw(cL, 4, sf::Quads);

            sf::Vertex cR[] = {
                sf::Vertex(sR1, curbC), sf::Vertex(sf::Vector2f(sR1.x + bW1, sR1.y), curbC),
                sf::Vertex(sf::Vector2f(sR2.x + bW2, sR2.y), curbC), sf::Vertex(sR2, curbC)
            };
            window.draw(cR, 4, sf::Quads);

            sf::Vector2f mid1 = (sL1 + sR1) * 0.5f;
            sf::Vector2f mid2 = (sL2 + sR2) * 0.5f;
            float mW1 = (sR1.x - sL1.x) * 0.02f;
            float mW2 = (sR2.x - sL2.x) * 0.02f;
            sf::Vertex mLine[] = {
                sf::Vertex(sf::Vector2f(mid1.x - mW1, mid1.y), lineC),
                sf::Vertex(sf::Vector2f(mid1.x + mW1, mid1.y), lineC),
                sf::Vertex(sf::Vector2f(mid2.x + mW2, mid2.y), lineC),
                sf::Vertex(sf::Vector2f(mid2.x - mW2, mid2.y), lineC)
            };
            window.draw(mLine, 4, sf::Quads);

            for (const auto& obs : obstacles) {
                if (obs.collected) continue;
                if (obs.x >= x1 && obs.x < x2) {
                    sf::Vector2f oPos;
                    float oSc;
                    float obsBaseY = y1 - obs.y;
                    if (projectPoint(obs.x, obsBaseY, obs.z, oPos, oSc)) {
                        float oW = obs.w * oSc;
                        float oH = obs.h * oSc;

                        if (obs.type == HighwayObstacleType::LogHurdle) {
                            sf::RectangleShape hurdle(sf::Vector2f(oW, oH));
                            hurdle.setOrigin(oW * 0.5f, oH);
                            hurdle.setPosition(oPos);
                            hurdle.setFillColor(sf::Color(140, 92, 48));
                            hurdle.setOutlineColor(sf::Color(45, 25, 12));
                            hurdle.setOutlineThickness(1.f);
                            window.draw(hurdle);

                            sf::RectangleShape rail(sf::Vector2f(oW + 6.f * oSc, 5.f * oSc));
                            rail.setOrigin((oW + 6.f * oSc) * 0.5f, 5.f * oSc);
                            rail.setPosition(oPos.x, oPos.y - oH + 3.f * oSc);
                            rail.setFillColor(sf::Color(185, 130, 75));
                            window.draw(rail);
                        } else if (obs.type == HighwayObstacleType::RoadsideRock) {
                            sf::CircleShape rock(oW * 0.5f);
                            rock.setOrigin(oW * 0.5f, oW * 0.5f);
                            rock.setScale(1.f, 0.85f);
                            rock.setPosition(oPos.x, oPos.y - oW * 0.4f);
                            rock.setFillColor(sf::Color(120, 120, 125));
                            rock.setOutlineColor(sf::Color(45, 45, 50));
                            rock.setOutlineThickness(1.2f);
                            window.draw(rock);
                        } else if (obs.type == HighwayObstacleType::HangingVine) {
                            sf::RectangleShape branch(sf::Vector2f(oW, oH));
                            branch.setOrigin(oW * 0.5f, oH * 0.5f);
                            branch.setPosition(oPos);
                            branch.setFillColor(sf::Color(85, 52, 28));
                            branch.setOutlineColor(sf::Color(35, 20, 10));
                            branch.setOutlineThickness(1.f);
                            window.draw(branch);

                            for (int v = 0; v < 3; ++v) {
                                sf::RectangleShape vine(sf::Vector2f(3.f * oSc, 22.f * oSc));
                                vine.setPosition(oPos.x - oW * 0.35f + v * (oW * 0.35f), oPos.y);
                                vine.setFillColor(sf::Color(65, 120, 45));
                                window.draw(vine);
                            }
                        } else if (obs.type == HighwayObstacleType::AmberCrystal) {
                            sf::ConvexShape gem(4);
                            float gS = 13.f * oSc;
                            gem.setPoint(0, sf::Vector2f(0.f, -gS));
                            gem.setPoint(1, sf::Vector2f(gS * 0.8f, 0.f));
                            gem.setPoint(2, sf::Vector2f(0.f, gS));
                            gem.setPoint(3, sf::Vector2f(-gS * 0.8f, 0.f));
                            gem.setPosition(oPos);
                            gem.setFillColor(sf::Color(255, 185, 35));
                            gem.setOutlineColor(sf::Color(120, 55, 10));
                            gem.setOutlineThickness(1.f);
                            window.draw(gem);
                        }
                    }
                }
            }
        }

        sf::Vector2f pScreenRoad;
        float pSc;
        if (projectPoint(pX, pGroundY, laneZ, pScreenRoad, pSc)) {
            float pScreenY = pScreenRoad.y - jumpY * pSc;

            float shadowW = 46.f * pSc;
            float shadowH = 15.f * pSc;
            float airFade = std::clamp(1.0f - (jumpY / 110.f), 0.25f, 1.0f);

            sf::CircleShape shadow(shadowW * 0.5f);
            shadow.setScale(1.f, shadowH / shadowW);
            shadow.setOrigin(shadowW * 0.5f, shadowW * 0.5f);
            shadow.setPosition(pScreenRoad.x, pScreenRoad.y);
            shadow.setFillColor(sf::Color(12, 10, 8, static_cast<sf::Uint8>(145 * airFade)));
            window.draw(shadow);

            float bBob = std::sin(strideAnim * 6.f) * (3.f * pSc);
            float bodyW = 40.f * pSc;
            float bodyH = (isSliding ? 24.f : 54.f) * pSc;

            sf::RectangleShape torso(sf::Vector2f(bodyW, bodyH));
            torso.setOrigin(bodyW * 0.5f, bodyH);
            torso.setPosition(pScreenRoad.x, pScreenY + bBob);
            torso.setFillColor((stumbleTimer > 0.f) ? sf::Color(225, 75, 65) : sf::Color(115, 78, 48));
            torso.setOutlineColor(sf::Color(35, 20, 12));
            torso.setOutlineThickness(1.5f);
            window.draw(torso);

            sf::CircleShape head(bodyW * 0.36f);
            head.setOrigin(bodyW * 0.36f, bodyW * 0.36f);
            head.setPosition(pScreenRoad.x, pScreenY - bodyH + bBob);
            head.setFillColor((stumbleTimer > 0.f) ? sf::Color(245, 95, 85) : sf::Color(95, 62, 38));
            head.setOutlineColor(sf::Color(35, 20, 12));
            head.setOutlineThickness(1.2f);
            window.draw(head);

            bool isKing = (ape && ape->currentKingdom != 0);
            if (isKing) {
                sf::RectangleShape cloak(sf::Vector2f(bodyW * 0.72f, bodyH * 0.55f));
                cloak.setOrigin(bodyW * 0.36f, 0.f);
                cloak.setPosition(pScreenRoad.x, pScreenY - bodyH + 8.f * pSc + bBob);
                cloak.setFillColor(sf::Color(165, 45, 35));
                cloak.setOutlineColor(sf::Color(65, 18, 14));
                cloak.setOutlineThickness(0.8f);
                window.draw(cloak);

                sf::ConvexShape crown(5);
                float cW = bodyW * 0.28f;
                crown.setPoint(0, sf::Vector2f(-cW, 0.f));
                crown.setPoint(1, sf::Vector2f(-cW, -6.f * pSc));
                crown.setPoint(2, sf::Vector2f(0.f, -3.f * pSc));
                crown.setPoint(3, sf::Vector2f(cW, -6.f * pSc));
                crown.setPoint(4, sf::Vector2f(cW, 0.f));
                crown.setPosition(pScreenRoad.x, pScreenY - bodyH - bodyW * 0.35f + bBob);
                crown.setFillColor(sf::Color(245, 195, 55));
                crown.setOutlineColor(sf::Color(45, 25, 8));
                crown.setOutlineThickness(0.8f);
                window.draw(crown);
            }
        }

        drawHUD(window, pX);
    }

    void drawMilestoneIn2D(sf::RenderTarget& target, const sf::Font& f, float pX) {
        float mX = highwayMilestoneX;
        sf::RectangleShape post(sf::Vector2f(14.f, 65.f));
        post.setOrigin(7.f, 65.f);
        post.setPosition(mX, 500.f);
        post.setFillColor(sf::Color(145, 140, 130));
        post.setOutlineColor(sf::Color(45, 40, 35));
        post.setOutlineThickness(1.2f);
        target.draw(post);

        sf::ConvexShape cap(3);
        cap.setPoint(0, sf::Vector2f(-9.f, 0.f));
        cap.setPoint(1, sf::Vector2f(0.f, -12.f));
        cap.setPoint(2, sf::Vector2f(9.f, 0.f));
        cap.setPosition(mX, 500.f - 65.f);
        cap.setFillColor(sf::Color(185, 180, 170));
        cap.setOutlineColor(sf::Color(45, 40, 35));
        cap.setOutlineThickness(1.f);
        target.draw(cap);

        sf::RectangleShape banner(sf::Vector2f(100.f, 22.f));
        banner.setOrigin(50.f, 11.f);
        banner.setPosition(mX, 500.f - 42.f);
        banner.setFillColor(sf::Color(26, 18, 12, 235));
        banner.setOutlineColor(sf::Color(235, 195, 75));
        banner.setOutlineThickness(1.f);
        target.draw(banner);

        sf::Text bTxt("ROYAL ROAD", f, 9);
        bTxt.setStyle(sf::Text::Bold);
        bTxt.setFillColor(sf::Color(255, 230, 145));
        sf::FloatRect btb = bTxt.getLocalBounds();
        bTxt.setOrigin(btb.left + btb.width * 0.5f, btb.top + btb.height * 0.5f);
        bTxt.setPosition(mX, 500.f - 42.f);
        target.draw(bTxt);

        if (isNearJunction(pX)) {
            sf::RectangleShape promptBox(sf::Vector2f(270.f, 24.f));
            promptBox.setOrigin(135.f, 12.f);
            promptBox.setPosition(mX, 500.f - 85.f);
            promptBox.setFillColor(sf::Color(18, 12, 9, 245));
            promptBox.setOutlineColor(sf::Color(245, 215, 65));
            promptBox.setOutlineThickness(1.2f);
            target.draw(promptBox);

            sf::Text pTxt("[W / Up] Embark on Royal Highway  |  [V] Pivot", f, 9);
            pTxt.setStyle(sf::Text::Bold);
            pTxt.setFillColor(sf::Color(255, 240, 190));
            sf::FloatRect ptb = pTxt.getLocalBounds();
            pTxt.setOrigin(ptb.left + ptb.width * 0.5f, ptb.top + ptb.height * 0.5f);
            pTxt.setPosition(mX, 500.f - 85.f);
            target.draw(pTxt);
        }
    }

private:
    const sf::Texture* playerTex = nullptr;
    const sf::Font* font = nullptr;

    float perspectiveBlend = 0.f;
    float targetPerspective = 0.f;
    float transitionTimer = 0.f;

    float highwayMilestoneX = 1450.f;
    float laneZ = 0.f;
    float targetLaneZ = 0.f;
    float jumpY = 0.f;
    float jumpVelY = 0.f;
    bool isJumping = false;
    bool isSliding = false;
    float slideTimer = 0.f;
    float runSpeed = 440.f;
    float strideAnim = 0.f;
    int lootedAmber = 0;
    float stumbleTimer = 0.f;

    std::vector<HighwayObstacle> obstacles;

    void drawHUD(sf::RenderWindow& window, float currentWorldX) {
        if (!font) return;

        sf::RectangleShape banner(sf::Vector2f(1280.f, 36.f));
        banner.setPosition(0.f, 0.f);
        banner.setFillColor(sf::Color(18, 12, 10, 235));
        banner.setOutlineColor(sf::Color(165, 115, 55));
        banner.setOutlineThickness(1.f);
        window.draw(banner);

        sf::Text title("ROYAL HIGHWAY (2.5D BEHIND-THE-BACK PERSPECTIVE)", *font, 11);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(245, 215, 135));
        title.setPosition(20.f, 9.f);
        window.draw(title);

        std::string posStr = "Highway Mile: " + std::to_string(static_cast<int>(currentWorldX)) + "m";
        sf::Text posTxt(posStr, *font, 10);
        posTxt.setStyle(sf::Text::Bold);
        posTxt.setFillColor(sf::Color(235, 230, 210));
        posTxt.setPosition(540.f, 10.f);
        window.draw(posTxt);

        std::string ambStr = "Looted Amber: +" + std::to_string(lootedAmber);
        sf::Text ambTxt(ambStr, *font, 10);
        ambTxt.setStyle(sf::Text::Bold);
        ambTxt.setFillColor(sf::Color(255, 205, 55));
        ambTxt.setPosition(750.f, 10.f);
        window.draw(ambTxt);

        sf::Text exitTxt("[V / F4 / S] Return to 2D Side View  |  [A/D] Steer  |  [W/Space] Leap  |  [S] Duck", *font, 9);
        exitTxt.setFillColor(sf::Color(200, 175, 145));
        sf::FloatRect etb = exitTxt.getLocalBounds();
        exitTxt.setPosition(1280.f - etb.width - 20.f, 11.f);
        window.draw(exitTxt);
    }
};