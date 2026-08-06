#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "core/AudioManager.h"
#include "world/ParticleSystem.h"
#include <string>
#include <vector>

class BonfireInteractionTarget : public InteractionTarget {
private:
    sim::EntityID villageId;
    sim::SimulationRegistry& registry;
    AudioManager* audioManager;
    ParticleSystem* particleSystem;
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

    std::string numberToWord(int num) const {
        std::vector<std::string> words = {"Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten"};
        if (num >= 0 && num <= 10) return words[num];
        return std::to_string(num);
    }

public:
    BonfireInteractionTarget(sim::EntityID id, sim::SimulationRegistry& reg, float x, float y, AudioManager* audio, ParticleSystem* particles)
        : villageId(id), registry(reg), audioManager(audio), particleSystem(particles), posX(x), posY(y) {}

    std::string getInteractionType() const override { return "Bonfire"; }
    sf::Vector2f getInteractionPosition() const override { return sf::Vector2f(posX, posY); }
    bool canInteract() const override { return true; }

    std::string getInteractionTitle() const override {
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return "Abandoned Bonfire";
        return v->name;
    }

    int getPriority() const override { return 15; }

    void onInteract() override {
    }

    void onClose() override {
    }

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        sim::VillageData* v = registry.getVillage(villageId);
        if (!v) return entries;

        std::string mood = getVillageMood(v);
        entries.push_back({"The village is " + mood + ".", nullptr});

        int pop = v->members.size();
        entries.push_back({numberToWord(pop) + " apes live here.", nullptr});
        entries.push_back({"", nullptr});

        if (v->food >= pop * 2) entries.push_back({"Food stores are plentiful.", nullptr});
        else if (v->food < pop) entries.push_back({"Food reserves are critically low.", nullptr});
        else entries.push_back({"Food supplies are stable.", nullptr});

        if (v->wood >= 50) entries.push_back({"Wood supplies are abundant.", nullptr});
        else entries.push_back({"Wood supplies are stable.", nullptr});

        if (!v->constructionQueue.empty()) {
            entries.push_back({"Construction is progressing well.", nullptr});
        } else {
            entries.push_back({"There are no active construction projects.", nullptr});
        }

        entries.push_back({"", nullptr});
        entries.push_back({"--- RECENT EVENTS ---", nullptr});

        const auto& history = registry.getHistory();
        int eventsAdded = 0;
        
        for (auto it = history.rbegin(); it != history.rend() && eventsAdded < 5; ++it) {
            entries.push_back({it->description, nullptr});
            eventsAdded++;
        }

        if (eventsAdded == 0) {
            entries.push_back({"All is quiet.", nullptr});
        }

        return entries;
    }
};