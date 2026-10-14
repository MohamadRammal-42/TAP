#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

void log_success(Client *client, JsonValue room) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "INFO";
    log["event"] = "LOOK";
    log["response"] = "OK";
    log["data"] = room;
    log["player"] = client->getNickname();
    printJson(log);
    std::cout << std::endl;
}

int handle_look(Client *client, std::string line, JsonValue *world) {
    if (!client->is_connected) {
        log_unauthorized_action(client);
        std::string error_msg = "ERR 710 UNAUTHORIZED_ACTION\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    if (line.size() != 4) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    JsonValue room = JsonValue(nullptr);
    for (size_t i = 0; i < (*world)["world"]["locations"].size(); i++) {
        if ((*world)["world"]["locations"][i]["name"].getString() == client->get_current_room())
        {
            room = (*world)["world"]["locations"][i];
            break;
        }
    }
    if (!room.isNull()) {
        std::string full_json = "OK " + ReturnJson(room);
        full_json += "\r\n";
        log_success(client, room);
        send(client->getFd(), full_json.c_str(), full_json.size(), 0);
        return 1;
    }
    return 0;
}