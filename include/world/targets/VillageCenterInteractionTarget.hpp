#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "core/AudioManager.h"
#include <string>
#include <vector>

class VillageCenterInteractionTarget : public InteractionTarget {
private:
    sim::EntityID villageId;
    sim::SimulationRegistry& registry;
    AudioManager* audioManager;
    float posX;
    float posY;

    enum class MenuState { Main, Roster, Construction };
    MenuState currentState;

    std::string getJobName(sim::Job job) const {
        switch(job) {
            case sim::Job::Idle: return "Idle";
            case sim::Job::Builder: return "Builder";
            case sim::Job::Woodcutter: return "Woodcutter";
            case sim::Job::StoneGatherer: return "Miner";
            case sim::Job::Forage: return "Gatherer";
            case sim::Job::Guard: return "Guard";
            case sim::Job::Scout: return "Scout";
            default: return "Worker";
        }
    }

    sim::Job getNextJob(sim::Job current) const {
        if (current == sim::Job::Idle) return sim::Job::Builder;
        if (current == sim::Job::Builder) return sim::Job::Woodcutter;
        if (current == sim::Job::Woodcutter) return sim::Job::StoneGatherer;
        if (current == sim::Job::StoneGatherer) return sim::Job::Forage;
        if (current == sim::Job::Forage) return sim::Job::Guard;
        if (current == sim::Job::Guard) return sim::Job::Scout;
        return sim::Job::Idle;
    }

public:
    VillageCenterInteractionTarget(sim::EntityID id, sim::SimulationRegistry& reg, float x, float y, AudioManager* audio)
        : villageId(id), registry(reg), audioManager(audio), posX(x), posY(y), currentState(MenuState::Main) {}

    std::string getInteractionType() const override { return "VillageCenter"; }
    sf::Vector2f getInteractionPosition() const override { return sf::Vector2f(posX, posY); }
    bool canInteract() const override { return true; }

    std::string getInteractionTitle() const override {
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return "Ruined Center";
        
        if (currentState == MenuState::Roster) return v->name + " - Citizens";
        if (currentState == MenuState::Construction) return v->name + " - Projects";
        return v->name + " - Administration";
    }

    int getPriority() const override { return 20; }

    void onInteract() override {
        currentState = MenuState::Main;
    }

    void onClose() override {
    }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return entries;

        if (currentState == MenuState::Main) {
            entries.push_back({"Population: " + std::to_string(v->members.size()), nullptr});
            entries.push_back({"Treasury: " + std::to_string(v->food) + " Food | " + std::to_string(v->wood) + " Wood | " + std::to_string(v->stone) + " Stone", nullptr});
            entries.push_back({"", nullptr});
            
            entries.push_back({"Manage Citizens", [this]() {
                currentState = MenuState::Roster;
            }});
            
            entries.push_back({"View Construction Queue", [this]() {
                currentState = MenuState::Construction;
            }});
        }
        else if (currentState == MenuState::Roster) {
            entries.push_back({"<- Return to Overview", [this]() {
                currentState = MenuState::Main;
            }});
            entries.push_back({"", nullptr});

            for (auto apeId : v->members) {
                sim::ApeData* ape = registry.getApe(apeId);
                if (ape) {
                    std::string label = ape->name + " - " + getJobName(ape->currentJob);
                    entries.push_back({label, [this, ape]() {
                        ape->currentJob = getNextJob(ape->currentJob);
                    }});
                }
            }
        }
        else if (currentState == MenuState::Construction) {
            entries.push_back({"<- Return to Overview", [this]() {
                currentState = MenuState::Main;
            }});
            entries.push_back({"", nullptr});

            if (v->constructionQueue.empty()) {
                entries.push_back({"No active construction projects.", nullptr});
            } else {
                for (auto structId : v->constructionQueue) {
                    sim::StructureData* sd = registry.getStructure(structId);
                    if (sd) {
                        int pct = 0;
                        int totReq = sd->reqWood + sd->reqStone;
                        if (totReq > 0) pct = ((sd->curWood + sd->curStone) * 100) / totReq;
                        entries.push_back({"Project: " + std::to_string(pct) + "% Complete", nullptr});
                    }
                }
            }
        }

        return entries;
    }
};