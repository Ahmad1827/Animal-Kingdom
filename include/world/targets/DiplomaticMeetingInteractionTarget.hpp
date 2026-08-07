#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include <string>
#include <vector>
#include <cmath>

enum class MeetingState {
    Idle,
    WaitingForRepresentative,
    MeetingActive
};

class DiplomaticMeetingInteractionTarget : public InteractionTarget {
private:
    sim::EntityID entityA_Id;
    bool isKingdomA;
    sim::EntityID entityB_Id;
    bool isKingdomB;
    
    float worldX;
    float worldY;
    sim::SimulationRegistry& registry;
    sim::EntityID playerApeId;

    MeetingState state;
    sim::EntityID summonedRepresentativeId;

public:
    DiplomaticMeetingInteractionTarget(sim::EntityID idA, bool kA, sim::EntityID idB, bool kB, float x, float y, sim::SimulationRegistry& reg, sim::EntityID pId)
        : entityA_Id(idA), isKingdomA(kA), entityB_Id(idB), isKingdomB(kB), worldX(x), worldY(y), registry(reg), playerApeId(pId), state(MeetingState::Idle), summonedRepresentativeId(0) {}

    std::string getInteractionType() const override { return "Diplomacy"; }
    
    sf::Vector2f getInteractionPosition() const override { 
        return sf::Vector2f(worldX, worldY - 40.f); 
    }
    
    bool canInteract() const override { 
        sim::ApeData* player = registry.getApe(playerApeId);
        if (!player) return false;
        
        bool belongsToA = (isKingdomA && player->currentKingdom == entityA_Id) || (!isKingdomA && player->villageId == entityA_Id && player->currentKingdom == 0);
        bool belongsToB = (isKingdomB && player->currentKingdom == entityB_Id) || (!isKingdomB && player->villageId == entityB_Id && player->currentKingdom == 0);
        
        return belongsToA || belongsToB;
    }

    std::string getInteractionTitle() const override {
        return "Diplomatic Meeting";
    }

    int getPriority() const override { return 30; }
    void onInteract() override {}
    
    void onClose() override {
        // If the player walks away or presses ESC, release the representative
        if (state != MeetingState::Idle && summonedRepresentativeId != 0) {
            sim::ApeData* rep = registry.getApe(summonedRepresentativeId);
            if (rep) {
                rep->hasTravelDestination = true;
                rep->travelDestinationX = rep->homeX; // Send them physically walking back home
            }
        }
        state = MeetingState::Idle;
    }

    void requestAudience(sim::EntityID repId) {
        state = MeetingState::WaitingForRepresentative;
        summonedRepresentativeId = repId;
        
        sim::ApeData* rep = registry.getApe(repId);
        if (rep) {
            rep->hasTravelDestination = true;
            // Stand 50 units away from the center fire
            rep->travelDestinationX = (rep->worldX < worldX) ? worldX - 50.f : worldX + 50.f; 
        }
    }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::ApeData* player = registry.getApe(playerApeId);
        if (!player) return entries;

        bool belongsToA = (isKingdomA && player->currentKingdom == entityA_Id) || (!isKingdomA && player->villageId == entityA_Id && player->currentKingdom == 0);
        
        sim::EntityID myId = belongsToA ? entityA_Id : entityB_Id;
        bool myIsKingdom = belongsToA ? isKingdomA : isKingdomB;

        sim::EntityID targetId = belongsToA ? entityB_Id : entityA_Id;
        bool targetIsKingdom = belongsToA ? isKingdomB : isKingdomA;

        std::string targetName = "Unknown";
        sim::EntityID targetRepId = 0;
        
        if (targetIsKingdom) {
            sim::KingdomData* k = registry.getKingdom(targetId);
            if (k) {
                targetName = "Kingdom of " + k->name;
                targetRepId = k->currentKingId;
            }
        } else {
            sim::VillageData* v = registry.getVillage(targetId);
            if (v) {
                targetName = "Village of " + v->name;
                if (!v->members.empty()) {
                    targetRepId = v->members[0];
                }
            }
        }

        // --- STATE: IDLE (Initial Arrival) ---
        if (state == MeetingState::Idle) {
            entries.push_back({targetName, nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"[ Request Audience ]", [this, targetRepId]() { 
                this->requestAudience(targetRepId); 
            }});
            entries.push_back({"[ Leave ]", nullptr});
            return entries;
        }

        // --- STATE: WAITING FOR PHYSICAL ARRIVAL ---
        if (state == MeetingState::WaitingForRepresentative) {
            sim::ApeData* rep = registry.getApe(summonedRepresentativeId);
            if (rep) {
                float dist = std::abs(rep->worldX - worldX);
                // PHYSICAL ARRIVAL DETECTION
                if (dist <= 150.0f) {
                    state = MeetingState::MeetingActive;
                    return buildInteractionMenu(); // Immediately refresh UI to conversation
                }
            }
            
            entries.push_back({"Requesting an audience...", nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"Awaiting the representative", nullptr});
            entries.push_back({"from " + targetName + "...", nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"[ Cancel & Leave ]", [this]() {
                this->onClose(); // Triggers the representative to walk back home
            }});
            return entries;
        }

        // --- STATE: MEETING ACTIVE ---
        if (state == MeetingState::MeetingActive) {
            std::string repName = "Representative";
            sim::ApeData* rep = registry.getApe(summonedRepresentativeId);
            if (rep) repName = rep->name;

            entries.push_back({targetName, nullptr});
            entries.push_back({"Representative " + repName, nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"\"Why have you summoned me?\"", nullptr});
            entries.push_back({"", nullptr});
            
            // Placeholder for next step (Diplomacy actions)
            if (myIsKingdom && targetIsKingdom) {
                sim::KingdomData* myK = registry.getKingdom(myId);
                if (myK) {
                    if (myK->relations[targetId] == sim::DiplomacyStatus::Neutral || myK->relations[targetId] == sim::DiplomacyStatus::Trade) {
                        entries.push_back({"[ Declare War ]", nullptr});
                    }
                    if (myK->relations[targetId] == sim::DiplomacyStatus::Neutral) {
                        entries.push_back({"[ Discuss Trade ]", nullptr});
                    }
                }
            }

            entries.push_back({"", nullptr});
            entries.push_back({"[ End Audience ]", [this]() {
                this->onClose(); // Ends conversation, unlocks player, sends rep home.
            }});
            return entries;
        }

        return entries;
    }
};