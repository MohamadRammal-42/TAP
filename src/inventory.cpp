#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

int handle_inventory(Client *client, std::string line) {
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
    if (line.size() != 9) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    std::string message = "[";
    for (std::string item: client->inventory) {
        message += "\"item." + item + "\", ";
    }
    message += "coins: " + std::to_string(client->getCoins()) += "]";
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "INFO";
    log["event"] = "INVENTORY";
    log["response"] = message;
    log["player"] = client->getNickname();
    printJson(log);
    std::cout << std::endl;
    message += "\r\n";
    send(client->getFd(), message.c_str(), message.size(), 0);
    return 1;
}