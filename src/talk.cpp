#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

int handle_talk(Client *client, std::string line, JsonValue *world) {
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
    if (line.size() <= 5) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    bool npc_found = false;
    std::string npc_name = line.substr(5);
    for (size_t index = 0; index < (*world)["world"]["locations"].size(); index++) {
        if (client->get_current_room() == (*world)["world"]["locations"][index]["name"].getString()) {
            for (size_t mosafa = 0; mosafa < (*world)["world"]["locations"][index]["spawns"].size(); mosafa++) {
                if ((*world)["world"]["locations"][index]["spawns"][mosafa]["npc_type"].getString() == npc_name) {
                    npc_found = true;
                    break;
                }
            }
        }
        if (npc_found)
            break;
    }
    bool quest = false;
    if (npc_found) {
        for (size_t index = 0; index < (*world)["world"]["npcs"]["traders"].size(); index++) {
            if ((*world)["world"]["npcs"]["traders"][index]["name"].getString() == npc_name) {
                for (size_t j = 0; j < client->quests.size(); j++) {
                    if (client->quests[j]["provider"].getString() == npc_name && client->quests[j]["type"].getString() == "eliminate") {
                        if (client->quests[j]["progress"].getDouble() >= client->quests[j]["required"].getDouble()) {
                            quest = true;
                            if (!std::get<bool>(client->quests[j]["completed"].value)) {
                                client->quests[j]["completed"] = true;
                            }
                            else {
                                quest = false;
                                break;
                            }
                            JsonObject& rewards = std::get<JsonObject>((*world)["world"]["npcs"]["traders"][index]["rewards"].value);
                            std::string drops[3] = {"weapon", "armor", "consumable"};
                            for (int k = 0; k < 3; k++) {
                                auto v = rewards.find(drops[k]);
                                if (v != rewards.end()) {
                                    client->inventory.push_back(v->second.getString());
                                }
                            }
                            auto v_coins = rewards.find("coins");
                            if (v_coins != rewards.end()) {
                                client->setCoins(client->getCoins() + v_coins->second.getDouble());
                                for (unsigned long q_index = 0; q_index < client->quests.size(); q_index++) {
                                    if (client->quests[q_index]["type"].getString() == "fetch" && client->quests[q_index]["target"].getString() == "coins"
                                        && !std::get<bool>(client->quests[q_index]["completed"].value)) {
                                        client->quests[q_index]["progress"] = client->quests[q_index]["progress"].getDouble() + 1;
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
                            }
                            auto v_exp = rewards.find("exp");
                            if (v_exp != rewards.end()) {
                                client->setXp(client->getXp() + v_exp->second.getDouble());
                            }
                            std::string message = ("OK " +
                                (*world)["world"]["npcs"]["traders"][index]["qc_dialogue"].getString() + "\n"
                                + ReturnJson((*world)["world"]["npcs"]["traders"][index]["rewards"])
                            );
                            // client->quests.erase(client->quests.begin() + j);
                            JsonObject log = JsonObject();
                            log["timestamp"] = timestamp();
                            log["level"] = "INFO";
                            log["event"] = line;
                            log["response"] = message;
                            log["player"] = client->getNickname();
                            printJson(log);
                            std::cout << std::endl;
                            message += "\r\n";
                            send(client->getFd(), message.c_str(), message.size(), 0);
                        }
                    }
                    else if (client->quests[j]["provider"].getString() == npc_name && client->quests[j]["type"].getString() == "fetch") {
                        if (client->quests[j]["target"].getString() != "coins") {
                            int count_obj = 0;
                            for (unsigned long k = 0; k < client->inventory.size(); k++) {
                                if (client->inventory[k] == (*world)["world"]["npcs"]["traders"][index]["quest"]["target"].getString()) {
                                    count_obj += 1;
                                }
                            }
                            if (count_obj >= client->quests[j]["required"].getDouble()) {
                                if (!std::get<bool>(client->quests[j]["completed"].value)) {
                                    client->quests[j]["completed"] = true;
                                }
                                else {
                                    quest = false;
                                    break;
                                }
                                std::cout << "hehehehehe\n";
                                std::string message = ("OK " +
                                    (*world)["world"]["npcs"]["traders"][index]["qc_dialogue"].getString() + "\n"
                                    + ReturnJson((*world)["world"]["npcs"]["traders"][index]["rewards"])
                                    + "\r\n"
                                );
                                quest = true;
                            }
                        }
                        else if (client->quests[j]["target"].getString() == "coins") {
                            if (client->getCoins() >= client->quests[j]["required"].getDouble()) {
                                if (!std::get<bool>(client->quests[j]["completed"].value)) {
                                    client->quests[j]["completed"] = true;
                                }
                                else {
                                    quest = false;
                                    break;
                                }
                                std::string message = ("OK " +
                                    (*world)["world"]["npcs"]["traders"][index]["qc_dialogue"].getString() + "\n"
                                    + ReturnJson((*world)["world"]["npcs"]["traders"][index]["rewards"])
                                    + "\r\n"
                                );
                                client->setCoins(client->getCoins() - client->quests[j]["required"].getDouble());
                                for (unsigned long q_index = 0; q_index < client->quests.size(); q_index++) {
                                    if (client->quests[q_index]["type"].getString() == "fetch" && client->quests[q_index]["target"].getString() == "coins"
                                        && !std::get<bool>(client->quests[q_index]["completed"].value)) {
                                        client->quests[q_index]["progress"] = client->quests[q_index]["progress"].getDouble() + 1;
                                        JsonObject log = JsonObject();
                                        log["timestamp"] = timestamp();
                                        log["level"] = "INFO";
                                        log["event"] = "QUEST_PROGRESS";
                                        log["progress"] = client->getCoins();
                                        log["target"] = client->quests[q_index]["required"].getDouble();
                                        log["player"] = client->getNickname();
                                        printJson(log);
                                        std::cout << std::endl;
                                    }
                                }
                                quest = true;
                            }
                        }
                        if (quest)
                        {
                            JsonObject& rewards = std::get<JsonObject>((*world)["world"]["npcs"]["traders"][index]["rewards"].value);
                            std::string drops[3] = {"weapon", "armor", "consumable"};
                            for (int k = 0; k < 3; k++) {
                                auto v = rewards.find(drops[k]);
                                if (v != rewards.end()) {
                                    client->inventory.push_back(v->second.getString());
                                }
                            }
                            auto v_coins = rewards.find("coins");
                            if (v_coins != rewards.end()) {
                                client->setCoins(client->getCoins() + v_coins->second.getDouble());
                                for (unsigned long q_index = 0; q_index < client->quests.size(); q_index++) {
                                    if (client->quests[q_index]["type"].getString() == "fetch" && client->quests[q_index]["target"].getString() == "coins"
                                        && !std::get<bool>(client->quests[q_index]["completed"].value)) {
                                        client->quests[q_index]["progress"] = client->quests[q_index]["progress"].getDouble() + 1;
                                        JsonObject log = JsonObject();
                                        log["timestamp"] = timestamp();
                                        log["level"] = "INFO";
                                        log["event"] = "QUEST_PROGRESS";
                                        log["progress"] = client->getCoins();
                                        log["target"] = client->quests[q_index]["required"].getDouble();
                                        log["player"] = client->getNickname();
                                        printJson(log);
                                        std::cout << std::endl;
                                    }
                                }
                            }
                            auto v_exp = rewards.find("exp");
                            if (v_exp != rewards.end()) {
                                client->setXp(client->getXp() + v_exp->second.getDouble());
                            }
                            std::string message = ("OK " +
                                (*world)["world"]["npcs"]["traders"][index]["qc_dialogue"].getString() + "\n"
                                + ReturnJson((*world)["world"]["npcs"]["traders"][index]["rewards"])
                            );
                            // client->quests.erase(client->quests.begin() + j);
                            JsonObject log = JsonObject();
                            log["timestamp"] = timestamp();
                            log["level"] = "INFO";
                            log["event"] = line;
                            log["response"] = message;
                            log["player"] = client->getNickname();
                            printJson(log);
                            std::cout << std::endl;
                            message += "\r\n";
                            send(client->getFd(), message.c_str(), message.size(), 0);
                        }
                    }
                    
                }
                if (!quest) {
                    std::string message = "OK " + (*world)["world"]["npcs"]["traders"][index]["dialogue"].getString() + "\r\n";
                    JsonObject log = JsonObject();
                    log["timestamp"] = timestamp();
                    log["level"] = "INFO";
                    log["event"] = line;
                    log["response"] = message;
                    log["player"] = client->getNickname();
                    printJson(log);
                    std::cout << std::endl;
                    send(client->getFd(), message.c_str(), message.size(), 0);
                }
            }
        }
    }
    else {
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
    return 1;
}