#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

int handle_attack(Client *client, std::string line, JsonValue *world, Locations *npcs) {
    if (!client->is_connected) {
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "ERROR";
        log["event"] = "UNAUTHORIZED_ACTION";
        log["response"] = "ERR 710 UNAUTHORIZED_ACTION";
        log["code"] = 710;
        log["ip"] = client->getIp();
        log["port"] = client->port;
        printJson(log);
        std::cout << std::endl;
        std::string error_msg = "ERR 710 UNAUTHORIZED_ACTION\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    if (line.size() <= 7) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    std::string npc_name = line.substr(7);
    JsonValue& locations = (*world)["world"]["locations"];
    std::string current_location;
    for (size_t j = 0; j < locations.size(); j++) {
        if (locations[j]["name"].getString() == client->get_current_room()) {
            current_location = locations[j]["name"].getString();
            break;
        }
    }
    LocationNpcsNode *tmp = npcs->head;
    while (tmp && tmp->location != current_location) {
        tmp = tmp->next;
    }
    Node *npc_node = tmp->npcs->head;
    while (npc_node) {
        if (npc_node->npc->getNickname() == npc_name && npc_node->npc->timer <= time(0))
            break;
        npc_node = npc_node->next;
    }
    if (!npc_node) {
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "WARN";
        log["event"] = "NPC_NOT_FOUND";
        log["response"] = "ERR 404 NPC_NOT_FOUND";
        log["code"] = 404;
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
        std::string error_msg = "ERR 404 NPC_NOT_FOUND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    npc_node->npc->setHp(npc_node->npc->getHp() - (client->getAttackPower() * client->attack_multiplier));
    if (npc_node->npc->getHp() > 0) {
        if (dynamic_cast<Boss*>(npc_node->npc)) {
            Boss* boss = dynamic_cast<Boss*>(npc_node->npc);
            client->setHp(client->getHp() - (boss->getAttackPower() - (boss->getAttackPower() * client->defense_ratio) / 100));
            if (static_cast<int>(client->defense_ratio) != 0) {
                std::uniform_int_distribution<> distrib(1, static_cast<int>(client->armor_durability));
                if (static_cast<int>(distrib(gen)) == 1) {
                    client->current_armor = "";
                    client->defense_ratio = 0;
                }
            }
        }
        if (client->getHp() > 0)
        {
            JsonObject combat_status = JsonObject{};
            combat_status["attacker_hp"] = client->getHp();
            combat_status["target_hp"] = npc_node->npc->getHp();
            combat_status["damage"] = client->getAttackPower() * client->attack_multiplier;
            combat_status["status"] = "combat";
            std::string status = ReturnJson(combat_status);
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "INFO";
            log["event"] = line;
            log["response"] = "OK";
            log["data"] = combat_status;
            log["player"] = client->getNickname();
            printJson(log);
            std::cout << std::endl;
            status += "\r\n";
            send(client->getFd(), (success + status).c_str(), (success + status).size() + 1, 0);
        }
        else {
            std::string death_message = "You were eliminated by " + npc_node->npc->getNickname();
            client->setHp(client->getMaxHp() / 2);
            client->set_current_room((*world)["world"]["locations"][0]["name"].getString());
            std::cout << client->getAttackPower() << std::endl;
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "INFO";
            log["event"] = line;
            log["response"] = death_message;
            log["player"] = client->getNickname();
            printJson(log);
            std::cout << std::endl;
            death_message += "\r\n";
            send(client->getFd(), death_message.c_str(), death_message.size(), 0);
            return 0;
        }
    }
    else {
        std::string elimination_message = "You killed " + npc_node->npc->getNickname() + "\n";
        if (dynamic_cast<Boss*>(npc_node->npc))
        {
            elimination_message += "You gained " + ReturnJson(npc_node->npc->drops["drops"]);
            client->setCoins(client->getCoins() + npc_node->npc->drops["drops"]["coins"].getDouble());
            client->setXp(client->getXp() + npc_node->npc->drops["drops"]["exp"].getDouble());
            JsonObject& drops = std::get<JsonObject>(
                npc_node->npc->drops["drops"].value
            );
            if (drops.find("weapon") != drops.end()){
                for(size_t loc = 0; loc < (*world)["world"]["locations"].size(); loc++) {
                    if((*world)["world"]["locations"][loc]["name"].getString() == client->get_current_room()) {
                        (*world)["world"]["locations"][loc]["items"].push_back(drops["weapon"]);
                    }
                }
            }
            if (drops.find("consumable") != drops.end()){
                for(size_t loc = 0; loc < (*world)["world"]["locations"].size(); loc++) {
                    if((*world)["world"]["locations"][loc]["name"].getString() == client->get_current_room()) {
                        (*world)["world"]["locations"][loc]["items"].push_back(drops["consumable"]);
                    }
                }
            }
        }
        for (unsigned long index = 0; index < client->quests.size(); index++) {
            if (client->quests[index]["type"].getString() == "eliminate" && client->quests[index]["target"].getString() == npc_node->npc->getNickname()
                && !std::get<bool>(client->quests[index]["completed"].value)) {
                client->quests[index]["progress"] = client->quests[index]["progress"].getDouble() + 1;
                JsonObject log = JsonObject();
                log["timestamp"] = timestamp();
                log["level"] = "INFO";
                log["event"] = "QUEST_PROGRESS";
                log["progress"] = client->quests[index]["progress"].getDouble();
                log["target"] = client->quests[index]["required"].getDouble();
                log["player"] = client->getNickname();
                printJson(log);
                std::cout << std::endl;
            }
            if (client->quests[index]["type"].getString() == "fetch" && client->quests[index]["target"].getString() == "coins"
                && !std::get<bool>(client->quests[index]["completed"].value)) {
                client->quests[index]["progress"] = client->quests[index]["progress"].getDouble() + 1;
                JsonObject log = JsonObject();
                log["timestamp"] = timestamp();
                log["level"] = "INFO";
                log["event"] = "QUEST_PROGRESS";
                log["progress"] = client->getCoins();
                log["target"] = client->quests[index]["required"].getDouble();
                log["player"] = client->getNickname();
                printJson(log);
                std::cout << std::endl;
            }
        }
        if (client->getXp() >= 50 * client->lvlup_buffer) {
            client->setXp(client->getXp() - (50 * client->lvlup_buffer));
            client->setLevel(client->getLevel() + 1);
            client->setAttackPower(client->getAttackPower() + 2);
            client->setMaxHp(client->getMaxHp() + 20);
            client->setHp(client->getMaxHp());
            client->lvlup_buffer += 0.2;
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "INFO";
            log["event"] = "LEVEL_UP";
            log["new_level"] = client->getLevel();
            log["player"] = client->getNickname();
            printJson(log);
        }
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "INFO";
        log["event"] = line;
        log["response"] = elimination_message;
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
        elimination_message += "\r\n";
        send(client->getFd(), elimination_message.c_str(), elimination_message.size(), 0);
        npc_node->npc->timer = time(nullptr) + 60;
        npc_node->npc->setHp(npc_node->npc->getMaxHp());
        if (npc_node != tmp->npcs->tail) {
            if (npc_node->prev)
                npc_node->prev->next = npc_node->next;
            else
                tmp->npcs->head = npc_node->next;
            if (npc_node->next)
                npc_node->next->prev = npc_node->prev;
            npc_node->prev = tmp->npcs->tail;
            npc_node->next = nullptr;
            if (tmp->npcs->tail)
                tmp->npcs->tail->next = npc_node;
            else
                tmp->npcs->head = npc_node;
            tmp->npcs->tail = npc_node;
        }
    }
    if (static_cast<double>(client->attack_multiplier) != 1) {
        std::uniform_int_distribution<> distrib(1, static_cast<int>(client->weapon_durability));
        if (static_cast<int>(distrib(gen)) == 1) {
            client->current_weapon = "";
            client->attack_multiplier = 1;\
        }
    }
    return 1;
}