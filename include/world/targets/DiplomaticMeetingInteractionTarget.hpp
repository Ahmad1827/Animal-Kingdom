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

    sim::EntityID determineTargetRepresentative() const {
        sim::ApeData* player = registry.getApe(playerApeId);
        if (!player) return 0;
        
        bool belongsToA = (isKingdomA && player->currentKingdom == entityA_Id) || (!isKingdomA && player->villageId == entityA_Id && player->currentKingdom == 0);
        sim::EntityID targetId = belongsToA ? entityB_Id : entityA_Id;
        bool targetIsKingdom = belongsToA ? isKingdomB : isKingdomA;

        if (targetIsKingdom) {
            sim::KingdomData* k = registry.getKingdom(targetId);
            return k ? k->currentKingId : 0;
        } else {
            sim::VillageData* v = registry.getVillage(targetId);
            return (v && !v->members.empty()) ? v->members[0] : 0;
        }
    }

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
        
        // Hide the [E] prompt and auto-close the wooden menu during the physical waiting phase
        if (player->isWaitingForAudience) {
            sim::ApeData* rep = registry.getApe(player->summonedRepId);
            if (!rep || std::abs(rep->worldX - player->meetingX) > 150.f) {
                return false; 
            }
        }
        
        bool belongsToA = (isKingdomA && player->currentKingdom == entityA_Id) || (!isKingdomA && player->villageId == entityA_Id && player->currentKingdom == 0);
        bool belongsToB = (isKingdomB && player->currentKingdom == entityB_Id) || (!isKingdomB && player->villageId == entityB_Id && player->currentKingdom == 0);
        
        return belongsToA || belongsToB;
    }

    std::string getInteractionTitle() const override {
        if (state == MeetingState::Idle) return "Request Audience";
        if (state == MeetingState::WaitingForRepresentative) return "Awaiting Arrival...";
        return "Diplomatic Meeting";
    }

    int getPriority() const override { return 30; }

    void onInteract() override {
        // As soon as the player presses [E], instantly bypass the menu and summon the representative.
        if (state == MeetingState::Idle) {
            sim::EntityID repId = determineTargetRepresentative();
            if (repId != 0) {
                state = MeetingState::WaitingForRepresentative;
                summonedRepresentativeId = repId;
                
                sim::ApeData* rep = registry.getApe(repId);
                if (rep) {
                    rep->hasTravelDestination = true;
                    rep->travelDestinationX = (rep->worldX < worldX) ? worldX - 50.f : worldX + 50.f; 
                }
            }
        }
    }
    
    void onClose() override {
        if (state != MeetingState::Idle && summonedRepresentativeId != 0) {
            sim::ApeData* rep = registry.getApe(summonedRepresentativeId);
            if (rep) {
                rep->hasTravelDestination = true;
                rep->travelDestinationX = rep->homeX; // Send them physically walking back home
            }
        }
        state = MeetingState::Idle;
    }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::ApeData* player = registry.getApe(playerApeId);
        if (!player) return entries;

        bool belongsToA = (isKingdomA && player->currentKingdom == entityA_Id) || (!isKingdomA && player->villageId == entityA_Id && player->currentKingdom == 0);
        sim::EntityID targetId = belongsToA ? entityB_Id : entityA_Id;
        bool targetIsKingdom = belongsToA ? isKingdomB : isKingdomA;

        std::string targetName = "Unknown Entity";
        sim::EntityID targetRepId = 0;
        
        if (targetIsKingdom) {
            sim::KingdomData* k = registry.getKingdom(targetId);
            if (k) { targetName = "Kingdom of " + k->name; targetRepId = k->currentKingId; }
        } else {
            sim::VillageData* v = registry.getVillage(targetId);
            if (v) { 
                targetName = "Village of " + v->name; 
                if (!v->members.empty()) targetRepId = v->members[0]; 
            }
        }

        // --- STATE: REPRESENTATIVE HAS ARRIVED ---
        if (player->isWaitingForAudience) {
            sim::ApeData* rep = registry.getApe(player->summonedRepId);
            if (rep && std::abs(rep->worldX - player->meetingX) <= 150.f) {
                entries.push_back({targetName, nullptr});
                entries.push_back({"Representative " + rep->name, nullptr});
                entries.push_back({"", nullptr});
                entries.push_back({"\"Why have you summoned me?\"", nullptr});
                entries.push_back({"", nullptr});
                entries.push_back({"[ Propose Peace ]", nullptr});
                entries.push_back({"[ Discuss Trade ]", nullptr});
                entries.push_back({"[ Declare War ]", nullptr});
                entries.push_back({"", nullptr});
                entries.push_back({"[ End Audience ]", [player]() {
                    // Release the representative back to autonomous AI
                    player->isWaitingForAudience = false;
                    player->summonedRepId = 0;
                }});
                return entries;
            }
        }

        // --- STATE: IDLE / INITIAL APPROACH ---
        entries.push_back({"Target: " + targetName, nullptr});
        entries.push_back({"", nullptr});
        
        if (targetRepId != 0) {
            entries.push_back({"[ Request Audience ]", [this, player, targetRepId]() {
                // Requesting triggers the cinematic state and forces canInteract() to false, closing the menu.
                player->isWaitingForAudience = true;
                player->summonedRepId = targetRepId;
                player->meetingX = this->worldX;
            }});
        } else {
            entries.push_back({"No representative available.", nullptr});
        }
        
        entries.push_back({"[ Leave ]", nullptr});
        return entries;
    }
};