#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

namespace {
    void log_success(Client *client, std::string line, std::string message) {
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "INFO";
        log["event"] = line;
        log["response"] = message;
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
    }
}

int handle_drop(Client *client, std::string line, JsonValue *world) {
    if (!client->is_connected) {
        log_unauthorized_action(client);
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
    std::string item = line.substr(5);
    JsonValue& locations = (*world)["world"]["locations"];
    JsonValue* current_location = nullptr;
    bool is_found = false;
    for (size_t j = 0; j < locations.size(); j++) {
        if (locations[j]["name"].getString() == client->get_current_room()) {
            current_location = &(locations[j]);
            break;
        }
    }
    for (size_t k = 0; k < client->inventory.size(); k++) {
        if (item == client->inventory[k]) {
            client->inventory.erase(client->inventory.begin() + k);
            (*current_location)["items"].push_back(item);
            is_found = true;
            std::string message = "OK droped=items." + item;
            log_success(client, line, message);
            message += "\r\n";
            send(client->getFd(), message.c_str(), message.size(), 0);
            break;
        }
    }
    if (!is_found) {
        log_warning(client, 404, line, "ITEM_NOT_FOUND", "ERR 404 ITEM_NOT_FOUND");
        std::string error_msg = "ERR 404 ITEM_NOT_FOUND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        is_found = false;
    }
    return 1;
}