#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

namespace {
    void log_success(Client *client, std::string event, std::string message) {
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "INFO";
        log["event"] = event;
        log["message"] = message;
        log["response"] = "OK";
        log["player"] = client->getNickname();
        if (event == "CHAT_GROUP")
            log["group"] = client->getGroupId();
        printJson(log);
        std::cout << std::endl;
    }
}

int handle_chat(Client *client, std::string line, std::vector<Client> *clients) {
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
    
    std::string rest = line.substr(5);
    int pos = rest.find(" ");
    std::string chat_command = rest.substr(0, pos);
    if (chat_command == "GLOBAL") {
        std::string message = "EVT GLOBAL CHAT " + client->getNickname() + ": "+ rest.substr(pos + 7) + "\r\n";
        for (Client c: (*clients)) {
            if (c.getFd() != client->getFd())
                send(c.getFd(), message.c_str(), message.size(), 0);
        }
        log_success(client, "CHAT_GLOBAL", message);
        send(client->getFd(), "OK\r\n", 4, 0);
        return 1;
    }
    else if (chat_command == "ROOM") {
        std::string message = "EVT ROOM CHAT " + client->getNickname() + ": "+ rest.substr(pos + 5) + "\r\n";
        for (Client c: (*clients)) {
            if (c.get_current_room() == client->get_current_room() &&
                c.getFd() != client->getFd()
            )
                send(c.getFd(), message.c_str(), message.size(), 0);
        }
        log_success(client, "CHAT_ROOM", message);
        send(client->getFd(), "OK\r\n", 4, 0);
        return 1;
    }
    else if (chat_command == "GROUP") {
        if (!client->in_group()) {
            log_warning(client, 401, line, "CHAT_GROUP", "ERR 401 NOT_IN_GROUP");
            std::string error_msg = "ERR 401 NOT_IN_GROUP\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        std::string message = "EVT GROUP CHAT " +  client->getNickname() + ": "+ rest.substr(pos + 6) + "\r\n";
        for (Client c: (*clients)) {
            if (c.getGroupId() == client->getGroupId() &&
                c.getFd() != client->getFd()
            )
                send(c.getFd(), message.c_str(), message.size(), 0);
        }
        log_success(client, "CHAT_GROUP", message);
        send(client->getFd(), "OK\r\n", 4, 0);
        return 1;
    }
    else {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
}