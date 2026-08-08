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
        
        if (player->isWaitingForAudience) return false;
        
        bool belongsToA = (isKingdomA && player->currentKingdom == entityA_Id) || (!isKingdomA && player->villageId == entityA_Id && player->currentKingdom == 0);
        bool belongsToB = (isKingdomB && player->currentKingdom == entityB_Id) || (!isKingdomB && player->villageId == entityB_Id && player->currentKingdom == 0);
        return belongsToA || belongsToB;
    }

    std::string getInteractionTitle() const override {
        sim::ApeData* player = registry.getApe(playerApeId);
        if (player && player->isWaitingForAudience) {
            return "Start Audience"; // Appears only when they physically arrive
        }
        return "Request Audience";
    }

    int getPriority() const override { return 30; }

    void onInteract() override {
        sim::ApeData* player = registry.getApe(playerApeId);
        if (!player) return;

        // DIRECTLY ENTER WAITING STATE UPON PRESSING 'E'
        if (!player->isWaitingForAudience) {
            sim::EntityID repId = determineTargetRepresentative();
            if (repId != 0) {
                player->isWaitingForAudience = true;
                player->summonedRepId = repId;
                player->meetingX = worldX;
                
                sim::ApeData* rep = registry.getApe(repId);
                if (rep) {
                    rep->hasTravelDestination = true;
                    // Destination is exactly 60 pixels across the fire
                    rep->travelDestinationX = (rep->worldX < worldX) ? worldX - 60.f : worldX + 60.f; 
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
        // std::vector<InteractionMenuEntry> entries;
        // sim::ApeData* player = registry.getApe(playerApeId);
        // if (!player) return entries;

        // bool belongsToA = (isKingdomA && player->currentKingdom == entityA_Id) || (!isKingdomA && player->villageId == entityA_Id && player->currentKingdom == 0);
        // sim::EntityID targetId = belongsToA ? entityB_Id : entityA_Id;
        // bool targetIsKingdom = belongsToA ? isKingdomB : isKingdomA;

        // std::string targetName = "Unknown Entity";
        // if (targetIsKingdom) {
        //     sim::KingdomData* k = registry.getKingdom(targetId);
        //     if (k) targetName = "Kingdom of " + k->name;
        // } else {
        //     sim::VillageData* v = registry.getVillage(targetId);
        //     if (v) targetName = "Village of " + v->name;
        // }

        // // --- STATE 3: REPRESENTATIVE HAS PHYSICALLY ARRIVED ---
        // if (player->isWaitingForAudience) {
        //     sim::ApeData* rep = registry.getApe(player->summonedRepId);
            
        //     // If they are physically traveling, return an EMPTY menu.
        //     if (!rep || std::abs(rep->worldX - player->meetingX) > 150.0f) {
        //         return entries; 
        //     }

        //     // THEY HAVE ARRIVED - SHOW ACTUAL DIPLOMACY OPTIONS
        //     entries.push_back({"AUDIENCE WITH " + targetName, nullptr});
        //     entries.push_back({"Representative: " + rep->name, nullptr});
        //     entries.push_back({"Relationship: Neutral", nullptr});
        //     entries.push_back({"", nullptr});
            
        //     entries.push_back({"[ Discuss Trade ]", nullptr});
        //     entries.push_back({"[ Propose Alliance ]", nullptr});
        //     entries.push_back({"[ Demand Tribute ]", nullptr});
        //     entries.push_back({"[ Declare War ]", nullptr});
        //     entries.push_back({"", nullptr});
        //     entries.push_back({"[ End Audience ]", [player, rep]() {
        //         player->isWaitingForAudience = false;
        //         player->summonedRepId = 0;
        //         if (rep) {
        //             rep->hasTravelDestination = true;
        //             rep->travelDestinationX = rep->homeX; 
        //         }
        //     }});
        //     return entries;
        // }

        // // STATE 1 & 2: IDLE OR WAITING
        // // THE REQUEST AUDIENCE MENU HAS BEEN COMPLETELY REMOVED.
        // // Pressing [E] routes directly through onInteract() and bypasses this entirely.
        // return entries;
        return {};
    }
};