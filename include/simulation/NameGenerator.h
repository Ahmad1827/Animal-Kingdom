#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <random>

namespace sim {

class NameGenerator {
private:
    inline static std::unordered_set<std::string> usedNames;

    inline static const std::vector<std::string> historicalBases = {
        "Julius Simian", "Napoleon Bonape", "Ape-raham Lincoln", "Genghis Kong",
        "Cleopawtra", "Alexander the Ape", "Winston Chimphill", "Joan of Bark",
        "Ragnar Lothbark", "Marcus Gorillius", "Attila the Orang", "Otto von Chimpmark",
        "Sun Tzu-panzee", "Leonardo da Chimpci", "Tutankhamonkey", "Gorillamo Marconi",
        "Charlemangrove", "Aristootle", "Plato-panzee", "Hammurape", "Hannibal Barka",
        "Scipio Africong", "Oda Nobunape", "Saladin the Swift", "Boudikong",
        "Tiberius Gracchus", "Perichimp", "Montezuma", "King Arthurbonobo", "Leif Erikchimp",
        "Erik the Redback", "Vlad the Brancher", "Mansa Monkey", "Ibn Baboon",
        "Tokugawa Ieyape", "Monkeymoto Musashi", "Monkeyangelo", "Machiape-velli",
        "King Chimpy VIII", "Queen Elizabark", "George Washingchimp", "Thomas Jeffersimian",
        "Benjamonkey Franklin", "Peter the Great Ape", "Catherine the Great Ape",
        "Louis the Sun Ape", "Lord Horatio Chimpson", "Duke of Wellingkong", "Teddy Roosevelk",
        "Charles Orangutan", "Gorilleo Galilei", "Isaac Nut-on", "Albert Einchimp",
        "Nikola Tree-sla", "Socratapes", "Spartakong", "Chimp Shi Huang", "Leonipanzee",
        "Rameses the Red", "Cyrus the Gorilla", "Daripe the Great", "Ashurbani-chimp",
        "Justinian the Jumper", "Eleanor of Apequitaine", "Alfred the Silverback",
        "Nero the Bonobo", "Caligula the Wild", "Vespasian", "Trajan the Tall",
        "Hadrian the Wallbuilder", "Antoninus Pius", "Septimius Severus", "Aurelian the Restorer",
        "Diocletian", "Constantine the Great", "Clovis the Chimp", "Theodoric the Great",
        "Charles Martel", "Pepin the Short", "Barbarossa", "Richard the Apeheart",
        "Philip Augustus", "Edward Longarms", "Robert the Bruce-chimp", "William Wall-ape",
        "Gustavus Adolphus", "Cardinal Richelieu", "Oliver Cromwell-chimp", "Sun Yat-simian"
    };

    inline static const std::vector<std::string> titles = {
        "Patriarch", "Silverback", "Elder", "High Chieftain", "Warmaster",
        "Grand Shaman", "Warlord", "High Arbiter", "Duke", "Arch-Forager",
        "Lord Protector", "Baron", "Margrave", "Consul", "Imperator",
        "High Marshall", "Dominus", "Keeper of the Canopy", "Vanguard"
    };

    inline static const std::vector<std::string> epithets = {
        "the Conqueror", "the Lawgiver", "the Bold", "the Wise", "the Cruel",
        "the Silverback", "the Lionheart", "the Builder", "the Ironclad", "the Silent",
        "the Restorer", "the Pious", "the Terrible", "the Navigator", "the Fair",
        "the Unbroken", "the Fierce", "the Peacemaker", "the Astute", "the Wanderer",
        "the Stalwart", "the Vigilant", "the Cunning", "the Fearless", "the Resolute",
        "the Unyielding", "the Far-Seer", "the Thunderer", "the Stonebreaker", "the Swift"
    };

public:
    static void reset() {
        usedNames.clear();
    }

    static std::string generateUniqueHistoricalName(uint32_t seed) {
        std::mt19937 rng(seed + static_cast<uint32_t>(usedNames.size() * 1337));
        std::uniform_int_distribution<size_t> baseDist(0, historicalBases.size() - 1);
        std::uniform_int_distribution<size_t> titleDist(0, titles.size() - 1);
        std::uniform_int_distribution<size_t> epiDist(0, epithets.size() - 1);
        std::uniform_int_distribution<int> roll(0, 100);

        for (int attempts = 0; attempts < 200; ++attempts) {
            std::string candidate = historicalBases[baseDist(rng)];
            int type = roll(rng);

            if (type < 35) {
                candidate = titles[titleDist(rng)] + " " + candidate;
            } else if (type < 70) {
                candidate = candidate + " " + epithets[epiDist(rng)];
            } else if (type < 85) {
                candidate = titles[titleDist(rng)] + " " + candidate + " " + epithets[epiDist(rng)];
            }

            if (usedNames.find(candidate) == usedNames.end()) {
                usedNames.insert(candidate);
                return candidate;
            }
        }

        int numeral = 2;
        std::string fallbackBase = historicalBases[baseDist(rng)];
        while (true) {
            std::string numberedCandidate = fallbackBase + " " + std::to_string(numeral);
            if (usedNames.find(numberedCandidate) == usedNames.end()) {
                usedNames.insert(numberedCandidate);
                return numberedCandidate;
            }
            numeral++;
        }
    }
};

}