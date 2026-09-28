#include "../Client.hpp"
#include "../helpers.hpp"
#include "vector"
#include <algorithm>
#include <unistd.h>

void log_message(Client *client) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "INFO";
    log["event"] = "PLAYER_DISCONNECTED";
    log["response"] = "OK bye";
    log["player"] = client->getNickname();
    log["ip"] = client->getIp();
    log["port"] = client->port;
    printJson(log);
    std::cout << std::endl;
}

int handle_quit(Client *client, std::string line, std::vector<Client> *clients, JsonValue *world, std::vector<pollfd> *fds, int i) {
    if (!client->is_connected) {
        return 0;
    }
    if (line.size() > 4) {
        log_error(client, line);
        std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
        send((*fds)[i].fd, error_msg.c_str(), error_msg.size(), 0);
        return 0;
    }
    std::string room = client->get_current_room();
    std::string name = client->getNickname();
    for (size_t j = 0; j < (*world)["world"]["locations"].size(); j++) {
        if ((*world)["world"]["locations"][j]["name"].getString() == room) {
            for (size_t k = 0; k < (*world)["world"]["locations"][j]["players"].size(); k++) {
                if ((*world)["world"]["locations"][j]["players"][k].getString() == name) {
                    (*world)["world"]["locations"][j]["players"].erase(k);
                    break ;
                }
            }
        }
    }
    log_message(client);
    char quit_message[] = "OK bye\r\n";  
    send((*fds)[i].fd, quit_message, sizeof(quit_message), 0);
    for (std::vector<Client>::iterator it = (*clients).begin(); it != (*clients).end();) {
        if (it->getFd() == (*fds)[i].fd)
        it = (*clients).erase(it);
        else
        ++it;
    }
    close((*fds)[i].fd);
    (*fds).erase((*fds).begin() + i);
    --i;
    for (int index = 0; index < (*clients).size(); index++) {
        std::string event = "EVT STATS players=" + std::to_string((*clients).size()) + "\r\n";
        send((*clients)[index].getFd(), event.c_str(), event.size(), 0);
        
    }
    return 1;
}