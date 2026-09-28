#include "helpers.hpp"


bool parseworld(const JsonValue& json) {
    if (!std::holds_alternative<JsonObject>(json.value)) {
        std::cerr << "Root must be an object\n";
        return false;
    }
    const JsonObject& root = std::get<JsonObject>(json.value);


    //
    // Validate "world"
    //
    auto worldIt = root.find("world");
    if (worldIt == root.end()) {
        std::cerr << "Missing 'world'\n";
        return false;
    }
    if (!std::holds_alternative<JsonObject>(worldIt->second.value)) {
        std::cerr << "'world' must be an object\n";
        return false;
    }
    const JsonObject& world = std::get<JsonObject>(worldIt->second.value);
    
    


    //
    // Validate "world.items"
    //
    auto itemsIt = world.find("items");
    if (itemsIt == world.end()) {
        std::cerr << "Missing 'world.items'\n";
        return false;
    }
    if (!std::holds_alternative<JsonObject>(itemsIt->second.value)) {
        std::cerr << "'world.items' must be an object\n";
        return false;
    }
    const JsonObject& items = std::get<JsonObject>(itemsIt->second.value);

        //
        // Validate "world.items.consumables"
        //
    auto consumablesIt = items.find("consumables"); // Find consumables
    if (consumablesIt == items.end()) {
        std::cerr << "Missing 'items.consumables'\n";
        return false;
    }
    if (!std::holds_alternative<JsonArray>(consumablesIt->second.value)) {
        std::cerr << "'items.consumables' must be an array\n";
        return false;
    }
    const JsonArray& consumables = std::get<JsonArray>(consumablesIt->second.value);
    for (size_t i = 0; i < consumables.size(); ++i) {
        if (!std::holds_alternative<JsonObject>(consumables[i].value)) {
            std::cerr << "items.consumables[" << i << "] must be an object\n";
            return false;
        }
        const JsonObject& cons = std::get<JsonObject>(consumables[i].value);
        auto nameIt = cons.find("name"); // Find "name"
        if (nameIt == cons.end()) {
            std::cerr << "items.consumables[" << i << "] is missing 'name'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(nameIt->second.value)) {
            std::cerr << "items.consumables[" << i << "].name must be a string\n";
            return false;
        }
        auto descriptionIt = cons.find("description"); // Find "description"
        if (descriptionIt == cons.end()) {
            std::cerr << "items.consumables[" << i << "] is missing 'description'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(descriptionIt->second.value)) {
            std::cerr << "items.consumables[" << i << "].description must be a string\n";
            return false;
        }
        auto obtainableIt = cons.find("obtainable"); // Find "obtainable"
        if (obtainableIt == cons.end()) {
            std::cerr << "items.consumables[" << i << "] is missing 'obtainable'\n";
            return false;
        }
        if (!std::holds_alternative<bool>(obtainableIt->second.value)) {
            std::cerr << "items.consumables[" << i << "].obtainable must be a boolean\n";
            return false;
        }
        auto healIt = cons.find("heal"); // Find "heal"
        if (healIt == cons.end()) {
            std::cerr << "items.consumables[" << i << "] is missing 'heal'\n";
            return false;
        }
        if (!std::holds_alternative<double>(healIt->second.value)) {
            std::cerr << "items.consumables[" << i << "].heal must be an int\n";
            return false;
        }
        if (std::get<double>(healIt->second.value) <= 0) {
            std::cerr << "items.consumables[" << i << "].heal must be greater than 0\n";
            return false;
        }
    }


        //
        // Validate "world.items.weapons"
        //
    auto weaponsIt = items.find("weapons"); // Find weapons
    if (weaponsIt == items.end()) {
        std::cerr << "Missing 'items.weapons'\n";
        return false;
    }
    if (!std::holds_alternative<JsonArray>(weaponsIt->second.value)) {
        std::cerr << "'items.weapons' must be an array\n";
        return false;
    }
    const JsonArray& weapons = std::get<JsonArray>(weaponsIt->second.value);
    for (size_t i = 0; i < weapons.size(); ++i) {
        if (!std::holds_alternative<JsonObject>(weapons[i].value)) {
            std::cerr << "items.weapons[" << i << "] must be an object\n";
            return false;
        }
        const JsonObject& wep = std::get<JsonObject>(weapons[i].value);
        auto nameIt = wep.find("name"); // Find "name"
        if (nameIt == wep.end()) {
            std::cerr << "items.weapons[" << i << "] is missing 'name'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(nameIt->second.value)) {
            std::cerr << "items.weapons[" << i << "].name must be a string\n";
            return false;
        }
        auto descriptionIt = wep.find("description"); // Find "description"
        if (descriptionIt == wep.end()) {
            std::cerr << "items.weapons[" << i << "] is missing 'description'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(descriptionIt->second.value)) {
            std::cerr << "items.weapons[" << i << "].description must be a string\n";
            return false;
        }
        auto obtainableIt = wep.find("obtainable"); // Find "obtainable"
        if (obtainableIt == wep.end()) {
            std::cerr << "items.weapons[" << i << "] is missing 'obtainable'\n";
            return false;
        }
        if (!std::holds_alternative<bool>(obtainableIt->second.value)) {
            std::cerr << "items.weapons[" << i << "].obtainable must be a boolean\n";
            return false;
        }
        auto attackIt = wep.find("attack"); // Find "attack"
        if (attackIt == wep.end()) {
            std::cerr << "items.weapons[" << i << "] is missing 'attack'\n";
            return false;
        }
        if (!std::holds_alternative<double>(attackIt->second.value)) {
            std::cerr << "items.weapons[" << i << "].attack must be an int\n";
            return false;
        }
        if (std::get<double>(attackIt->second.value) <= 0) {
            std::cerr << "items.weapons[" << i << "].attack must be greater than 0\n";
            return false;
        }
        auto durabilityIt = wep.find("durability"); // Find "durability"
        if (durabilityIt == wep.end()) {
            std::cerr << "items.weapons[" << i << "] is missing 'durability'\n";
            return false;
        }
        if (!std::holds_alternative<double>(durabilityIt->second.value)) {
            std::cerr << "items.weapons[" << i << "].durability must be an int\n";
            return false;
        }
        if (std::get<double>(durabilityIt->second.value) <= 0) {
            std::cerr << "items.weapons[" << i << "].durability must be greater than 0\n";
            return false;
        }
    }


        //
        // Validate "world.items.armors"
        //
    auto armorsIt = items.find("armors"); // Find armors
    if (armorsIt == items.end()) {
        std::cerr << "Missing 'items.armors'\n";
        return false;
    }
    if (!std::holds_alternative<JsonArray>(armorsIt->second.value)) {
        std::cerr << "'items.armors' must be an array\n";
        return false;
    }
    const JsonArray& armors = std::get<JsonArray>(armorsIt->second.value);
    for (size_t i = 0; i < armors.size(); ++i) {
        if (!std::holds_alternative<JsonObject>(armors[i].value)) {
            std::cerr << "items.armors[" << i << "] must be an object\n";
            return false;
        }
        const JsonObject& arm = std::get<JsonObject>(armors[i].value);
        auto nameIt = arm.find("name"); // Find "name"
        if (nameIt == arm.end()) {
            std::cerr << "items.armors[" << i << "] is missing 'name'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(nameIt->second.value)) {
            std::cerr << "items.armors[" << i << "].name must be a string\n";
            return false;
        }
        auto descriptionIt = arm.find("description"); // Find "description"
        if (descriptionIt == arm.end()) {
            std::cerr << "items.armors[" << i << "] is missing 'description'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(descriptionIt->second.value)) {
            std::cerr << "items.armors[" << i << "].description must be a string\n";
            return false;
        }
        auto obtainableIt = arm.find("obtainable"); // Find "obtainable"
        if (obtainableIt == arm.end()) {
            std::cerr << "items.armors[" << i << "] is missing 'obtainable'\n";
            return false;
        }
        if (!std::holds_alternative<bool>(obtainableIt->second.value)) {
            std::cerr << "items.armors[" << i << "].obtainable must be a boolean\n";
            return false;
        }
        auto defenseIt = arm.find("defense"); // Find "defense"
        if (defenseIt == arm.end()) {
            std::cerr << "items.armors[" << i << "] is missing 'defense'\n";
            return false;
        }
        if (!std::holds_alternative<double>(defenseIt->second.value)) {
            std::cerr << "items.armors[" << i << "].defense must be an int\n";
            return false;
        }
        if (std::get<double>(defenseIt->second.value) <= 0) {
            std::cerr << "items.armors[" << i << "].defense must be greater than 0\n";
            return false;
        }
        auto durabilityIt = arm.find("durability"); // Find "durability"
        if (durabilityIt == arm.end()) {
            std::cerr << "items.armors[" << i << "] is missing 'durability'\n";
            return false;
        }
        if (!std::holds_alternative<double>(durabilityIt->second.value)) {
            std::cerr << "items.armors[" << i << "].durability must be an int\n";
            return false;
        }
        if (std::get<double>(durabilityIt->second.value) <= 0) {
            std::cerr << "items.armors[" << i << "].durability must be greater than 0\n";
            return false;
        }
    }





    //
    // Validate "world.npcs"
    //
    auto npcsIt = world.find("npcs");
    if (npcsIt == world.end()) {
        std::cerr << "Missing 'world.npcs'\n";
        return false;
    }
    if (!std::holds_alternative<JsonObject>(npcsIt->second.value)) {
        std::cerr << "'world.npcs' must be an object\n";
        return false;
    }
    const JsonObject& npcs = std::get<JsonObject>(npcsIt->second.value);


        //
        // Validate "world.npcs.bosses"
        //
    auto bossesIt = npcs.find("bosses"); // Find bosses
    if (bossesIt == npcs.end()) {
        std::cerr << "Missing 'npcs.bosses'\n";
        return false;
    }
    if (!std::holds_alternative<JsonArray>(bossesIt->second.value)) {
        std::cerr << "'npcs.bosses' must be an array\n";
        return false;
    }
    const JsonArray& bosses = std::get<JsonArray>(bossesIt->second.value);

    for (size_t i = 0; i < bosses.size(); ++i) {
        if (!std::holds_alternative<JsonObject>(bosses[i].value)) {
            std::cerr << "npcs.bosses[" << i << "] must be an object\n";
            return false;
        }
        const JsonObject& boss = std::get<JsonObject>(bosses[i].value);

        auto nameIt = boss.find("name"); // Find "name"
        if (nameIt == boss.end()) {
            std::cerr << "npcs.bosses[" << i << "] is missing 'name'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(nameIt->second.value)) {
            std::cerr << "npcs.bosses[" << i << "].name must be a string\n";
            return false;
        }

        auto descriptionIt = boss.find("description"); // Find "description"
        if (descriptionIt == boss.end()) {
            std::cerr << "npcs.bosses[" << i << "] is missing 'description'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(descriptionIt->second.value)) {
            std::cerr << "npcs.bosses[" << i << "].description must be a string\n";
            return false;
        }

        auto statsIt = boss.find("stats"); // Find "stats"
        if (statsIt == boss.end()) {
            std::cerr << "npcs.bosses[" << i << "] is missing 'stats'\n";
            return false;
        }
        if (!std::holds_alternative<JsonObject>(statsIt->second.value)) {
            std::cerr << "'stats' must be an object\n";
            return false;
        }
        const JsonObject& stats = std::get<JsonObject>(statsIt->second.value);

        auto hpIt = stats.find("hp"); // Find "hp"
        if (hpIt == stats.end()) {
            std::cerr << "npcs.bosses[" << i << "] is missing 'hp'\n";
            return false;
        }
        if (!std::holds_alternative<double>(hpIt->second.value)) {
            std::cerr << "npcs.bosses[" << i << "].hp must be an int\n";
            return false;
        }
        if (std::get<double>(hpIt->second.value) <= 0) {
            std::cerr << "npcs.bosses[" << i << "].hp must be greater than 0\n";
            return false;
        }

        auto damageIt = stats.find("damage"); // Find "damage"
        if (damageIt == stats.end()) {
            std::cerr << "npcs.bosses[" << i << "] is missing 'damage'\n";
            return false;
        }
        if (!std::holds_alternative<double>(damageIt->second.value)) {
            std::cerr << "npcs.bosses[" << i << "].damage must be an int\n";
            return false;
        }
        if (std::get<double>(damageIt->second.value) <= 0) {
            std::cerr << "npcs.bosses[" << i << "].damage must be greater than 0\n";
            return false;
        }

        auto dropsIt = boss.find("drops"); // Find "drops"
        if (dropsIt == boss.end()) {
            std::cerr << "npcs.bosses[" << i << "] is missing 'drops'\n";
            return false;
        }
        if (!std::holds_alternative<JsonObject>(dropsIt->second.value)) {
            std::cerr << "'drops' must be an object\n";
            return false;
        }
        const JsonObject& drops = std::get<JsonObject>(dropsIt->second.value);

        auto coinsIt = drops.find("coins"); // Find "coins"
        if (coinsIt == drops.end()) {
            std::cerr << "npcs.bosses[" << i << "].drops is missing 'coins'\n";
            return false;
        }
        if (!std::holds_alternative<double>(coinsIt->second.value)) {
            std::cerr << "'coins' must be an int\n";
            return false;
        }
        if (std::get<double>(coinsIt->second.value) <= 0) {
            std::cerr << "'coins' must be greater than 0\n";
            return false;
        }

        auto expIt = drops.find("exp"); // Find "exp"
        if (expIt == drops.end()) {
            std::cerr << "npcs.bosses[" << i << "].drops is missing 'exp'\n";
            return false;
        }
        if (!std::holds_alternative<double>(expIt->second.value)) {
            std::cerr << "'exp' must be an int\n";
            return false;
        }
        if (std::get<double>(expIt->second.value) <= 0) {
            std::cerr << "'exp' must be greater than 0\n";
            return false;
        }


        //
        // Validate boss drop "weapon"
        //
        auto weaponIt = drops.find("weapon");
        if (weaponIt != drops.end()) {
            if (!std::holds_alternative<std::string>(weaponIt->second.value)) {
                std::cerr << "'weapon' must be a string\n";
                return false;
            }
            const std::string& weaponName = std::get<std::string>(weaponIt->second.value);
            bool found = false;
            for (size_t j = 0; j < weapons.size(); ++j) {
                const JsonObject& weapon = std::get<JsonObject>(weapons[j].value);
                const auto itemNameIt = weapon.find("name");
                if (itemNameIt != weapon.end() && std::get<std::string>(itemNameIt->second.value) == weaponName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "npcs.bosses[" << i << "].drops.weapon '" << weaponName << "' does not exist in world.items.weapons\n";
                return false;
            }
        }


        //
        // Validate boss drop "consumable"
        //
        auto consumableIt = drops.find("consumable");
        if (consumableIt != drops.end()) {
            if (!std::holds_alternative<std::string>(consumableIt->second.value)) {
                std::cerr << "'consumable' must be a string\n";
                return false;
            }
            const std::string& consumableName = std::get<std::string>(consumableIt->second.value);
            bool found = false;
            for (size_t j = 0; j < consumables.size(); ++j) {
                const JsonObject& consumable = std::get<JsonObject>(consumables[j].value);
                const auto itemNameIt = consumable.find("name");
                if (itemNameIt != consumable.end() && std::get<std::string>(itemNameIt->second.value) == consumableName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "npcs.bosses[" << i << "].drops.consumable '" << consumableName << "' does not exist in world.items.consumables\n";
                return false;
            }
        }


        //
        // Validate boss drop "armor"
        //
        auto armorIt = drops.find("armor");
        if (armorIt != drops.end()) {
            if (!std::holds_alternative<std::string>(armorIt->second.value)) {
                std::cerr << "'armor' must be a string\n";
                return false;
            }
            const std::string& armorName = std::get<std::string>(armorIt->second.value);
            bool found = false;
            for (size_t j = 0; j < armors.size(); ++j) {
                const JsonObject& armor = std::get<JsonObject>(armors[j].value);
                const auto itemNameIt = armor.find("name");
                if (itemNameIt != armor.end() &&
                    std::get<std::string>(itemNameIt->second.value) == armorName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "npcs.bosses[" << i << "].drops.armor '" << armorName << "' does not exist in world.items.armors\n";
                return false;
            }
        }
    }


        //
        // Validate "world.npcs.traders"
        //
    auto tradersIt = npcs.find("traders"); // Find traders
    if (tradersIt == npcs.end()) {
        std::cerr << "Missing 'npcs.traders'\n";
        return false;
    }
    if (!std::holds_alternative<JsonArray>(tradersIt->second.value)) {
        std::cerr << "'npcs.traders' must be an array\n";
        return false;
    }
    const JsonArray& traders = std::get<JsonArray>(tradersIt->second.value);

    for (size_t i = 0; i < traders.size(); ++i) {
        if (!std::holds_alternative<JsonObject>(traders[i].value)) {
            std::cerr << "items.traders[" << i << "] must be an object\n";
            return false;
        }
        const JsonObject& trade = std::get<JsonObject>(traders[i].value);

        auto nameIt = trade.find("name"); // Find "name"
        if (nameIt == trade.end()) {
            std::cerr << "npcs.traders[" << i << "] is missing 'name'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(nameIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].name must be a string\n";
            return false;
        }

        auto descriptionIt = trade.find("description"); // Find "description"
        if (descriptionIt == trade.end()) {
            std::cerr << "npcs.traders[" << i << "] is missing 'description'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(descriptionIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].description must be a string\n";
            return false;
        }

        auto statsIt = trade.find("stats"); // Find "stats"
        if (statsIt == trade.end()) {
            std::cerr << "npcs.traders[" << i << "] is missing 'stats'\n";
            return false;
        }
        if (!std::holds_alternative<JsonObject>(statsIt->second.value)) {
            std::cerr << "'stats' must be an object\n";
            return false;
        }
        const JsonObject& stats = std::get<JsonObject>(statsIt->second.value);

        auto hpIt = stats.find("hp"); // Find "hp"
        if (hpIt == stats.end()) {
            std::cerr << "npcs.traders[" << i << "] is missing 'hp'\n";
            return false;
        }
        if (!std::holds_alternative<double>(hpIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].hp must be an int\n";
            return false;
        }
        if (std::get<double>(hpIt->second.value) <= 0) {
            std::cerr << "npcs.traders[" << i << "].hp must be greater than 0\n";
            return false;
        }

        auto damageIt = stats.find("damage"); // Find "damage"
        if (damageIt == stats.end()) {
            std::cerr << "npcs.traders[" << i << "] is missing 'damage'\n";
            return false;
        }
        if (!std::holds_alternative<double>(damageIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].damage must be an int\n";
            return false;
        }
        if (std::get<double>(damageIt->second.value) != 0) {
            std::cerr << "npcs.traders[" << i << "].damage must be 0\n";
            return false;
        }

        auto questIt = trade.find("quest"); // Find "quest"
        if (questIt == trade.end()) {
            std::cerr << "npcs.traders[" << i << "] is missing 'quest'\n";
            return false;
        }
        if (!std::holds_alternative<JsonObject>(questIt->second.value)) {
            std::cerr << "'quest' must be an object\n";
            return false;
        }
        const JsonObject& quest = std::get<JsonObject>(questIt->second.value);

        auto typeIt = quest.find("type"); // Find "type"
        if (typeIt == quest.end()) {
            std::cerr << "npcs.traders[" << i << "].quest is missing 'type'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(typeIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].quest.type must be a string\n";
            return false;
        }
        if (std::get<std::string>(typeIt->second.value) != "fetch" && std::get<std::string>(typeIt->second.value) != "eliminate") {
            std::cerr << "npcs.traders[" << i << "].quest must be of a valid type\n";
            return false;
        }

        auto targetIt = quest.find("target"); // Find "target"
        if (targetIt == quest.end()) {
            std::cerr << "npcs.traders[" << i << "].quest is missing 'target'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(targetIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].quest.target must be a string\n";
            return false;
        }
        std::string qtarget = std::get<std::string>(targetIt->second.value);
        if (std::get<std::string>(typeIt->second.value) == "fetch") {
            bool found = false;
            // Check consumables
            for (const auto& item : consumables) {
                const JsonObject& obj = std::get<JsonObject>(item.value);
                const auto itemNameIt = obj.find("name");

                if (itemNameIt != obj.end() && std::get<std::string>(itemNameIt->second.value) == qtarget) {
                    found = true;
                    break;
                }
            }
            // Check weapons
            if (!found) {
                for (const auto& item : weapons) {
                    const JsonObject& obj = std::get<JsonObject>(item.value);
                    const auto itemNameIt = obj.find("name");

                    if (itemNameIt != obj.end() && std::get<std::string>(itemNameIt->second.value) == qtarget) {
                        found = true;
                        break;
                    }
                }
            }
            // Check armors
            if (!found) {
                for (const auto& item : armors) {
                    const JsonObject& obj = std::get<JsonObject>(item.value);
                    const auto itemNameIt = obj.find("name");
                    if (itemNameIt != obj.end() && std::get<std::string>(itemNameIt->second.value) == qtarget) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                if (qtarget != "coins") {
                    std::cerr << "world.npcs.traders[" << i << "].quest.target references item '" << qtarget << "' which does not exist in world.items\n";
                    return false;
                }
            }
        }
        if (std::get<std::string>(typeIt->second.value) == "eliminate") {
            bool found = false;
            // Check bosses
            for (const auto& boss : bosses) {
                const JsonObject& act = std::get<JsonObject>(boss.value);
                const auto locbossIt = act.find("name");

                if (locbossIt != act.end() && std::get<std::string>(locbossIt->second.value) == qtarget) {
                    found = true;
                    break;
                }
            }
            // Check weapons
            if (!found) {
                for (const auto& trade : traders) {
                    const JsonObject& act = std::get<JsonObject>(trade.value);
                    const auto loctradeIt = act.find("name");

                    if (loctradeIt != act.end() && std::get<std::string>(loctradeIt->second.value) == qtarget) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                std::cerr << "world.npcs.traders[" << i << "].quest.target references item '" << qtarget << "' which does not exist in world.npcs\n";
                return false;
            }
        }


        auto qstatusIt = quest.find("status"); // Find "status"
        if (qstatusIt == quest.end()) {
            std::cerr << "npcs.traders[" << i << "].quest is missing 'status'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(qstatusIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].quest.status must be a string\n";
            return false;
        }

        auto countIt = quest.find("count"); // Find "count"
        if (countIt == quest.end()) {
            std::cerr << "npcs.traders[" << i << "].quest is missing 'count'\n";
            return false;
        }
        if (!std::holds_alternative<double>(countIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].quest.count must be an int\n";
            return false;
        }
        if (std::get<double>(countIt->second.value) <= 0) {
            std::cerr << "npcs.traders[" << i << "].quest must greater than 0\n";
            return false;
        }

        auto rewardsIt = trade.find("rewards"); // Find "rewards"
        if (rewardsIt == trade.end()) {
            std::cerr << "npcs.traders[" << i << "] is missing 'rewards'\n";
            return false;
        }
        if (!std::holds_alternative<JsonObject>(rewardsIt->second.value)) {
            std::cerr << "'rewards' must be an object\n";
            return false;
        }
        const JsonObject& rewards = std::get<JsonObject>(rewardsIt->second.value);


        //
        // Validate trader reward "weapon"
        //
        auto weaponIt = rewards.find("weapon");
        if (weaponIt != rewards.end()) {
            if (!std::holds_alternative<std::string>(weaponIt->second.value)) {
                std::cerr << "'weapon' must be a string\n";
                return false;
            }
            const std::string& weaponName = std::get<std::string>(weaponIt->second.value);
            bool found = false;
            for (size_t j = 0; j < weapons.size(); ++j) {
                const JsonObject& weapon = std::get<JsonObject>(weapons[j].value);
                const auto itemNameIt = weapon.find("name");
                if (itemNameIt != weapon.end() && std::get<std::string>(itemNameIt->second.value) == weaponName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "npcs.traders[" << i << "].rewards.weapon '" << weaponName << "' does not exist in world.items.weapons\n";
                return false;
            }
        }


        //
        // Validate trader reward "consumable"
        //
        auto consumableIt = rewards.find("consumable");
        if (consumableIt != rewards.end()) {
            if (!std::holds_alternative<std::string>(consumableIt->second.value)) {
                std::cerr << "'consumable' must be a string\n";
                return false;
            }
            const std::string& consumableName = std::get<std::string>(consumableIt->second.value);
            bool found = false;
            for (size_t j = 0; j < consumables.size(); ++j) {
                const JsonObject& consumable = std::get<JsonObject>(consumables[j].value);
                const auto itemNameIt = consumable.find("name");
                if (itemNameIt != consumable.end() && std::get<std::string>(itemNameIt->second.value) == consumableName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "npcs.traders[" << i << "].rewards.consumable '" << consumableName << "' does not exist in world.items.consumables\n";
                return false;
            }
        }


        //
        // Validate trader reward "armor"
        //
        auto armorIt = rewards.find("armor");
        if (armorIt != rewards.end()) {
            if (!std::holds_alternative<std::string>(armorIt->second.value)) {
                std::cerr << "'armor' must be a string\n";
                return false;
            }
            const std::string& armorName = std::get<std::string>(armorIt->second.value);
            bool found = false;
            for (size_t j = 0; j < armors.size(); ++j) {
                const JsonObject& armor = std::get<JsonObject>(armors[j].value);
                const auto itemNameIt = armor.find("name");
                if (itemNameIt != armor.end() && std::get<std::string>(itemNameIt->second.value) == armorName) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                std::cerr << "npcs.traders[" << i << "].rewards.armor '" << armorName << "' does not exist in world.items.armors\n";
                return false;
            }
        }


        //
        // Validate trader reward "coins"
        //
        auto coinsIt = rewards.find("coins");
        if (coinsIt != rewards.end()) {
            if (!std::holds_alternative<double>(coinsIt->second.value)) {
                std::cerr << "'coins' must be an int\n";
                return false;
            }
        }

        auto dialogueIt = trade.find("dialogue"); // Find "dialogue"
        if (dialogueIt == trade.end()) {
            std::cerr << "npcs.traders[" << i << "] is missing 'dialogue'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(dialogueIt->second.value)) {
            std::cerr << "npcs.traders[" << i << "].dialogue must be a string\n";
            return false;
        }
    }






    //
    // Validate "world.locations"
    //
    auto locationsIt = world.find("locations");
    if (locationsIt == world.end()) {
        std::cerr << "Missing 'world.locations'\n";
        return false;
    }
    if (!std::holds_alternative<JsonArray>(locationsIt->second.value)) {
        std::cerr << "'world.locations' must be an array\n";
        return false;
    }
    const JsonArray& locations = std::get<JsonArray>(locationsIt->second.value);
    for (size_t i = 0; i < locations.size(); ++i) {
        if (!std::holds_alternative<JsonObject>(locations[i].value)) {
            std::cerr << "world.locations[" << i << "] must be an object\n";
            return false;
        }
        const JsonObject& location = std::get<JsonObject>(locations[i].value);

        auto nameIt = location.find("name"); // Find "name"
        if (nameIt == location.end()) {
            std::cerr << "world.locations[" << i << "] is missing 'name'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(nameIt->second.value)) {
            std::cerr << "world.locations[" << i << "].name must be a string\n";
            return false;
        }

        auto descriptionIt = location.find("description"); // Find "description"
        if (descriptionIt == location.end()) {
            std::cerr << "world.locations[" << i << "] is missing 'description'\n";
            return false;
        }
        if (!std::holds_alternative<std::string>(descriptionIt->second.value)) {
            std::cerr << "world.locations[" << i << "].description must be a string\n";
            return false;
        }

        auto exitsIt = location.find("exits"); // Find "exits"
        if (exitsIt == location.end()) {
            std::cerr << "world.locations[" << i << "] is missing 'exits'\n";
            return false;
        }
        if (!std::holds_alternative<JsonObject>(exitsIt->second.value)) {
            std::cerr << "'exits' must be an object\n";
            return false;
        }
        const JsonObject& exits = std::get<JsonObject>(exitsIt->second.value);
        for (const auto& [direction, destination] : exits) {
            if (direction != "north" && direction != "south" &&
                direction != "east" && direction != "west") {
                std::cerr << "exits has invalid direction '" << direction << "'\n";
                return false;
            }
            if (!std::holds_alternative<std::string>(destination.value)) {
                std::cerr << "exits['" << direction << "'] must be a string\n";
                return false;
            }
            bool found = false;
            for (const auto& location : locations) {
                const JsonObject& act = std::get<JsonObject>(location.value);
                const auto locnameIt = act.find("name");

                if (locnameIt != act.end() && std::get<std::string>(locnameIt->second.value) == std::get<std::string>(destination.value)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "world.locations[" << i << "].spawns.location must be existing\n";
                return false;
            }
        }

        auto spawnsIt = location.find("spawns"); // Find "spawns"
        if (spawnsIt == location.end()) {
            std::cerr << "world.locations[" << i << "] is missing 'spawns'\n";
            return false;
        }

        if (std::holds_alternative<std::nullptr_t>(spawnsIt->second.value)) {
            // null is allowed
        }
        else if (std::holds_alternative<JsonArray>(spawnsIt->second.value)) {
            const JsonArray& spawns = std::get<JsonArray>(spawnsIt->second.value);
            for (size_t j = 0; j < spawns.size(); ++j) {
                if (!std::holds_alternative<JsonObject>(spawns[j].value)) {
                    std::cerr << "world.locations[" << i << "].spawns[" << j << "] must be an object\n";
                    return false;
                }
                const JsonObject& spawn = std::get<JsonObject>(spawns[j].value);

                auto npc_typeIt = spawn.find("npc_type");
                if (npc_typeIt == spawn.end()) {
                    std::cerr << "world.locations[" << i << "].spawns[" << j << "] is missing 'npc_type'\n";
                    return false;
                }
                if (!std::holds_alternative<std::string>(npc_typeIt->second.value)) {
                    std::cerr << "world.locations[" << i << "].spawns[" << j << "].npc_type must be a string\n";
                    return false;
                }
                std::string npc_type_name = std::get<std::string>(npc_typeIt->second.value);
                bool found = false;
                // Check bosses
                for (const auto& boss : bosses) {
                    const JsonObject& act = std::get<JsonObject>(boss.value);
                    const auto locbossIt = act.find("name");

                    if (locbossIt != act.end() && std::get<std::string>(locbossIt->second.value) == npc_type_name) {
                        found = true;
                        break;
                    }
                }
                // Check weapons
                if (!found) {
                    for (const auto& trade : traders) {
                        const JsonObject& act = std::get<JsonObject>(trade.value);
                        const auto loctradeIt = act.find("name");

                        if (loctradeIt != act.end() && std::get<std::string>(loctradeIt->second.value) == npc_type_name) {
                            found = true;
                            break;
                        }
                    }
                }
                if (!found) {
                    std::cerr << "world.locations[" << i << "].spawns[" << j << "] references item '" << npc_type_name << "' which does not exist in world.npcs\n";
                    return false;
                }

                auto countIt = spawn.find("count");
                if (countIt == spawn.end()) {
                    std::cerr << "world.locations[" << i << "].spawns[" << j << "] is missing 'count'\n";
                    return false;
                }
                if (!std::holds_alternative<double>(countIt->second.value)) {
                    std::cerr << "world.locations[" << i << "].spawns[" << j << "].count must be an int\n";
                    return false;
                }
                if (std::get<double>(countIt->second.value) <= 0) {
                    std::cerr << "world.locations[" << i << "].spawns[" << j << "].count must be greater than 0\n";
                    return false;
                }
            }
        }
        else {
            std::cerr << "world.locations[" << i << "].spawns must be an array or null\n";
            return false;
        }

        auto locationItemsIt = location.find("items"); // Find "items"
        if (locationItemsIt == location.end()) {
            std::cerr << "world.locations[" << i << "] is missing 'items'\n";
            return false;
        }

        if (std::holds_alternative<std::nullptr_t>(locationItemsIt->second.value)) {
            // null is allowed
        }
        else if (std::holds_alternative<JsonArray>(locationItemsIt->second.value)) {
            const JsonArray& locationItems = std::get<JsonArray>(locationItemsIt->second.value);
            for (size_t j = 0; j < locationItems.size(); ++j) {
                if (!std::holds_alternative<std::string>(locationItems[j].value)) {
                    std::cerr << "world.locations[" << i << "].items[" << j << "] must be a string\n";
                    return false;
                }
                const std::string& itemName = std::get<std::string>(locationItems[j].value);
                bool found = false;
                // Check consumables
                for (const auto& item : consumables) {
                    const JsonObject& obj = std::get<JsonObject>(item.value);
                    const auto itemNameIt = obj.find("name");

                    if (itemNameIt != obj.end() && std::get<std::string>(itemNameIt->second.value) == itemName) {
                        found = true;
                        break;
                    }
                }
                // Check weapons
                if (!found) {
                    for (const auto& item : weapons) {
                        const JsonObject& obj = std::get<JsonObject>(item.value);
                        const auto itemNameIt = obj.find("name");

                        if (itemNameIt != obj.end() && std::get<std::string>(itemNameIt->second.value) == itemName) {
                            found = true;
                            break;
                        }
                    }
                }
                // Check armors
                if (!found) {
                    for (const auto& item : armors) {
                        const JsonObject& obj = std::get<JsonObject>(item.value);
                        const auto itemNameIt = obj.find("name");
                        if (itemNameIt != obj.end() && std::get<std::string>(itemNameIt->second.value) == itemName) {
                            found = true;
                            break;
                        }
                    }
                }
                if (!found) {
                    std::cerr << "world.locations[" << i << "].items[" << j << "] references item '" << itemName << "' which does not exist in world.items\n";
                    return false;
                }
            }
        }
        else {
            std::cerr << "world.locations[" << i << "].items must be an array or null\n";
            return false;
        }
        auto playersIt = location.find("players"); // Find "players"
        if (playersIt == location.end()) {
            std::cerr << "world.locations[" << i << "] is missing 'players'\n";
            return false;
        }
        if (!std::holds_alternative<JsonArray>(playersIt->second.value)) {
            std::cerr << "world.locations[" << i << "].players must be an array\n";
            return false;
        }
        const JsonArray& players = std::get<JsonArray>(playersIt->second.value);
        if (!players.empty()) {
            std::cerr << "world.locations[" << i << "].players must be empty\n";
            return false;
        }
    }
    return true;
}