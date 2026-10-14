#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

int handle_status(Client *client, std::string line) {
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
    std::string reply = "OK ";
    JsonValue stats = JsonValue(JsonObject());
    stats["hp"] = client->getHp();
    stats["max_hp"] = client->getMaxHp();
    stats["attack_power"] = client->getAttackPower() * client->attack_multiplier;
    stats["defense_ratio"] = client->defense_ratio;
    stats["coins"] = client->getCoins();
    stats["level"] = client->getLevel();
    stats["xp"] = client->getXp();
    reply += ReturnJson(stats);
    reply += "\r\n";
    send(client->getFd(), reply.c_str(), reply.size(), 0);
    return 1;
}