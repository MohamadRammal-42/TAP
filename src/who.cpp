#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>

void log_success(Client *client, std::string answer) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "INFO";
    log["event"] = "WHO";
    log["response"] = answer;
    log["player"] = client->getNickname();
    printJson(log);
    std::cout << std::endl;
}

int handle_who(Client *client, std::string line, std::vector<Client> *clients) {
    if (!client->is_connected) {
        log_unauthorized_action(client);
        std::string error_msg = "ERR 710 UNAUTHORIZED_ACTION\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    if (line.size() != 3) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    std::string answer = "OK { \"room\": [";
    for (Client c: (*clients)) {
        if (c.get_current_room() == client->get_current_room()) {
            answer += (+ "\"" + c.getNickname() + "\", ");
        }
    }
    answer.erase(answer.size() - 2);
    answer += "], \"server\": ";
    answer += std::to_string((*clients).size());
    answer += " }\n";
    log_success(client, answer);
    send(client->getFd(), answer.c_str(), answer.size(), 0);
    return 1;
}
