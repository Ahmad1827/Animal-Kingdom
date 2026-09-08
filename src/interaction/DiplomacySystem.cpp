#include "interaction/DiplomacySystem.h"
#include "simulation/WarfareManager.h"
#include <algorithm>
#include <cstdlib>

DiplomacySystem::DiplomacySystem()
    : font(nullptr), isDialogueActive(false), currentDialogueRepId(0),
      dialogueSelectedIndex(0), currentDialogueNode(0), crowdSpawnTimer(0.f) {}

void DiplomacySystem::init(const sf::Font& f) {
    font = &f;
}

void DiplomacySystem::startDialogue(sim::EntityID repId, sim::SimulationRegistry& reg, sim::EntityID playerApeId, int startNode) {
    isDialogueActive = true;
    currentDialogueRepId = repId;

    sim::ApeData* player = reg.getApe(playerApeId);
    sim::ApeData* rep = reg.getApe(repId);
    if (!player || !rep) { endDialogue(reg, playerApeId); return; }

    std::string entityName = "Unknown Land";
    bool isKing = false;

    if (rep->currentKingdom != 0) {
        sim::KingdomData* rK = reg.getKingdom(rep->currentKingdom);
        if (rK) {
            entityName = "Kingdom of " + rK->name;
            if (rK->currentKingId == rep->id) isKing = true;
        }
    } else {
        sim::VillageData* v = reg.getVillage(rep->villageId);
        if (v) entityName = "Village of " + v->name;
    }

    dialogueSpeakerName = (isKing ? "King " : "Chief ") + rep->name + " of " + entityName;
    loadDialogueNode(startNode, reg, playerApeId);
}

void DiplomacySystem::endDialogue(sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    isDialogueActive = false;
    sim::ApeData* pData = reg.getApe(playerApeId);
    if (pData) {
        if (currentDialogueNode >= 600 && currentDialogueNode < 700) {
            pData->scheduledAudienceHost = 0;
        }
        if (pData->isWaitingForAudience) {
            pData->isWaitingForAudience = false;
            sim::ApeData* rep = reg.getApe(pData->summonedRepId);
            if (rep) {
                rep->hasTravelDestination = true;
                rep->travelDestinationX = rep->homeX;
            }
            pData->summonedRepId = 0;
        }
    }
}

void DiplomacySystem::update(float dt, sim::SimulationRegistry& reg, sim::EntityID playerApeId, WorldManager* worldManager, CameraManager* cameraManager) {
    sim::ApeData* pDataCheck = reg.getApe(playerApeId);

    if (isDialogueActive && currentDialogueNode >= 600 && currentDialogueNode < 700) {
        crowdSpawnTimer -= dt;
        sim::ApeData* rep = reg.getApe(currentDialogueRepId);
        int op = 0;
        sim::DiplomacyStatus status = sim::DiplomacyStatus::Neutral;

        if (rep) {
            sim::VillageData* rV = reg.getVillage(rep->villageId);
            sim::KingdomData* rK = (rep->currentKingdom != 0) ? reg.getKingdom(rep->currentKingdom) : nullptr;
            if (pDataCheck) {
                if (rK && pDataCheck->currentKingdom != 0) {
                    sim::KingdomData* pK = reg.getKingdom(pDataCheck->currentKingdom);
                    if (pK && pK->relations.count(rK->id)) status = pK->relations[rK->id];
                } else if (rV && rV->personalOpinions.count(pDataCheck->id)) {
                    op = rV->personalOpinions[pDataCheck->id];
                }
            }
        }

        bool isFriendly = (status == sim::DiplomacyStatus::Friendly || status == sim::DiplomacyStatus::Alliance || op >= 30);
        bool isHostile = (status == sim::DiplomacyStatus::War || status == sim::DiplomacyStatus::Rival || op <= -30);

        if ((isFriendly || isHostile) && crowdSpawnTimer <= 0.f && pDataCheck) {
            crowdSpawnTimer = 0.8f + (std::rand() % 15) / 10.f;
            sf::Vector2f throwerPos;
            std::vector<sim::ApeData*> potentialThrowers;

            for (auto& pair : reg.getAllApes()) {
                if (rep && pair.second.villageId == rep->villageId && pair.first != rep->id && pair.second.alive) {
                    if (std::abs(pair.second.worldX - pDataCheck->worldX) < 900.f) {
                        potentialThrowers.push_back(&pair.second);
                    }
                }
            }

            if (!potentialThrowers.empty()) {
                int rIdx = std::rand() % potentialThrowers.size();
                throwerPos = sf::Vector2f(potentialThrowers[rIdx]->worldX, potentialThrowers[rIdx]->worldY - 60.f);
            } else if (cameraManager) {
                throwerPos = cameraManager->getView().getCenter();
                throwerPos.x += (std::rand() % 2 == 0 ? -400.f : 400.f);
                throwerPos.y -= 100.f;
            }

            CrowdProjectile p;
            p.pos = throwerPos;
            float distX = pDataCheck->worldX - p.pos.x;
            p.vel = sf::Vector2f(distX * 0.8f, -250.f - (std::rand() % 150));
            p.life = 2.0f;
            p.color = isFriendly ? ((std::rand() % 2 == 0) ? sf::Color(255, 100, 150) : sf::Color(255, 255, 100)) : sf::Color(220, 40, 40);
            crowdProjectiles.push_back(p);
        }
    } else {
        crowdSpawnTimer = 0.f;
    }

    for (auto it = crowdProjectiles.begin(); it != crowdProjectiles.end(); ) {
        it->vel.y += 600.f * dt;
        it->pos += it->vel * dt;
        it->life -= dt;
        float ground = worldManager ? worldManager->getTerrainHeight(it->pos.x) : 500.f;
        if (it->life <= 0.f || it->pos.y > ground) {
            it = crowdProjectiles.erase(it);
        } else {
            ++it;
        }
    }
}

