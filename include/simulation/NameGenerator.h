#pragma once
#include <string>
#include <vector>
#include <random>

namespace sim {

class NameGenerator {
public:
    static std::string generateHistoricalParodyName(uint32_t seed) {
        static const std::vector<std::string> firstNames = {
            "Julius Simian", "Napoleon Bonape", "Ape-raham", "Genghis Kong",
            "Cleopawtra", "Alexander the Ape", "Winston Chimphill", "Joan of Bark",
            "Ragnar Lothbark", "Marcus Aurelius", "Attila the Orang", "Otto von Chimpmark",
            "Sun Tzu-panzee", "Leonardo da Chimpci", "Tutankhamonkey", "Gorillamo Marconi",
            "Charlemangrove", "Aristootle", "Plato-panzee", "Hammurape", "Hannibal Barka",
            "Scipio Africong", "Oda Nobunape", "Saladin the Swift", "Boudicca",
            "Tiberius Gracchus", "Pericles", "Montezuma", "King Arthur", "Leif Erikchimp"
        };

        static const std::vector<std::string> epithets = {
            "the Conqueror", "the Lawgiver", "the Bold", "the Wise", "the Cruel",
            "the Silverback", "the Lionheart", "the Builder", "the Ironclad", "the Silent",
            "the Restorer", "the Pious", "the Terrible", "the Navigator", "the Fair",
            "the Unbroken", "the Fierce", "the Peacemaker", "the Astute", "the Wanderer"
        };

        std::mt19937 rng(seed);
        std::uniform_int_distribution<size_t> firstDist(0, firstNames.size() - 1);
        std::uniform_int_distribution<size_t> epiDist(0, epithets.size() - 1);
        std::uniform_int_distribution<int> chanceDist(0, 100);

        std::string name = firstNames[firstDist(rng)];
        if (chanceDist(rng) < 40) {
            name += " " + epithets[epiDist(rng)];
        }
        return name;
    }
};

}