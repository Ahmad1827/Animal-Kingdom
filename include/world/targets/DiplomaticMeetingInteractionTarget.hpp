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
    void onClose() override {}

    void requestAudience(sim::EntityID repId) {
        state = MeetingState::WaitingForRepresentative;
        summonedRepresentativeId = repId;
        // Step 4 integration: Here we will inject the movement hook to the NPCManager.
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

        // --- STATE: IDLE ---
        if (state == MeetingState::Idle) {
            entries.push_back({"Target: " + targetName, nullptr});
            entries.push_back({"", nullptr});
            
            // InteractionMenuEntry expects a std::string and a std::function<void()>
            entries.push_back({"[ Request Audience ]", [this, targetRepId]() { 
                this->requestAudience(targetRepId); 
            }});
            
            entries.push_back({"[ Leave ]", nullptr});
            return entries;
        }

        // --- STATE: WAITING FOR ARRIVAL ---
        if (state == MeetingState::WaitingForRepresentative) {
            // Check if representative has physically arrived
            sim::ApeData* rep = registry.getApe(summonedRepresentativeId);
            if (rep) {
                float dist = std::abs(rep->worldX - worldX);
                if (dist < 150.0f) {
                    state = MeetingState::MeetingActive;
                    // Force rebuild of menu immediately upon state change
                    return buildInteractionMenu();
                }
            }
            
            entries.push_back({"Requesting an audience...", nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"Awaiting the representative", nullptr});
            entries.push_back({"from " + targetName + "...", nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"[ Cancel & Leave ]", [this]() {
                this->state = MeetingState::Idle;
            }});
            return entries;
        }

        // --- STATE: MEETING ACTIVE (Step 6/7 Placeholder) ---
        if (state == MeetingState::MeetingActive) {
            std::string repName = "Representative";
            sim::ApeData* rep = registry.getApe(summonedRepresentativeId);
            if (rep) repName = rep->name;

            entries.push_back({targetName, nullptr});
            entries.push_back({repName, nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"\"Why have you summoned me?\"", nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"[ End Audience ]", [this]() {
                this->state = MeetingState::Idle;
            }});
            return entries;
        }

        return entries;
    }
};