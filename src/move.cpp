#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>
#include <unistd.h>

int handle_move(Client *client, std::string line, std::vector<Client> *clients, JsonValue *world) {
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
    std::string move_direction = line.substr(5);
    std::string current_room = client->get_current_room();
    JsonValue& locations = (*world)["world"]["locations"];
    JsonValue* currentLocationData = nullptr;
    for (size_t j = 0; j < locations.size(); ++j) {
        if (locations[j]["name"].getString() == current_room) {
            currentLocationData = &locations[j];
            break;
        }
    }
    if (currentLocationData == nullptr) {
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "WARN";
        log["event"] = "LOCATION_NOT_FOUND";
        log["response"] = "ERR 404 LOCATION_NOT_FOUND";
        log["code"] = 404;
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
        std::string error_msg = "ERR 404 LOCATION_NOT_FOUND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    JsonValue& exits = (*currentLocationData)["exits"]; 
    if (!exits.contains(move_direction)) { 
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "WARN";
        log["event"] = "NO_EXIT";
        log["response"] = "ERR 301 NO_EXIT";
        log["code"] = 301;
        log["player"] = client->getNickname();
        printJson(log);
        std::cout << std::endl;
        std::string error_msg = "ERR 301 NO_EXIT\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    std::string destination = exits[move_direction].getString();
    std::string room = client->get_current_room();
    std::string name = client->getNickname();
    for (size_t k = 0; k < (*currentLocationData)["players"].size(); k++) {
        if ((*currentLocationData)["players"][k].getString() == name) {
            (*currentLocationData)["players"].erase(k);
            k--;
        }
        else {
            int fd = get_client_by_nickname((*currentLocationData)["players"][k].getString(), (*clients))->getFd();
            std::string message = "EVT ROOM PRESENCE LEAVE "+ name + "\r\n";
            send(fd, message.c_str(), message.size(), 0);
        }
    }
    for (size_t j = 0; j < locations.size(); ++j) {
        if (locations[j]["name"].getString() == destination) {
            for (unsigned long player = 0; player < locations[j]["players"].size(); player++) {
                int fd = get_client_by_nickname(locations[j]["players"][player].getString(), (*clients))->getFd();
                std::string message = "EVT ROOM PRESENCE ENTER "+ name + "\r\n";
                send(fd, message.c_str(), message.size(), 0);
            }
            locations[j]["players"].push_back(JsonValue(client->getNickname()));
            break;
        }
    }
    client->set_current_room(destination);
    std::string s = "OK room=" + destination + "\r\n";
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "INFO";
    log["event"] = line;
    log["from"] = room;
    log["to"] = destination;
    log["response"] = "OK room=" + destination;
    log["player"] = client->getNickname();
    printJson(log);
    std::cout << std::endl;
    send(client->getFd(), s.c_str(), s.size(), 0);
    return 1;
}