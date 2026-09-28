#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

int handle_group(Client *client, std::string line, std::vector<Client> *clients, int *group_constant) {
    if (!client->is_connected) {
        std::string error_msg = "ERR 710 UNAUTHORIZED_ACTION\r\n";
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "ERROR";
        log["code"] = 710;
        log["error"] = "UNAUTHORIZED_ACTION";
        log["response"] = "ERR 710 UNAUTHORIZED_ACTION";
        log["ip"] = client->getIp();
        log["port"] = client->port;
        printJson(log);
        std::cout << std::endl;
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    if (line.size() == 5) {
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        log_error(client, line);
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    std::string rest = line.substr(6);
    int pos = rest.find(" ");
    std::string group_command = rest.substr(0, pos);
    if (group_command == "INVITE") {
        if (!client->in_group()) {
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "WARN";
            log["event"] = "GROUP_INVITE";
            log["response"] = "ERR 401 NOT_IN_GROUP";
            log["code"] = 401;
            log["error"] = "NOT_IN_GROUP";
            log["player"] = client->getNickname();
            printJson(log);
            std::cout << std::endl;
            std::string error_msg = "ERR 401 NOT_IN_GROUP\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        std::string client_name = rest.substr(pos + 1);
        Client *receiving_client = get_client_by_nickname(client_name, (*clients));
        if (!receiving_client) {
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["plyer"] = client->getNickname();
            log["response"] = "ERR 404 PLAYER_NOT_FOUND";
            log["level"] = "WARN";
            log["event"] = "PLAYER_NOT_FOUND";
            log["code"] = 404;
            printJson(log);
            std::cout << std::endl;
            std::string error_msg = "ERR 404 PLAYER_NOT_FOUND\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        if (receiving_client->in_group()) {
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["plyer"] = client->getNickname();
            log["response"] = "ERR 402 ALREADY_IN_GROUP";
            log["level"] = "WARN";
            log["event"] = "ALREADY_IN_GROUP";
            log["code"] = 402;
            printJson(log);
            std::cout << std::endl;
            std::string error_msg = "ERR 402 ALREADY_IN_GROUP\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        else {
            std::string invitation = (
                "EVT GROUP INVITE " +
                client->getNickname() + " " +
                std::to_string(client->getGroupId()) + "\r\n"
            );
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "INFO";
            log["event"] = "GROUP_INVITE";
            log["response"] = "OK";
            log["sender"] = client->getNickname();
            log["receiver"] = receiving_client->getNickname();
            log["group"] = client->getGroupId();
            printJson(log);
            std::cout << std::endl;
            send(receiving_client->getFd(), invitation.c_str(), invitation.size(), 0);
            send(client->getFd(), success.c_str(), success.size(), 0);
            return 1;
        }
    }
    else if (group_command == "LEAVE") {
        if (!client->in_group()) {
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "WARN";
            log["event"] = "GROUP_LEAVE";
            log["code"] = 401;
            log["response"] = "ERR 401 NOT_IN_GROUP";
            log["error"] = "NOT_IN_GROUP";
            log["player"] = client->getNickname();
            printJson(log);
            std::cout << std::endl;
            std::string error_msg = "ERR 401 NOT_IN_GROUP\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        else {
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "INFO";
            log["event"] = "GROUP_LEAVE";
            log["response"] = "OK";
            log["player"] = client->getNickname();
            log["group"] = client->getGroupId();
            printJson(log);
            std::cout << std::endl;
            send(client->getFd(), success.c_str(), success.size(), 0);
            std::string event_message = "EVT GROUP LEAVE " + client->getNickname() + "\r\n";
            for (int k = 0; k < (*clients).size(); k++) {
                if (((*clients)[k].getGroupId() == client->getGroupId()) && (*clients)[k].getFd() != client->getFd()) {
                    send((*clients)[k].getFd(), event_message.c_str(), event_message.size(), 0);
                }
            }
            client->setGroupId(-1);
        }
    }
    else if (group_command == "JOIN") {
        if (client->in_group()) {
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "WARN";
            log["event"] = "GROUP_JOIN";
            log["code"] = 402;
            log["response"] = "ERR 402 ALREADY_IN_GROUP";
            log["error"] = "ALREADY_IN_GROUP";
            log["player"] = client->getNickname();
            printJson(log);
            std::cout << std::endl;
            std::string error_msg = "ERR 402 ALREADY_IN_GROUP\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        bool valid_group_name = true;
        std::string group = rest.substr(pos + 1);
        for (char c: group) {
            if (!isdigit(c)) {
                log_error(client, line);
                std::string error_msg = "ERR 911 INVALID_GROUP\r\n";
                send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
                valid_group_name = false;
                break;
            }
        }
        if (!valid_group_name)
            return 0;
        int group_id = atoi(group.c_str());
        bool group_found = false;
        for (Client c: (*clients)) {
            if (c.getGroupId() == group_id) {
                JsonObject log = JsonObject();
                log["timestamp"] = timestamp();
                log["level"] = "INFO";
                log["event"] = "GROUP_JOIN";
                log["response"] = "OK";
                log["player"] = client->getNickname();
                log["group"] = client->getGroupId();
                printJson(log);
                std::cout << std::endl;
                client->setGroupId(group_id);
                std::string event_message = "EVT GROUP JOIN " + client->getNickname() + "\r\n";
                for (int k = 0; k < (*clients).size(); k++) {
                    if (((*clients)[k].getGroupId() == client->getGroupId()) && (*clients)[k].getFd() != client->getFd()) {
                        send((*clients)[k].getFd(), event_message.c_str(), event_message.size(), 0);
                    }
                }
                send(client->getFd(), success.c_str(), success.size(), 0);
                group_found = true;
                return 1;
            }
        }
        if (!group_found) {
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "WARN";
            log["event"] = "GROUP_JOIN";
            log["code"] = 404;
            log["response"] = "ERR 404 GROUP_NOT_FOUND";
            log["error"] = "GROUP_NOT_FOUND";
            log["player"] = client->getNickname();
            printJson(log);
            std::cout << std::endl;
            std::string error_msg = "ERR 404 GROUP_NOT_FOUND\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
    }
    else if (group_command == "CREATE") {
        if (client->in_group()) {
            JsonObject log = JsonObject();
            log["timestamp"] = timestamp();
            log["level"] = "WARN";
            log["event"] = "GROUP_CREATE";
            log["code"] = 402;
            log["response"] = "ERR 402 ALREADY_IN_GROUP";
            log["error"] = "ALREADY_IN_GROUP";
            log["player"] = client->getNickname();
            printJson(log);
            std::cout << std::endl;
            std::string error_msg = "ERR 402 ALREADY_IN_GROUP\r\n";
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        if (rest.size() > 6) {
            std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
            log_error(client, line);
            send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
            return 0;
        }
        client->setGroupId((*group_constant)++);
        std::string creation_msg = (
            "OK group=" +
            std::to_string(client->getGroupId()) + "\r\n"
        );
        JsonObject log = JsonObject();
        log["timestamp"] = timestamp();
        log["level"] = "INFO";
        log["event"] = "GROUP_CREATE";
        log["response"] = "OK group=" + std::to_string(client->getGroupId());
        log["player"] = client->getNickname();
        log["response"] = creation_msg;
        printJson(log);
        std::cout << std::endl;
        send(client->getFd(), creation_msg.c_str(), creation_msg.size(), 0);
    }
    else {
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        log_error(client, line);
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    return 1;
}