bool DiplomacySystem::handleEvent(const sf::Event& event) {
    if (!isDialogueActive) return false;

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) {
            if (!dialogueOptions.empty()) {
                dialogueSelectedIndex = (dialogueSelectedIndex - 1 + dialogueOptions.size()) % dialogueOptions.size();
            }
            return true;
        } else if (event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) {
            if (!dialogueOptions.empty()) {
                dialogueSelectedIndex = (dialogueSelectedIndex + 1) % dialogueOptions.size();
            }
            return true;
        } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::E || event.key.code == sf::Keyboard::Space) {
            if (!dialogueOptions.empty() && dialogueSelectedIndex < static_cast<int>(dialogueOptions.size())) {
                dialogueOptions[dialogueSelectedIndex].action();
            }
            return true;
        }
    }
    return false;
}

void DiplomacySystem::draw(sf::RenderWindow& window) {
    if (!isDialogueActive || !font) return;

    sf::Text speakerText(dialogueSpeakerName, *font, 24);
    speakerText.setFillColor(sf::Color(255, 215, 100));
    speakerText.setOutlineColor(sf::Color::Black);
    speakerText.setOutlineThickness(2.f);
    sf::FloatRect sRect = speakerText.getLocalBounds();
    speakerText.setOrigin(sRect.left + sRect.width / 2.0f, sRect.top + sRect.height / 2.0f);
    speakerText.setPosition(640.f, 86.f);
    window.draw(speakerText);

    sf::Text bodyText(dialogueText, *font, 20);
    bodyText.setFillColor(sf::Color(255, 255, 255));
    bodyText.setOutlineColor(sf::Color::Black);
    bodyText.setOutlineThickness(2.f);
    sf::FloatRect bRect = bodyText.getLocalBounds();
    bodyText.setOrigin(bRect.left + bRect.width / 2.0f, bRect.top + bRect.height / 2.0f);
    bodyText.setPosition(640.f, 126.f);
    window.draw(bodyText);

    float optionsStartY = 196.f;
    for (size_t i = 0; i < dialogueOptions.size(); ++i) {
        sf::Text optText("", *font, 18);
        if (static_cast<int>(i) == dialogueSelectedIndex) {
            optText.setFillColor(sf::Color(255, 255, 150));
            optText.setString("> " + dialogueOptions[i].text + " <");
        } else {
            optText.setFillColor(sf::Color(180, 180, 180));
            optText.setString(dialogueOptions[i].text);
        }
        optText.setOutlineColor(sf::Color::Black);
        optText.setOutlineThickness(1.5f);
        sf::FloatRect oRect = optText.getLocalBounds();
        optText.setOrigin(oRect.left + oRect.width / 2.0f, oRect.top + oRect.height / 2.0f);
        optText.setPosition(640.f, optionsStartY + (i * 30.f));
        window.draw(optText);
    }
}

void DiplomacySystem::loadDialogueNode(int nodeId, sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    sim::ApeData* player = reg.getApe(playerApeId);
    sim::ApeData* rep = reg.getApe(currentDialogueRepId);
    if (!player || !rep) { endDialogue(reg, playerApeId); return; }

    sim::DiplomacyStatus status = sim::DiplomacyStatus::Neutral;
    float tension = 0.0f;
    sim::KingdomID pKID = player->currentKingdom;
    sim::KingdomID rKID = rep->currentKingdom;

    if (pKID != 0 && rKID != 0) {
        sim::KingdomData* pK = reg.getKingdom(pKID);
        if (pK) {
            if (pK->relations.count(rKID)) status = pK->relations[rKID];
            if (pK->borderTension.count(rKID)) tension = pK->borderTension[rKID];
        }
    }

    currentDialogueNode = nodeId;
    dialogueSelectedIndex = 0;
    dialogueOptions.clear();

    if (loadIntroNodes(nodeId, status, tension, pKID, rKID, reg, playerApeId)) return;
    if (loadDiscoveryNodes(nodeId, pKID, rKID, reg, playerApeId)) return;
    if (loadNegotiationNodes(nodeId, pKID, rKID, reg, playerApeId)) return;
    if (loadGrievanceNodes(nodeId, pKID, rKID, reg, playerApeId)) return;
    if (loadEscalationNodes(nodeId, pKID, rKID, reg, playerApeId)) return;
    if (loadVisitNodes(nodeId, pKID, rKID, reg, playerApeId)) return;
    if (loadAudienceNodes(nodeId, pKID, rKID, reg, playerApeId)) return;

    dialogueText = "\"We have nothing more to say.\"";
    dialogueOptions.push_back({"[ End Meeting ]", [this, &reg, playerApeId]() { endDialogue(reg, playerApeId); }});
}

