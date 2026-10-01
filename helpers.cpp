#include <iostream>
#include "Client.hpp"
#include "helpers.hpp"
#include "npcs.hpp"
#include <vector>

std::string success = "OK\r\n";

Client* get_client_by_fd(int fd, std::vector<Client>& clients)
{
    for (Client& c : clients) {
        if (fd == c.getFd())
            return &c;
    }

    return nullptr;
}

Client *get_client_by_nickname(std::string nickname, std::vector<Client>& clients)
{
    for (Client& c: clients) {
        if(c.getNickname() == nickname) {
            return &c;
        }
    }
    return nullptr;
}

bool validNickname(const std::string& name)
{
    for (unsigned char c : name) {
        if (!std::isalnum(c))
            return false;
    }
    return !name.empty();
}

void log_error(Client *client, std::string line) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["player"] = client->getNickname();
    log["level"] = "ERROR";
    log["event"] = "INVALID_COMMAND";
    log["response"] = "ERR 911 INVALID_COMMAND";
    log["code"] = 911;
    log["command"] = line;
    printJson(log);
    std::cout << std::endl;
}

void log_warning(Client *client, int code, std::string line, std::string event, std::string response)
{
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["player"] = client->getNickname();
    log["level"] = "WARN";
    log["event"] = event;
    log["response"] = response;
    log["code"] = code;
    log["command"] = line;
    printJson(log);
    std::cout << std::endl;
}


void log_error_ip(Client *client, std::string line) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "ERROR";
    log["event"] = "INVALID_COMMAND";
    log["response"] = "ERR 911 INVALID_COMMAND";
    log["code"] = 911;
    log["command"] = line;
    log["ip"] = client->getIp();
    log["port"] = client->port;
    printJson(log);
    std::cout << std::endl;
}

void log_unauthorized_action(Client *client) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "ERROR";
    log["code"] = 710;
    log["error"] = "UNAUTHORIZED_ACTION";
    log["ip"] = client->getIp();
    log["port"] = client->port;
    printJson(log);
}

void log_connection_warning(Client *client, int code, std::string event, std::string response) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "WARN";
    log["event"] = event;
    log["response"] = response;
    log["code"] = code;
    log["ip"] = client->getIp();
    log["port"] = client->port;
    printJson(log);
    std::cout << std::endl;
}

void log_connection_error(Client *client, int code, std::string event, std::string response) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "ERROR";
    log["event"] = event;
    log["response"] = response;
    log["code"] = code;
    log["ip"] = client->getIp();
    log["port"] = client->port;
    printJson(log);
    std::cout << std::endl;
}

void log_connection_success(Client *client) {
    JsonObject log = JsonObject();
    log["timestamp"] = timestamp();
    log["level"] = "INFO";
    log["event"] = "PLAYER_CONNECTED";
    log["response"] = "OK connected";
    log["player"] = client->getNickname();
    log["ip"] = client->getIp();
    log["port"] = client->port;
    printJson(log);
    std::cout << std::endl;
}

Locations *add_npcs(JsonValue world) {
    Locations *npcs = new Locations();
    JsonObject npc = JsonObject();
    for (int i = 0; i < world["world"]["locations"].size(); i++) {
        JsonValue location = world["world"]["locations"][i];
        npcs->push_back(location["name"].getString());
        for (int j = 0; j < location["spawns"].size(); j++) {
            for (int k = 0; k < location["spawns"][j]["count"].getDouble(); k++) {
                for (int mosafa = 0; mosafa < world["world"]["npcs"]["bosses"].size(); mosafa++) {
                    if(world["world"]["npcs"]["bosses"][mosafa]["name"].getString() == location["spawns"][j]["npc_type"].getString()) {
                        npc["npc"] = world["world"]["npcs"]["bosses"][mosafa];
                        Boss *n = new Boss(
                            npc["npc"]["name"].getString(),
                            npc["npc"]["stats"]["hp"].getDouble(),
                            npc["npc"]["stats"]["damage"].getDouble(),
                            npc["npc"]["drops"]["coins"].getDouble(),
                            npc["npc"]["drops"]["exp"].getDouble()
                        );
                        n->drops["drops"] = npc["npc"]["drops"];
                        npcs->tail->npcs->push_back(n);
                        break;
                    }
                }
                for (int mosafa = 0; mosafa < world["world"]["npcs"]["traders"].size(); mosafa++) 
                {
                    if (world["world"]["npcs"]["traders"][mosafa]["name"].getString() == location["spawns"][j]["npc_type"].getString())
                    {
                        npc["npc"] = world["world"]["npcs"]["traders"][mosafa];
                        Trader *t = new Trader(
                            npc["npc"]["name"].getString(),
                            npc["npc"]["stats"]["hp"].getDouble()
                        );
                        npcs->tail->npcs->push_back(t);
                        break;
                    }
                }
            }
        }
    }
    return npcs;
}