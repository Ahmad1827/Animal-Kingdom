#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include <string>
#include <vector>
#include <cmath>

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

public:
    DiplomaticMeetingInteractionTarget(sim::EntityID idA, bool kA, sim::EntityID idB, bool kB, float x, float y, sim::SimulationRegistry& reg, sim::EntityID pId)
        : entityA_Id(idA), isKingdomA(kA), entityB_Id(idB), isKingdomB(kB), worldX(x), worldY(y), registry(reg), playerApeId(pId) {}

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

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::ApeData* player = registry.getApe(playerApeId);
        if (!player) return entries;

        bool belongsToA = (isKingdomA && player->currentKingdom == entityA_Id) || (!isKingdomA && player->villageId == entityA_Id && player->currentKingdom == 0);
        
        sim::EntityID myId = belongsToA ? entityA_Id : entityB_Id;
        bool myIsKingdom = belongsToA ? isKingdomA : isKingdomB;

        sim::EntityID targetId = belongsToA ? entityB_Id : entityA_Id;
        bool targetIsKingdom = belongsToA ? isKingdomB : isKingdomA;

        std::string myName = "Unknown";
        if (myIsKingdom) {
            sim::KingdomData* k = registry.getKingdom(myId);
            if (k) myName = k->name;
        } else {
            sim::VillageData* v = registry.getVillage(myId);
            if (v) myName = v->name;
        }

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

        entries.push_back({myName + " <-> " + targetName, nullptr});
        entries.push_back({"", nullptr});

        sim::ApeData* rep = registry.getApe(targetRepId);
        bool repIsPresent = false;
        
        if (rep) {
            float dist = std::abs(rep->worldX - worldX);
            if (dist < 400.0f) {
                repIsPresent = true;
            }
        }

        if (!repIsPresent) {
            entries.push_back({"[ Request Audience ]", nullptr});
            entries.push_back({"Representative is not present.", nullptr});
        } else {
            entries.push_back({"Representative Present", nullptr});
            
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
        }
        
        entries.push_back({"", nullptr});
        entries.push_back({"[ Leave ]", nullptr});
        
        return entries;
    }
};