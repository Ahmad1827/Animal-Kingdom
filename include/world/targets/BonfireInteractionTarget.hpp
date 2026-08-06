#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include <string>
#include <vector>

class BonfireInteractionTarget : public InteractionTarget {
private:
    sim::EntityID villageId;
    sim::SimulationRegistry& registry;
    float posX;
    float posY;

    std::string getVillageMood(sim::VillageData* v) const {
        if (!v) return "Abandoned";
        int pop = v->members.size();
        
        if (pop == 0) return "Abandoned";
        if (v->food < pop) return "Hungry";
        if (v->food >= pop * 2) return "Prosperous";
        if (!v->constructionQueue.empty()) return "Busy";
        
        return "Stable";
    }

    std::string getSeasonName(int season) const {
        switch(season) {
            case 0: return "Spring";
            case 1: return "Summer";
            case 2: return "Autumn";
            case 3: return "Winter";
            default: return "Unknown";
        }
    }

public:
    BonfireInteractionTarget(sim::EntityID id, sim::SimulationRegistry& reg, float x, float y)
        : villageId(id), registry(reg), posX(x), posY(y) {}

    std::string getInteractionType() const override { return "Bonfire"; }
    
    sf::Vector2f getInteractionPosition() const override { return sf::Vector2f(posX, posY); }
    
    bool canInteract() const override { return true; }
    
    std::string getInteractionTitle() const override { 
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return "Abandoned Bonfire";
        return v->name + " (" + getVillageMood(v) + ")";
    }
    
    int getPriority() const override { return 15; }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return entries;

        // 1. Weather / Time
        std::string season = getSeasonName(static_cast<int>(registry.getSeason()));
        entries.push_back({"Weather: " + season + ", Year " + std::to_string(registry.getYear()), nullptr});

        // 2. Village Status (Resources)
        int totalTools = v->toolsAxe + v->toolsPick + v->toolsSpear + v->toolsTorch + v->toolsBasket + v->toolsRope;
        entries.push_back({"Stores: " + std::to_string(v->food) + " Food | " + std::to_string(v->wood) + " Wood | " + std::to_string(v->stone) + " Stone", nullptr});

        // 3. Village Status (Population & Jobs)
        int idle = 0, build = 0, sleep = 0, work = 0, guard = 0;
        for (auto apeId : v->members) {
            sim::ApeData* ape = registry.getApe(apeId);
            if (ape) {
                if (ape->currentJob == sim::Job::Idle || ape->currentJob == sim::Job::Socialize) idle++;
                else if (ape->currentJob == sim::Job::Builder) build++;
                else if (ape->currentJob == sim::Job::Sleep) sleep++;
                else if (ape->currentJob == sim::Job::Guard) guard++;
                else work++;
            }
        }
        entries.push_back({"Citizens: " + std::to_string(v->members.size()) + " (" + std::to_string(work) + " Work, " + std::to_string(build) + " Build, " + std::to_string(idle) + " Idle)", nullptr});
        
        // 4. Construction Report
        if (!v->constructionQueue.empty()) {
            sim::StructureData* sd = registry.getStructure(v->constructionQueue.front());
            if (sd) {
                int pct = 0;
                int totReq = sd->reqWood + sd->reqStone;
                if (totReq > 0) pct = ((sd->curWood + sd->curStone) * 100) / totReq;
                entries.push_back({"Project: Under Construction (" + std::to_string(pct) + "% built by " + std::to_string(build) + " builders)", nullptr});
            }
        } else {
            entries.push_back({"Project: No active projects.", nullptr});
        }

        // 5. Kingdom News
        if (v->kingdomId != 0) {
            sim::KingdomData* kd = registry.getKingdom(v->kingdomId);
            if (kd) {
                std::string leaderName = "Unknown";
                sim::ApeData* king = registry.getApe(kd->currentKingId);
                if (king) leaderName = king->name;
                entries.push_back({"Realm: " + kd->name + " (Led by " + leaderName + ")", nullptr});
            }
        } else {
            entries.push_back({"Realm: Independent Village", nullptr});
        }

        // 6. Village News (Recent World History)
        entries.push_back({"--- RECENT RUMORS ---", nullptr});
        const auto& history = registry.getHistory();
        int rumorsAdded = 0;
        
        // Iterate backwards to get the absolute newest events
        for (auto it = history.rbegin(); it != history.rend() && rumorsAdded < 2; ++it) {
            entries.push_back({"- " + it->description, nullptr});
            rumorsAdded++;
        }
        
        if (rumorsAdded == 0) {
            entries.push_back({"- All is quiet.", nullptr});
        }

        return entries;
    }
};