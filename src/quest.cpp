#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

int handle_quests(Client *client, std::string line) {
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
    if (line.size() != 6) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "INFO";
    log["event"] = "LOOK";
    log["response"] = "OK";
    log["data"] = client->quests;
    log["player"] = client->getNickname();
    printJson(log);
    std::cout << std::endl;
    std::string client_quests = "OK " +  ReturnJson(client->quests) + "\r\n";
    send(client->getFd(), client_quests.c_str(), client_quests.size(), 0);
    return 1;
}

int handle_quest(Client *client, std::string line, JsonValue *world) {
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
    if (line.size() <= 6) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    std::string npc_name = line.substr(6);
    bool npc_found = false;
    for (size_t index = 0; index < (*world)["world"]["locations"].size(); index++) {
        if (client->get_current_room() == (*world)["world"]["locations"][index]["name"].getString()) {
            for (size_t mosafa = 0; mosafa < (*world)["world"]["locations"][index]["spawns"].size(); mosafa++) {
                if ((*world)["world"]["locations"][index]["spawns"][mosafa]["npc_type"].getString() == npc_name) {
                    npc_found = true;
                }
            }
        }
    }
    if (npc_found) {
        for (size_t index = 0; index < (*world)["world"]["npcs"]["traders"].size(); index++) {
            if ((*world)["world"]["npcs"]["traders"][index]["name"].getString() == npc_name) {
                JsonObject quest = JsonObject();
                JsonObject q = JsonObject();
                if ((*world)["world"]["npcs"]["traders"][index]["quest"]["status"].getString() == "available") {
                    quest["quest_id"] = (*world)["world"]["npcs"]["traders"][index]["quest"]["type"].getString() + "_" + (*world)["world"]["npcs"]["traders"][index]["quest"]["target"].getString();
                    quest["description"] = (
                        (*world)["world"]["npcs"]["traders"][index]["quest"]["type"].getString() + " " +
                        std::to_string((int)(*world)["world"]["npcs"]["traders"][index]["quest"]["count"].getDouble()) + " " +
                        (*world)["world"]["npcs"]["traders"][index]["quest"]["target"].getString()
                    );
                    q["type"] = (*world)["world"]["npcs"]["traders"][index]["quest"]["type"].getString();
                    q["target"] = (*world)["world"]["npcs"]["traders"][index]["quest"]["target"].getString();
                    q["required"] = (*world)["world"]["npcs"]["traders"][index]["quest"]["count"].getDouble();
                    q["quest_id"] = quest["quest_id"];
                    q["provider"] = (*world)["world"]["npcs"]["traders"][index]["name"].getString();
                    q["completed"] = false;
                    if (q["type"].getString() == "eliminate")
                        q["progress"] = 0.0;
                    else
                    {
                        std::string target = q["target"].getString();
                        double progress = 0;
                        if (target == "coins") {
                            progress = client->getCoins();
                        }
                        else {
                            for (unsigned long k = 0; k < client->inventory.size(); k++) {
                                if (client->inventory[k] == target) {
                                    progress += 1;
                                }
                            }
                        }
                        q["progress"] = progress;
                    }
                    std::cout << std::endl;
                    quest["rewards"] = (*world)["world"]["npcs"]["traders"][index]["rewards"];
                    client->quests.push_back(q);
                    (*world)["world"]["npcs"]["traders"][index]["quest"]["status"] = "unavailable";
                    std::string quest_message = "OK " + ReturnJson(quest) + "\r\n";
                    JsonObject log = JsonObject();
                    log["timestamp"] = timestamp();
                    log["level"] = "INFO";
                    log["event"] = line;
                    log["response"] = "OK";
                    log["data"] = quest;
                    log["player"] = client->getNickname();
                    printJson(log);
                    std::cout << std::endl;
                    send(client->getFd(), quest_message.c_str(), quest_message.size(), 0);
                }
                else {
                    JsonObject log = JsonObject();
                    log["timestamp"] = timestamp();
                    log["level"] = "ERROR";
                    log["event"] = "NO_QUEST_AVAILABLE";
                    log["response"] = "ERR 406 NO_QUEST_AVAILABLE";
                    log["code"] = 406;
                    log["player"] = client->getNickname();
                    printJson(log);
                    std::cout << std::endl;
                    std::string message = "ERR 406 NO_QUEST_AVAILABLE\r\n";
                    send(client->getFd(), message.c_str(), message.size(), 0);
                    break;
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
        return 0;;
    }
    return 1;
}