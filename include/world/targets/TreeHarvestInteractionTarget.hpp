#pragma once
#include "interaction/InteractionTarget.h"
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include "entities/Tree.h"
#include <string>
#include <vector>

class TreeHarvestInteractionTarget : public InteractionTarget {
private:
    Tree* targetTree;
    sim::VillageID playerVillageId;
    sim::SimulationRegistry& registry;
    WorldManager* worldManager;

public:
    TreeHarvestInteractionTarget(Tree* tree, sim::VillageID vId, sim::SimulationRegistry& reg, WorldManager* wm)
        : targetTree(tree), playerVillageId(vId), registry(reg), worldManager(wm) {}

    std::string getInteractionType() const override { return "TreeHarvest"; }
    
    sf::Vector2f getInteractionPosition() const override {
        if (!targetTree) return sf::Vector2f(0.f, 0.f);
        return sf::Vector2f(targetTree->getTrunkCenter(), 480.f);
    }

    bool canInteract() const override {
        return targetTree != nullptr && targetTree->getHarvestState() != TreeHarvestState::Harvested;
    }

    std::string getInteractionTitle() const override {
        if (!targetTree) return "Wild Timber";
        
        sim::VillageData* v = registry.getVillage(playerVillageId);
        if (v) {
            float tx = targetTree->getTrunkCenter();
            if (tx < v->borderMinX || tx > v->borderMaxX) {
                return "Wild Forest (Outside Territory)";
            }
        }

        if (targetTree->getHarvestState() == TreeHarvestState::Targeted || targetTree->getHarvestState() == TreeHarvestState::BeingHarvested) {
            return "Timber Order: Marked for Logging";
        }
        return "Forest Timber: Harvestable Wood";
    }

    int getPriority() const override { return 10; }

    void onInteract() override {
        if (!targetTree) return;

        sim::VillageData* v = registry.getVillage(playerVillageId);
        if (!v) return;

        float tx = targetTree->getTrunkCenter();
        if (tx < v->borderMinX || tx > v->borderMaxX) return;

        // Cancel order if already targeted
        if (targetTree->getHarvestState() == TreeHarvestState::Targeted || targetTree->getHarvestState() == TreeHarvestState::BeingHarvested) {
            uint64_t workerId = targetTree->getAssignedWorkerId();
            if (workerId != 0) {
                sim::ApeData* worker = registry.getApe(workerId);
                if (worker) {
                    worker->hasTravelDestination = false;
                    worker->currentTargetNode = 0;
                    worker->currentJob = sim::Job::Idle;
                }
            }
            targetTree->resetHarvest();
            return;
        }

        // Find available clan worker
        sim::EntityID chosenWorkerId = 0;
        for (sim::EntityID mId : v->members) {
            if (mId == v->leaderId) continue;
            sim::ApeData* ape = registry.getApe(mId);
            if (ape && ape->alive && (ape->currentJob == sim::Job::Idle || ape->currentJob == sim::Job::Wander || ape->currentJob == sim::Job::Socialize)) {
                chosenWorkerId = mId;
                break;
            }
        }

        if (chosenWorkerId != 0) {
            sim::ApeData* worker = registry.getApe(chosenWorkerId);
            worker->currentJob = sim::Job::Woodcutter;
            worker->hasTravelDestination = true;
            worker->travelDestinationX = targetTree->getTrunkCenter();
            worker->currentTargetNode = static_cast<sim::EntityID>(targetTree->getId());

            targetTree->setHarvestState(TreeHarvestState::Targeted);
            targetTree->setAssignedWorkerId(chosenWorkerId);
        }
    }

    void onClose() override {}

    std::vector<InteractionMenuEntry> buildInteractionMenu() override {
        std::vector<InteractionMenuEntry> entries;
        if (!targetTree) return entries;

        sim::VillageData* v = registry.getVillage(playerVillageId);
        if (!v) return entries;

        float tx = targetTree->getTrunkCenter();
        bool inTerritory = (tx >= v->borderMinX && tx <= v->borderMaxX);

        if (!inTerritory) {
            entries.push_back({"This forest lies beyond your clan's claimed borders.", nullptr});
            entries.push_back({"Expand tribal territory to harvest these trees.", nullptr});
            return entries;
        }

        if (targetTree->getHarvestState() == TreeHarvestState::Targeted || targetTree->getHarvestState() == TreeHarvestState::BeingHarvested) {
            entries.push_back({"Status: Workers Dispatched for Logging", nullptr});
            entries.push_back({"Timber Yield: +15 Raw Wood on Completion", nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"[ Cancel Harvest Order ]", [this]() {
                onInteract();
            }});
        } else {
            entries.push_back({"Rich hard-timber tree suitable for harvesting.", nullptr});
            entries.push_back({"Estimated Yield: +15 Wood to Clan Stores", nullptr});
            entries.push_back({"", nullptr});
            entries.push_back({"[ Order Clan Workers to Chop ]", [this]() {
                onInteract();
            }});
        }

        return entries;
    }
};