bool DiplomacySystem::loadIntroNodes(int nodeId, sim::DiplomacyStatus status, float tension, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    if (nodeId < 0 || nodeId >= 100) return false;

    sim::ApeData* player = reg.getApe(playerApeId);
    sim::ApeData* rep = reg.getApe(currentDialogueRepId);
    if (!player || !rep) return false;

    bool isKingdomTarget = (rKID != 0);
    bool isFirstMeeting = false;
    int opinion = 0;

    sim::VillageData* vTarget = nullptr;
    sim::KingdomData* pKData = (pKID != 0) ? reg.getKingdom(pKID) : nullptr;
    sim::KingdomData* rKData = isKingdomTarget ? reg.getKingdom(rKID) : nullptr;

    if (isKingdomTarget && pKData && rKData) {
        if (pKData->knownKingdoms.find(rKID) == pKData->knownKingdoms.end()) {
            isFirstMeeting = true;
            pKData->knownKingdoms.insert(rKID);
            rKData->knownKingdoms.insert(pKID);
            pKData->relations[rKID] = sim::DiplomacyStatus::Neutral;
            rKData->relations[pKID] = sim::DiplomacyStatus::Neutral;
            reg.addHistory({reg.getYear(), reg.getDay(), pKData->name + " made formal diplomatic contact with " + rKData->name + "."});
        }
    } else if (!isKingdomTarget && rep->villageId != 0) {
        vTarget = reg.getVillage(rep->villageId);
        if (vTarget) {
            if (vTarget->personalOpinions.count(player->id) == 0) {
                isFirstMeeting = true;
                vTarget->personalOpinions[player->id] = 0;
                reg.addHistory({reg.getYear(), reg.getDay(), player->name + " made first diplomatic contact with the independent village of " + vTarget->name + "."});
            }
            opinion = vTarget->personalOpinions[player->id];
        }
    }

    switch (nodeId) {
        case 0:
            if (isKingdomTarget) {
                if (isFirstMeeting) {
                    dialogueText = "\"We do not recognize your banners. Who are you, and what is your purpose here?\"";
                    dialogueOptions.push_back({"\"We come in peace. We wish to introduce ourselves.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"We seek formal relations with your realm.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"That is our business. Stay out of our way.\"", [this, &reg, playerApeId]() { loadDialogueNode(400, reg, playerApeId); }});
                } else if (status == sim::DiplomacyStatus::War) {
                    dialogueText = "\"We are at war. Our armies are fighting as we speak.\nState your business quickly.\"";
                    if (pKID != 0) dialogueOptions.push_back({"\"We should discuss terms of peace.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"[ Leave ]", [this, &reg, playerApeId]() { endDialogue(reg, playerApeId); }});
                    return true;
                } else if (tension > 50.0f || status == sim::DiplomacyStatus::Rival) {
                    dialogueText = "\"Your kingdom has pushed us too far lately.\nYou have nerve calling this meeting. What do you want?\"";
                    if (pKID != 0) {
                        dialogueOptions.push_back({"\"We must resolve these tensions before they worsen.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                        dialogueOptions.push_back({"\"Your arrogance will be your downfall.\"", [this, &reg, playerApeId]() { loadDialogueNode(400, reg, playerApeId); }});
                    }
                } else if (tension > 20.0f) {
                    dialogueText = "\"Our borders have been uneasy lately.\nWhy have you called us here?\"";
                    if (pKID != 0) dialogueOptions.push_back({"\"There are grievances we must address.\"", [this, &reg, playerApeId]() { loadDialogueNode(300, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"We simply wish to understand our neighbors.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                } else {
                    dialogueText = "\"We have heard good things about your people.\nWelcome to the meeting grounds. What brings you here?\"";
                    dialogueOptions.push_back({"\"We come in peace. We wish to learn about your kingdom.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                    if (pKID != 0) dialogueOptions.push_back({"\"We want to ensure our future relations remain strong.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                }
            } else {
                if (isFirstMeeting) {
                    dialogueText = "\"You have come to our borders, traveler. State your purpose.\"";
                    dialogueOptions.push_back({"\"We come in peace. We wish to learn about your people.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"We seek to establish formal relations.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                } else if (opinion >= 30) {
                    dialogueText = "\"You return! It is good to see a friend again. What news do you bring?\"";
                    dialogueOptions.push_back({"\"I have questions about your realm.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"Let us speak of our continued friendship.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                } else if (opinion <= -30) {
                    dialogueText = "\"You again. You have nerve returning here after what happened.\nSpeak quickly, or leave.\"";
                    dialogueOptions.push_back({"\"I wish to apologize and mend our relations.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"Watch your tone. I have demands.\"", [this, &reg, playerApeId]() { loadDialogueNode(300, reg, playerApeId); }});
                } else if (opinion <= -10) {
                    dialogueText = "\"You have returned. I hope you come with better intentions this time.\"";
                    dialogueOptions.push_back({"\"I wish to clear the air between us.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"We have issues to discuss.\"", [this, &reg, playerApeId]() { loadDialogueNode(300, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"I only wish to ask some questions.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                } else {
                    dialogueText = "\"We meet again. What brings you back to our fire?\"";
                    dialogueOptions.push_back({"\"I have more questions about your people.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"I wish to discuss our relations.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"There are concerns I must raise.\"", [this, &reg, playerApeId]() { loadDialogueNode(300, reg, playerApeId); }});
                }
            }
            dialogueOptions.push_back({"[ End Meeting ]", [this, &reg, playerApeId]() { endDialogue(reg, playerApeId); }});
            break;

        case 10:
            if (isKingdomTarget) {
                dialogueText = "\"What else would you discuss?\"";
                dialogueOptions.push_back({"\"I have questions about your realm.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                if (pKID != 0) {
                    dialogueOptions.push_back({"\"Let us speak of peace and relations.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"We have concerns regarding your actions.\"", [this, &reg, playerApeId]() { loadDialogueNode(300, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"I wish to discuss travel and access to your lands.\"", [this, &reg, playerApeId]() { loadDialogueNode(500, reg, playerApeId); }});
                }
            } else {
                if (opinion >= 30) {
                    dialogueText = "\"Always happy to listen. What else is on your mind?\"";
                    dialogueOptions.push_back({"\"Tell me more about your people.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"Let us discuss our cooperation.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                } else if (opinion <= -30) {
                    dialogueText = "\"I have little patience left. What is it?\"";
                    dialogueOptions.push_back({"\"I want to make amends.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"Do not test me. We have demands.\"", [this, &reg, playerApeId]() { loadDialogueNode(300, reg, playerApeId); }});
                } else if (opinion <= -10) {
                    dialogueText = "\"What else do you want from us?\"";
                    dialogueOptions.push_back({"\"Let's discuss our differences.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"I must raise some concerns.\"", [this, &reg, playerApeId]() { loadDialogueNode(300, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"I have a few questions.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                } else {
                    dialogueText = "\"What else would you discuss?\"";
                    dialogueOptions.push_back({"\"I have questions about your realm.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"Let us speak of peace and relations.\"", [this, &reg, playerApeId]() { loadDialogueNode(200, reg, playerApeId); }});
                    dialogueOptions.push_back({"\"We have concerns regarding your actions.\"", [this, &reg, playerApeId]() { loadDialogueNode(300, reg, playerApeId); }});
                }
            }
            dialogueOptions.push_back({"\"I wish to discuss travel and access to your lands.\"", [this, &reg, playerApeId]() { loadDialogueNode(500, reg, playerApeId); }});
            dialogueOptions.push_back({"\"That is all for now. Farewell.\"", [this, &reg, playerApeId]() { endDialogue(reg, playerApeId); }});
            break;
    }
    return true;
}

bool DiplomacySystem::loadDiscoveryNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    if (nodeId < 100 || nodeId >= 200) return false;

    sim::ApeData* player = reg.getApe(playerApeId);
    sim::ApeData* rep = reg.getApe(currentDialogueRepId);
    if (!player || !rep) return false;

    sim::KingdomData* rK = (rep->currentKingdom != 0) ? reg.getKingdom(rep->currentKingdom) : nullptr;
    sim::VillageData* rV = (rep->villageId != 0) ? reg.getVillage(rep->villageId) : nullptr;

    sim::DiplomacyStatus status = sim::DiplomacyStatus::Neutral;
    float tension = 0.0f;
    if (pKID != 0 && rKID != 0) {
        sim::KingdomData* pK = reg.getKingdom(pKID);
        if (pK && pK->relations.count(rKID)) status = pK->relations[rKID];
        if (pK && pK->borderTension.count(rKID)) tension = pK->borderTension[rKID];
    }

    int opinion = (rKID == 0 && rV && rV->personalOpinions.count(player->id)) ? rV->personalOpinions[player->id] : 0;
    bool isGuarded = (status == sim::DiplomacyStatus::War || status == sim::DiplomacyStatus::Rival || tension >= 40.0f || (rKID == 0 && opinion <= -10));

    switch (nodeId) {
        case 100:
            dialogueText = isGuarded ? "\"I have no reason to share our secrets with you. Ask quickly, or leave.\""
                                     : "\"There is little reason for hostility between us. Ask what you will.\"";
            dialogueOptions.push_back({"\"Who rules your people?\"", [this, &reg, playerApeId]() { loadDialogueNode(101, reg, playerApeId); }});
            dialogueOptions.push_back({"\"How strong are your people?\"", [this, &reg, playerApeId]() { loadDialogueNode(102, reg, playerApeId); }});
            dialogueOptions.push_back({"\"I have other matters to discuss.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            break;
        case 101:
            if (isGuarded) {
                dialogueText = "\"That is not your concern. Focus on your own lands.\"";
            } else if (rK) {
                sim::ApeData* king = reg.getApe(rK->currentKingId);
                std::string kName = king ? king->name : "an unknown ruler";
                dialogueText = "\"King " + kName + " guides our people. We follow his decrees.\"";
            } else if (rV) {
                sim::ApeData* chief = reg.getApe(rV->leaderId);
                std::string cName = chief ? chief->name : "the village elders";
                dialogueText = "\"We govern ourselves, guided by Chief " + cName + ".\"";
            } else {
                dialogueText = "\"We are wanderers. We govern ourselves.\"";
            }
            dialogueOptions.push_back({"\"I have another question.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
            dialogueOptions.push_back({"\"Let us change the subject.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            break;
        case 102:
            if (isGuarded) {
                dialogueText = "\"We are strong enough to defend ourselves against threats. Do not test us.\"";
            } else if (rK) {
                dialogueText = "\"We control " + std::to_string(rK->controlledVillages.size()) + " villages and our military strength is " + std::to_string(rK->militaryStrength) + ".\"";
            } else if (rV) {
                dialogueText = "\"We are an independent village of " + std::to_string(rV->members.size()) + " apes.\nWe survive on our own resources and strength.\"";
            } else {
                dialogueText = "\"We are a modest band, surviving the jungle day by day.\"";
            }
            dialogueOptions.push_back({"\"I see. I have other questions.\"", [this, &reg, playerApeId]() { loadDialogueNode(100, reg, playerApeId); }});
            dialogueOptions.push_back({"\"Let us change the subject.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            break;
    }
    return true;
}

bool DiplomacySystem::loadNegotiationNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    if (nodeId < 200 || nodeId >= 300) return false;

    auto applyConsequence = [this, &reg, playerApeId](sim::KingdomID p, sim::KingdomID r, float tensionAmount, int opinionAmount, const std::string& hist) {
        sim::ApeData* player = reg.getApe(playerApeId);
        sim::ApeData* rep = reg.getApe(currentDialogueRepId);
        if (!player || !rep) return;

        if (p != 0 && r != 0) {
            sim::KingdomData* pkData = reg.getKingdom(p);
            sim::KingdomData* rkData = reg.getKingdom(r);
            if (pkData && rkData) {
                pkData->borderTension[r] = std::max(0.0f, pkData->borderTension[r] + tensionAmount);
                rkData->borderTension[p] = std::max(0.0f, rkData->borderTension[p] + tensionAmount);
                if (pkData->relations[r] != sim::DiplomacyStatus::War) {
                    if (pkData->borderTension[r] >= 50.0f && pkData->relations[r] != sim::DiplomacyStatus::Rival) {
                        pkData->relations[r] = sim::DiplomacyStatus::Rival;
                        rkData->relations[p] = sim::DiplomacyStatus::Rival;
                    } else if (pkData->borderTension[r] < 40.0f && pkData->relations[r] == sim::DiplomacyStatus::Rival) {
                        pkData->relations[r] = sim::DiplomacyStatus::Neutral;
                        rkData->relations[p] = sim::DiplomacyStatus::Neutral;
                        reg.addHistory({reg.getYear(), reg.getDay(), pkData->name + " and " + rkData->name + " successfully eased their rivalry."});
                    }
                }
                if (!hist.empty()) reg.addHistory({reg.getYear(), reg.getDay(), hist});
            }
        } else if (rep->villageId != 0) {
            sim::VillageData* vTarget = reg.getVillage(rep->villageId);
            if (vTarget) {
                vTarget->personalOpinions[player->id] += opinionAmount;
                int op = vTarget->personalOpinions[player->id];
                sim::VillageID pVidKey = (player->currentKingdom != 0 && reg.getKingdom(player->currentKingdom)) ? reg.getKingdom(player->currentKingdom)->capitalVillageId : player->villageId;
                if (pVidKey != 0) {
                    if (op >= 30) vTarget->relations[pVidKey] = sim::Reputation::Friendly;
                    else if (op <= -30) vTarget->relations[pVidKey] = sim::Reputation::Hostile;
                    else if (op <= -10) vTarget->relations[pVidKey] = sim::Reputation::Suspicious;
                    else vTarget->relations[pVidKey] = sim::Reputation::Neutral;
                }
            }
        }
    };

    switch (nodeId) {
        case 200:
            dialogueText = "\"Words of peace are easily spoken. What are you offering?\"";
            dialogueOptions.push_back({"\"I apologize for any past friction between us.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                sim::KingdomData* pK = reg.getKingdom(pKID);
                std::string pName = pK ? pK->name : "A realm";
                applyConsequence(pKID, rKID, -15.0f, 15, pName + " apologized, easing diplomatic tension.");
                loadDialogueNode(201, reg, playerApeId);
            }});
            dialogueOptions.push_back({"\"We simply want to assure you of our good intentions.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                applyConsequence(pKID, rKID, -5.0f, 5, "");
                loadDialogueNode(202, reg, playerApeId);
            }});
            dialogueOptions.push_back({"\"Nevermind. Let us return to other topics.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            break;
        case 201:
            dialogueText = "\"It takes strength to admit fault. We will ease our guard.\nLet us hope this peace lasts.\"";
            dialogueOptions.push_back({"\"Thank you. Let us speak of other things.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            break;
        case 202:
            dialogueText = "\"Intentions are proven through actions, not meetings.\nBut we will keep our minds open.\"";
            dialogueOptions.push_back({"\"We will prove it. Let us change the subject.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            break;
    }
    return true;
}

bool DiplomacySystem::loadGrievanceNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    if (nodeId < 300 || nodeId >= 400) return false;

    auto applyConsequence = [this, &reg, playerApeId](sim::KingdomID p, sim::KingdomID r, float tensionAmount, int opinionAmount, const std::string& hist) {
        sim::ApeData* player = reg.getApe(playerApeId);
        sim::ApeData* rep = reg.getApe(currentDialogueRepId);
        if (!player || !rep) return;

        if (p != 0 && r != 0) {
            sim::KingdomData* pkData = reg.getKingdom(p);
            sim::KingdomData* rkData = reg.getKingdom(r);
            if (pkData && rkData) {
                pkData->borderTension[r] = std::max(0.0f, pkData->borderTension[r] + tensionAmount);
                rkData->borderTension[p] = std::max(0.0f, rkData->borderTension[p] + tensionAmount);
                if (pkData->relations[r] != sim::DiplomacyStatus::War) {
                    if (pkData->borderTension[r] >= 50.0f && pkData->relations[r] != sim::DiplomacyStatus::Rival) {
                        pkData->relations[r] = sim::DiplomacyStatus::Rival;
                        rkData->relations[p] = sim::DiplomacyStatus::Rival;
                    } else if (pkData->borderTension[r] < 40.0f && pkData->relations[r] == sim::DiplomacyStatus::Rival) {
                        pkData->relations[r] = sim::DiplomacyStatus::Neutral;
                        rkData->relations[p] = sim::DiplomacyStatus::Neutral;
                        reg.addHistory({reg.getYear(), reg.getDay(), pkData->name + " and " + rkData->name + " successfully eased their rivalry."});
                    }
                }
                if (!hist.empty()) reg.addHistory({reg.getYear(), reg.getDay(), hist});
            }
        } else if (rep->villageId != 0) {
            sim::VillageData* vTarget = reg.getVillage(rep->villageId);
            if (vTarget) {
                vTarget->personalOpinions[player->id] += opinionAmount;
                int op = vTarget->personalOpinions[player->id];
                sim::VillageID pVidKey = (player->currentKingdom != 0 && reg.getKingdom(player->currentKingdom)) ? reg.getKingdom(player->currentKingdom)->capitalVillageId : player->villageId;
                if (pVidKey != 0) {
                    if (op >= 30) vTarget->relations[pVidKey] = sim::Reputation::Friendly;
                    else if (op <= -30) vTarget->relations[pVidKey] = sim::Reputation::Hostile;
                    else if (op <= -10) vTarget->relations[pVidKey] = sim::Reputation::Suspicious;
                    else vTarget->relations[pVidKey] = sim::Reputation::Neutral;
                }
            }
        }
    };

    switch (nodeId) {
        case 300:
            dialogueText = "\"You come to our borders and speak of concern?\nYou should explain yourself carefully.\"";
            dialogueOptions.push_back({"\"Your scouts have been encroaching on our lands.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                applyConsequence(pKID, rKID, 10.0f, -10, "");
                loadDialogueNode(301, reg, playerApeId);
            }});
            dialogueOptions.push_back({"\"Your kingdom is growing too arrogant.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                sim::KingdomData* pK = reg.getKingdom(pKID);
                sim::KingdomData* rK = reg.getKingdom(rKID);
                std::string desc = (pK && rK) ? pK->name + " insulted the realm of " + rK->name + "." : "";
                applyConsequence(pKID, rKID, 20.0f, -20, desc);
                loadDialogueNode(400, reg, playerApeId);
            }});
            dialogueOptions.push_back({"\"Perhaps I spoke in haste. Let us step back.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                applyConsequence(pKID, rKID, -5.0f, 5, "");
                loadDialogueNode(200, reg, playerApeId);
            }});
            break;
        case 301:
            dialogueText = "\"The jungle belongs to no one. Our scouts go where they please.\nDo not mistake our curiosity for weakness.\"";
            dialogueOptions.push_back({"\"Then we must formally agree on where our borders lie.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                applyConsequence(pKID, rKID, -5.0f, 5, "");
                loadDialogueNode(200, reg, playerApeId);
            }});
            dialogueOptions.push_back({"\"You will withdraw them immediately, or else.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                applyConsequence(pKID, rKID, 15.0f, -15, "");
                loadDialogueNode(400, reg, playerApeId);
            }});
            break;
    }
    return true;
}

bool DiplomacySystem::loadEscalationNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    if (nodeId < 400 || nodeId >= 500) return false;

    auto applyConsequence = [this, &reg, playerApeId](sim::KingdomID p, sim::KingdomID r, float tensionAmount, int opinionAmount, const std::string& hist) {
        sim::ApeData* player = reg.getApe(playerApeId);
        sim::ApeData* rep = reg.getApe(currentDialogueRepId);
        if (!player || !rep) return;

        if (p != 0 && r != 0) {
            sim::KingdomData* pkData = reg.getKingdom(p);
            sim::KingdomData* rkData = reg.getKingdom(r);
            if (pkData && rkData) {
                pkData->borderTension[r] = std::max(0.0f, pkData->borderTension[r] + tensionAmount);
                rkData->borderTension[p] = std::max(0.0f, rkData->borderTension[p] + tensionAmount);
                if (pkData->relations[r] != sim::DiplomacyStatus::War) {
                    if (pkData->borderTension[r] >= 50.0f && pkData->relations[r] != sim::DiplomacyStatus::Rival) {
                        pkData->relations[r] = sim::DiplomacyStatus::Rival;
                        rkData->relations[p] = sim::DiplomacyStatus::Rival;
                    } else if (pkData->borderTension[r] < 40.0f && pkData->relations[r] == sim::DiplomacyStatus::Rival) {
                        pkData->relations[r] = sim::DiplomacyStatus::Neutral;
                        rkData->relations[p] = sim::DiplomacyStatus::Neutral;
                        reg.addHistory({reg.getYear(), reg.getDay(), pkData->name + " and " + rkData->name + " successfully eased their rivalry."});
                    }
                }
                if (!hist.empty()) reg.addHistory({reg.getYear(), reg.getDay(), hist});
            }
        } else if (rep->villageId != 0) {
            sim::VillageData* vTarget = reg.getVillage(rep->villageId);
            if (vTarget) {
                vTarget->personalOpinions[player->id] += opinionAmount;
                int op = vTarget->personalOpinions[player->id];
                sim::VillageID pVidKey = (player->currentKingdom != 0 && reg.getKingdom(player->currentKingdom)) ? reg.getKingdom(player->currentKingdom)->capitalVillageId : player->villageId;
                if (pVidKey != 0) {
                    if (op >= 30) vTarget->relations[pVidKey] = sim::Reputation::Friendly;
                    else if (op <= -30) vTarget->relations[pVidKey] = sim::Reputation::Hostile;
                    else if (op <= -10) vTarget->relations[pVidKey] = sim::Reputation::Suspicious;
                    else vTarget->relations[pVidKey] = sim::Reputation::Neutral;
                }
            }
        }
    };

    switch (nodeId) {
        case 400:
            dialogueText = "\"Are you threatening us? That is a very dangerous path.\nWatch your next words carefully.\"";
            dialogueOptions.push_back({"\"I spoke in anger. Let us return to reason.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                applyConsequence(pKID, rKID, -10.0f, 15, "");
                loadDialogueNode(200, reg, playerApeId);
            }});
            dialogueOptions.push_back({"\"It is a warning. Withdraw, or face the consequences.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                sim::KingdomData* pK = reg.getKingdom(pKID);
                sim::KingdomData* rK = reg.getKingdom(rKID);
                std::string desc = (pK && rK) ? pK->name + " issued an ultimatum to " + rK->name + "." : "";
                applyConsequence(pKID, rKID, 25.0f, -25, desc);
                loadDialogueNode(401, reg, playerApeId);
            }});
            break;
        case 401:
            dialogueText = "\"We do not bow to threats. If it is blood you want, you shall have it.\nIs this what you desire?\"";
            dialogueOptions.push_back({"\"Then there is nothing left to say. It is war.\"", [this, &reg, playerApeId, pKID, rKID]() {
                sim::ApeData* rep = reg.getApe(currentDialogueRepId);
                sim::ApeData* player = reg.getApe(playerApeId);
                if (pKID != 0 && rKID != 0) {
                    sim::WarfareManager::declareWar(reg, pKID, rKID, "Diplomatic breakdown following an ultimatum.");
                } else if (pKID != 0 && rep && rep->villageId != 0 && player) {
                    sim::WarfareManager::declareRaid(reg, pKID, rep->villageId, player->id, "Diplomatic breakdown following an ultimatum.");
                }
                loadDialogueNode(402, reg, playerApeId);
            }});
            dialogueOptions.push_back({"\"Wait... let us not rush to war over this.\"", [this, &reg, playerApeId, applyConsequence, pKID, rKID]() {
                applyConsequence(pKID, rKID, -5.0f, 10, "");
                loadDialogueNode(200, reg, playerApeId);
            }});
            break;
        case 402:
            dialogueText = "\"So be it. The meeting is over.\nOur armies will meet on the field.\"";
            dialogueOptions.push_back({"[ End Meeting ]", [this, &reg, playerApeId]() { endDialogue(reg, playerApeId); }});
            break;
    }
    return true;
}

bool DiplomacySystem::loadVisitNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    if (nodeId < 500 || nodeId >= 600) return false;

    sim::ApeData* player = reg.getApe(playerApeId);
    sim::ApeData* rep = reg.getApe(currentDialogueRepId);
    if (!player || !rep) return false;

    sim::KingdomData* rKData = (rKID != 0) ? reg.getKingdom(rKID) : nullptr;
    sim::VillageData* rVData = (rep->villageId != 0) ? reg.getVillage(rep->villageId) : nullptr;

    bool hasPermission = (rKData && rKData->permittedApes.count(player->id)) || (rVData && rKID == 0 && rVData->permittedApes.count(player->id));
    bool isFriendly = false;

    if (rKData && pKID != 0) {
        sim::KingdomData* pKData = reg.getKingdom(pKID);
        if (pKData && pKData->relations.count(rKID)) {
            auto status = pKData->relations[rKID];
            if (status == sim::DiplomacyStatus::Friendly || status == sim::DiplomacyStatus::Alliance || status == sim::DiplomacyStatus::Trade) isFriendly = true;
        }
    } else if (rVData && rKID == 0) {
        int op = rVData->personalOpinions.count(player->id) ? rVData->personalOpinions[player->id] : 0;
        if (op >= 30) isFriendly = true;
    }

    switch (nodeId) {
        case 500:
            if (hasPermission) {
                dialogueText = "\"You already have permission to enter our lands. You are welcome among us.\"";
                if (player->scheduledAudienceHost == 0) {
                    dialogueOptions.push_back({"\"I would like to speak with you in your homeland.\"", [this, &reg, playerApeId]() { loadDialogueNode(510, reg, playerApeId); }});
                }
                dialogueOptions.push_back({"\"Thank you. Let us discuss other matters.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            } else {
                dialogueText = "\"You wish to enter our territory? That is not a request we grant lightly.\"";
                dialogueOptions.push_back({"\"May I visit your homeland?\"", [this, &reg, playerApeId, isFriendly]() {
                    if (isFriendly) loadDialogueNode(501, reg, playerApeId);
                    else loadDialogueNode(502, reg, playerApeId);
                }});
                dialogueOptions.push_back({"\"Nevermind. Let us return to other topics.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            }
            break;
        case 501:
            dialogueText = "\"You have proven yourself trustworthy. You may enter our lands in peace.\"";
            dialogueOptions.push_back({"\"I appreciate this honor.\"", [this, &reg, playerApeId, rKData, rVData, player]() {
                if (rKData) rKData->permittedApes.insert(player->id);
                else if (rVData) rVData->permittedApes.insert(player->id);
                reg.addHistory({reg.getYear(), reg.getDay(), player->name + " was granted permission to enter " + (rKData ? rKData->name : rVData->name) + "."});
                loadDialogueNode(500, reg, playerApeId);
            }});
            break;
        case 502:
            dialogueText = "\"We are not yet comfortable opening our borders to you. Prove your intentions first.\"";
            dialogueOptions.push_back({"\"I understand. I will earn your trust.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            dialogueOptions.push_back({"\"This is an insult!\"", [this, &reg, playerApeId]() { loadDialogueNode(400, reg, playerApeId); }});
            break;
        case 510:
            if (isFriendly) {
                dialogueText = "\"We would be honored to host you in our halls. Come to our homeland. I will receive you there.\"";
                dialogueOptions.push_back({"\"I will see you there.\"", [this, &reg, playerApeId, player, rep]() {
                    player->scheduledAudienceHost = rep->id;
                    reg.addHistory({reg.getYear(), reg.getDay(), player->name + " was granted an audience with " + rep->name + "."});
                    loadDialogueNode(10, reg, playerApeId);
                }});
            } else {
                dialogueText = "\"You may travel our lands, but I have no time for a formal audience right now.\"";
                dialogueOptions.push_back({"\"Very well.\"", [this, &reg, playerApeId]() { loadDialogueNode(10, reg, playerApeId); }});
            }
            break;
    }
    return true;
}

bool DiplomacySystem::loadAudienceNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId) {
    if (nodeId < 600 || nodeId >= 700) return false;

    sim::ApeData* player = reg.getApe(playerApeId);
    sim::ApeData* rep = reg.getApe(currentDialogueRepId);
    if (!player || !rep) return false;

    sim::KingdomData* rKData = (rKID != 0) ? reg.getKingdom(rKID) : nullptr;
    sim::VillageData* rVData = (rep->villageId != 0) ? reg.getVillage(rep->villageId) : nullptr;

    bool isFriendly = false;
    bool isHostile = false;

    if (rKData && pKID != 0) {
        sim::KingdomData* pKData = reg.getKingdom(pKID);
        if (pKData && pKData->relations.count(rKID)) {
            auto status = pKData->relations[rKID];
            if (status == sim::DiplomacyStatus::Friendly || status == sim::DiplomacyStatus::Alliance || status == sim::DiplomacyStatus::Trade) isFriendly = true;
            if (status == sim::DiplomacyStatus::War || status == sim::DiplomacyStatus::Rival) isHostile = true;
        }
    } else if (rVData && rKID == 0) {
        int op = rVData->personalOpinions.count(player->id) ? rVData->personalOpinions[player->id] : 0;
        if (op >= 30) isFriendly = true;
        if (op <= -30) isHostile = true;
    }

    switch (nodeId) {
        case 600:
            if (isHostile) {
                dialogueText = "\"You have nerve showing your face in my hall. Speak quickly before my guards remove you.\"";
            } else if (isFriendly) {
                dialogueText = "\"Welcome to our lands, friend! We honor the agreements made at the meeting grounds. What would you discuss?\"";
            } else {
                dialogueText = "\"You have made the journey. I am listening. What political matters bring you to my seat?\"";
            }
            dialogueOptions.push_back({"\"I would like to propose an alliance.\"", [this, &reg, playerApeId]() { loadDialogueNode(610, reg, playerApeId); }});
            dialogueOptions.push_back({"\"Can we strengthen trade between our peoples?\"", [this, &reg, playerApeId]() { loadDialogueNode(620, reg, playerApeId); }});
            dialogueOptions.push_back({"\"There are matters concerning our borders.\"", [this, &reg, playerApeId]() { loadDialogueNode(630, reg, playerApeId); }});
            dialogueOptions.push_back({"\"That is all. Thank you for receiving me.\"", [this, &reg, playerApeId]() { loadDialogueNode(699, reg, playerApeId); }});
            break;
        case 610:
            if (isHostile) dialogueText = "\"An alliance with you? Don't make me laugh. We are closer to war than friendship.\"";
            else if (isFriendly) dialogueText = "\"Our peoples already stand close. An alliance is the natural next step.\n(Alliance functionality to be implemented in a future phase).\"";
            else dialogueText = "\"An alliance is a heavy commitment. You have not yet earned that level of trust.\"";
            dialogueOptions.push_back({"\"Understood.\"", [this, &reg, playerApeId]() { loadDialogueNode(600, reg, playerApeId); }});
            break;
        case 620:
            dialogueText = isHostile ? "\"We do not share our resources with enemies.\""
                                     : "\"Trade benefits us all.\n(Economy & Caravan logistics to be expanded in future phase).\"";
            dialogueOptions.push_back({"\"Let us return to other matters.\"", [this, &reg, playerApeId]() { loadDialogueNode(600, reg, playerApeId); }});
            break;
        case 630:
            dialogueText = "\"If you wish to dispute borders or file grievances, you must do so at the neutral meeting ground. My hall is for internal affairs and high treaties.\"";
            dialogueOptions.push_back({"\"Understood.\"", [this, &reg, playerApeId]() { loadDialogueNode(600, reg, playerApeId); }});
            break;
        case 699:
            dialogueText = "\"Safe travels back to your own lands.\"";
            dialogueOptions.push_back({"[ End Audience ]", [this, &reg, playerApeId, player, rep, rKData, rVData]() {
                player->scheduledAudienceHost = 0;
                reg.addHistory({reg.getYear(), reg.getDay(), player->name + " completed a formal audience with " + rep->name + "."});
                endDialogue(reg, playerApeId);
            }});
            break;
    }
    return true;
}