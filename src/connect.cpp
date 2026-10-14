#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

int handle_connect(Client *client, std::string line, std::vector<Client> *clients, std::vector<Client> *candidate_clients, JsonValue *world) {
    if (line.size() == 7) {
        log_error_ip(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    if (line.substr(0, 8) == "CONNECT ") {
        if (line.size() <= 8) {
            log_error_ip(client, line);
            std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
    }
    if (!client->is_connected) {
        bool name_available = true;
        std::string name = line.substr(8, line.size() - 1);
        if (!validNickname(name)) {
            log_connection_error(client, 911, "INVALID_NAME", "ERR 911 INVALID_NAME");
            std::string error_msg = "ERR 911 INVALID_NAME\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        for (Client c: (*clients)) {
            if (c.getNickname() == name) {
                log_connection_warning(client, 201, "NAME_IN_USE", "ERR 201 NAME_IN_USE");
                std::string error_msg = "ERR 201 NAME_IN_USE\r\n";
                name_available = false;
                send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
                return 0;
            }
        }
        if (name_available) {
            std::vector<Client>::iterator it =
            std::find_if((*candidate_clients).begin(), (*candidate_clients).end(),
            [&](const Client& c) {
                return c.getFd() == client->getFd();
            });
            if (it != (*candidate_clients).end()) {
                it->setNickname(name);
                it->is_connected = true;
                it->set_current_room((*world)["world"]["locations"][0]["name"].getString());
                for (unsigned long i = 0; i < (*world)["world"]["locations"].size(); i++) {
                    if ((*world)["world"]["locations"][i]["name"].getString() == it->get_current_room()) {
                        (*world)["world"]["locations"][i]["players"].push_back(JsonValue(it->getNickname()));
                        break;
                    }
                }
                log_connection_success(client);
                (*clients).push_back(std::move(*it));
                (*candidate_clients).erase(it);
                std::string welcome_msg = "OK connected\r\n";
                send(client->getFd(), welcome_msg.c_str(), welcome_msg.size(), 0);
                for (unsigned long index = 0; index < (*clients).size(); index++) {
                    std::string event = "EVT STATS players=" + std::to_string((*clients).size()) + "\r\n";
                    send((*clients)[index].getFd(), event.c_str(), event.size(), 0);
                }
            }
            return 1;
        }
    }
    else {
        log_connection_warning(client, 202, "NAME_IN_USE", "ERR 202 ALREADY_CONNECTED");
        std::string error_msg = "ERR 202 ALREADY_CONNECTED\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    return 0;
}