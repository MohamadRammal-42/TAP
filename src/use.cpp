#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

int handle_use(Client *client, std::string line, JsonValue *world) {
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
    if (line.size() <= 4) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    std::string item = line.substr(4);
    bool item_found = false;
    for (std::string i: client->inventory) {
        if (i == item) {
            item_found = true;
            for (size_t j = 0; j < (*world)["world"]["items"]["consumables"].size(); j++) {
                if (item == (*world)["world"]["items"]["consumables"][j]["name"].getString())
                {
                    client->setHp(
                        ((client->getHp() + (*world)["world"]["items"]["consumables"][j]["heal"].getDouble()) < client->getMaxHp())
                        ? client->getHp() + (*world)["world"]["items"]["consumables"][j]["heal"].getDouble()
                        : client->getMaxHp()

                    );
                    for (unsigned long index = 0; index < client->inventory.size(); index++) {
                        if (client->inventory[index] == item) {
                            client->inventory.erase(client->inventory.begin() + index);
                            break;
                        }
                    }
                    JsonObject log = JsonObject();
                    log["timestamp"] = timestamp();
                    log["level"] = "INFO";
                    log["event"] = line;
                    log["response"] = "OK";
                    log["player"] = client->getNickname();
                    printJson(log);
                    std::cout << std::endl;
                    send(client->getFd(), success.c_str(), success.size(), 0);
                }
            }
            for (size_t j = 0; j < (*world)["world"]["items"]["weapons"].size(); j++) {
                if (item == (*world)["world"]["items"]["weapons"][j]["name"].getString())
                {
                    if (client->current_weapon != "") {
                        client->inventory.push_back(client->current_weapon);
                    }
                    client->current_weapon = (*world)["world"]["items"]["weapons"][j]["name"].getString();
                    client->weapon_durability = (*world)["world"]["items"]["weapons"][j]["durability"].getDouble();
                    client->attack_multiplier = (*world)["world"]["items"]["weapons"][j]["attack"].getDouble();
                    for (unsigned long index = 0; index < client->inventory.size(); index++) {
                        if (client->inventory[index] == item) {
                            client->inventory.erase(client->inventory.begin() + index);
                            break;
                        }
                    }
                    JsonObject log = JsonObject();
                    log["timestamp"] = timestamp();
                    log["level"] = "INFO";
                    log["event"] = line;
                    log["response"] = "OK";
                    log["player"] = client->getNickname();
                    printJson(log);
                    std::cout << std::endl;
                    send(client->getFd(), success.c_str(), success.size(), 0);
                }
            }
            for (size_t j = 0; j < (*world)["world"]["items"]["armors"].size(); j++) {
                if (item == (*world)["world"]["items"]["armors"][j]["name"].getString())
                {
                    if (client->current_armor != "") {
                        client->inventory.push_back(client->current_armor);
                    }
                    client->current_armor = (*world)["world"]["items"]["armors"][j]["name"].getString();
                    client->armor_durability = (*world)["world"]["items"]["armors"][j]["durability"].getDouble();
                    client->defense_ratio = (*world)["world"]["items"]["armors"][j]["defense"].getDouble();
                    for (unsigned long index = 0; index < client->inventory.size(); index++) {
                        if (client->inventory[index] == item) {
                            client->inventory.erase(client->inventory.begin() + index);
                            break;
                        }
                    }
                    JsonObject log = JsonObject();
                    log["timestamp"] = timestamp();
                    log["level"] = "INFO";
                    log["event"] = line;
                    log["response"] = "OK";
                    log["player"] = client->getNickname();
                    printJson(log);
                    std::cout << std::endl;
                    send(client->getFd(), success.c_str(), success.size(), 0);
                }
            }
            break;
        }
    }
    if (!item_found) {
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "ERROR";
        log["event"] = "ITEM_NOT_FOUND";
        log["response"] = "ERR 404 ITEM_NOT_FOUND";
        log["code"] = 404;
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
        std::string fail = "ERR 404 ITEM_NOT_FOUND\r\n";
        send(client->getFd(), fail.c_str(), fail.size(), 0);
        return 0;
    }
    return 1;
}

int handle_unuse(Client *client, std::string line) {
    if (!client->is_connected) {
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "ERROR";
        log["event"] = "UNAUTHORIZED_ACTION";
        log["response"] = "ERR 710 UNAUTHORIZED_ACTION";
        log["code"] = 710;
        log["player"] = client->getNickname();
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
    std::string item = line.substr(6);
    bool item_found = false;
    if (client->current_weapon != "" && client->current_weapon == item) {
        item_found = true;
        client->inventory.push_back(client->current_weapon);
        client->current_weapon = "";
        client->attack_multiplier = 1;
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "INFO";
        log["event"] = line;
        log["response"] = "OK";
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
        send(client->getFd(), success.c_str(), success.size(), 0);
    }
    else if (client->current_armor != "" && client->current_armor == item) {
        item_found = true; 
        client->inventory.push_back(client->current_armor);
        client->current_armor = "";
        client->defense_ratio = 0;
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "INFO";
        log["event"] = line;
        log["response"] = "OK";
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
        send(client->getFd(), success.c_str(), success.size(), 0);       
    }
    if (!item_found) {
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "WARN";
        log["event"] = "ITEM_NOT_FOUND";
        log["response"] = "ERR 404 ITEM_NOT_FOUND";
        log["code"] = 404;
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
        std::string fail = "ERR 404 ITEM_NOT_FOUND\r\n";
        send(client->getFd(), fail.c_str(), fail.size(), 0);
        return 0;
    }
    return 1;
}