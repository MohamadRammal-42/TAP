#include <poll.h>
#include <random>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include "npcs.hpp"
#include <iostream>
#include <unistd.h>
#include <stdexcept>
#include <algorithm>
#include "helpers.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "json_parser.hpp"
#include <chrono>
#include <cstdio>

std::string timestamp()
{
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);

    char buffer[25];

    strftime(buffer, sizeof(buffer),
             "%Y-%m-%dT%H:%M:%SZ", t);

    return std::string(buffer);
}

JsonValue get_world() {
    std::ifstream file("world.json");
    if (!file.is_open()) {
        std::cerr << "Could not open world.json\n";
        return JsonValue(false);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    try {
        JsonParser parser(json);
        JsonValue root = parser.parse();
        return root;
    }
    catch (const std::exception& e) {
        std::cerr << "JSON error: " << e.what() << '\n';
        return JsonValue(false);
    }
}

void clear_lag(JsonValue& json, JsonValue& world) {
    for (int i = 0; i < world["world"]["locations"].size(); i++) {
        world["world"]["locations"][i]["items"] = json["world"]["locations"][i]["items"];
    }
}

int main() {
    JsonValue world = get_world();
    if (!parseworld(world))
        return 0;
    JsonValue world_2 = get_world();
    std::random_device rd;
    std::mt19937 gen(rd());
    Locations *npcs = add_npcs(world);
    std::vector<Client> clients;
    std::vector<Client> candidate_clients;
    std::vector<pollfd> fds;
    time_t clear_lag_timer = time(0);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int group_constant = 42;

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(6667);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    pollfd server_pollfd{};
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;

    fds.push_back(server_pollfd);

    while (true) {
        if(time(0) - clear_lag_timer - 120 >= 0) {
            clear_lag_timer = time(0);
            clear_lag(world_2, world);
        }
        int result = poll(fds.data(), fds.size(), -1);

        if (result < 0) {
            perror("poll");
            break;
        }
        for (size_t i = 0; i < fds.size(); ++i) {
            if (!(fds[i].revents & POLLIN))
                continue;

            if (fds[i].fd == server_fd) {
                sockaddr_in client_address{};
                socklen_t client_len = sizeof(client_address);
                int client_fd = accept(
                    server_fd,
                    (sockaddr*)&client_address,
                    &client_len
                );
                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(
                    AF_INET,
                    &client_address.sin_addr,
                    client_ip,
                    sizeof(client_ip)
                );
                int port = ntohs(client_address.sin_port);
                const char* message = "OK hello proto=1\r\n";
                send(client_fd, message, strlen(message), 0);
                Client c = Client(client_fd, "");
                c.setIp(client_ip);
                c.port = port;
                candidate_clients.push_back(c);
                pollfd client_pollfd{};
                client_pollfd.fd = client_fd;
                client_pollfd.events = POLLIN;
                fds.push_back(client_pollfd);
            }
            else {
                Client *client = get_client_by_fd(fds[i].fd, clients);
                if (!client)
                    client = get_client_by_fd(fds[i].fd, candidate_clients);

                char buffer[512];

                ssize_t bytes = recv(
                    (*client).getFd(),
                    buffer,
                    sizeof(buffer),
                    0
                );
                if (bytes > 0) {
                    (*client).getBuffer().append(buffer, bytes);

                    std::size_t pos;
                    while ((pos = (*client).getBuffer().find('\n')) != std::string::npos) {
                        std::string line = (*client).getBuffer().substr(0, pos);
                        (*client).getBuffer().erase(0, pos + 1);

                        if (!line.empty() && line.back() == '\r')
                            line.pop_back();

                        if (line.substr(0, 7) == "CONNECT") {
                            if (line.size() == 7) {
                                log_error_ip(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            if (line.substr(0, 8) == "CONNECT ") {
                                if (line.size() <= 8) {
                                    log_error_ip(client, line);
                                    std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                    send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                    break;
                                }
                            }
                        }
                        if (bytes > 8 && (line.substr(0, 8) == "CONNECT ")) {
                            if (!handle_connect(client, line, &clients, &candidate_clients, &world))
                                break;
                        }
                        else if (line.substr(0, 4) == "QUIT") {
                            if (!handle_quit(client, line, &clients, &world, &fds, i))
                                break;
                        }
                        else if (line.substr(0, 5) == "GROUP") {
                            if (!handle_group(client, line, &clients, &group_constant))
                                break;
                        }
                        else if (line.substr(0, 4) == "CHAT") {
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
                                break;
                            }
                            if (line.size() <= 5) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            
                            std::string rest = line.substr(5);
                            int pos = rest.find(" ");
                            std::string chat_command = rest.substr(0, pos);
                            if (chat_command == "GLOBAL") {
                                std::string message = "EVT GLOBAL CHAT " + client->getNickname() + ": "+ rest.substr(pos + 1) + "\r\n";
                                for (Client c: clients) {
                                    if (c.getFd() != client->getFd())
                                        send(c.getFd(), message.c_str(), message.size(), 0);
                                }
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "INFO";
                                log["event"] = "CHAT_GLOBAL";
                                log["player"] = client->getNickname();
                                log["message"] = message;
                                log["response"] = "OK";
                                printJson(log);
                                std::cout << std::endl;
                                send(client->getFd(), "OK\r\n", 4, 0);
                                break;
                            }
                            else if (chat_command == "ROOM") {
                                std::string message = "EVT ROOM CHAT " + client->getNickname() + ": "+ rest.substr(pos + 1) + "\r\n";
                                for (Client c: clients) {
                                    if (c.get_current_room() == client->get_current_room() &&
                                        c.getFd() != client->getFd()
                                    )
                                        send(c.getFd(), message.c_str(), message.size(), 0);
                                }
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "INFO";
                                log["event"] = "CHAT_ROOM";
                                log["player"] = client->getNickname();
                                log["message"] = message;
                                log["response"] = "OK";
                                printJson(log);
                                std::cout << std::endl;
                                send(client->getFd(), "OK\r\n", 4, 0);
                                break;
                            }
                            else if (chat_command == "GROUP") {
                                if (!client->in_group()) {
                                    JsonObject log = JsonObject();
                                    log["timestamp"] = timestamp();
                                    log["level"] = "WARN";
                                    log["event"] = "CHAT_GROUP";
                                    log["code"] = 401;
                                    log["response"] = "ERR 401 NOT_IN_GROUP";
                                    log["error"] = "NOT_IN_GROUP";
                                    log["player"] = client->getNickname();
                                    printJson(log);
                                    std::cout << std::endl;
                                    std::string error_msg = "ERR 401 NOT_IN_GROUP\r\n";
                                    send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
                                    break;
                                }
                                std::string message = "EVT GROUP CHAT " +  client->getNickname() + ": "+ rest.substr(pos + 1) + "\r\n";
                                for (Client c: clients) {
                                    if (c.getGroupId() == client->getGroupId() &&
                                        c.getFd() != client->getFd()
                                    )
                                        send(c.getFd(), message.c_str(), message.size(), 0);
                                }
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "INFO";
                                log["event"] = "CHAT_GROUP";
                                log["player"] = client->getNickname();
                                log["message"] = message;
                                log["response"] = "OK";
                                printJson(log);
                                std::cout << std::endl;
                                send(client->getFd(), "OK\r\n", 4, 0);
                                break;
                            }
                        }
                        else if (line.substr(0, 3) == "WHO") {
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
                                break;
                            }
                            if (line.size() != 3) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            std::string answer = "OK { \"room\": [";
                            for (Client c: clients) {
                                if (c.get_current_room() == client->get_current_room()) {
                                    answer += (+ "\"" + c.getNickname() + "\", ");
                                }
                            }
                            answer.erase(answer.size() - 2);
                            answer += "], \"server\": ";
                            answer += std::to_string(clients.size());
                            answer += " }\n";
                            JsonObject log = JsonObject();
                            log["timestamp"] = timestamp();
                            log["level"] = "INFO";
                            log["event"] = "WHO";
                            log["response"] = answer;
                            log["player"] = client->getNickname();
                            printJson(log);
                            send(fds[i].fd, answer.c_str(), answer.size(), 0);
                        }
                        else if (line.substr(0, 4) == "LOOK") {
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
                                break;
                            }
                            if (line.size() != 4) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            JsonValue room = JsonValue(nullptr);
                            for (int i = 0; i < world["world"]["locations"].size(); i++) {
                                if (world["world"]["locations"][i]["name"].getString() == client->get_current_room())
                                {
                                    room = world["world"]["locations"][i];
                                    break;
                                }
                            }
                            if (!room.isNull()) {
                                std::string full_json = "OK " + ReturnJson(room);
                                full_json += "\r\n";
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "INFO";
                                log["event"] = "LOOK";
                                log["response"] = "OK";
                                log["data"] = room;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                send(fds[i].fd, full_json.c_str(), full_json.size(), 0);
                            }
                        }
                        else if (line.substr(0, 4) == "MOVE") {
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
                                break;
                            }
                            if (line.size() <= 5) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            std::string move_direction = line.substr(5);
                            std::string current_room = client->get_current_room();
                            JsonValue& locations = world["world"]["locations"];
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
                                break;
                            }
                            JsonValue& exits = (*currentLocationData)["exits"]; 
                            if (!exits.contains(move_direction)) { 
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "WARN";
                                log["event"] = "NO_EXIT";
                                log["response"] = "ERR 301 NO_EXIT\r\n";
                                log["code"] = 301;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string error_msg = "ERR 301 NO_EXIT\r\n";
                                send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
                                break;
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
                                    int fd = get_client_by_nickname((*currentLocationData)["players"][k].getString(), clients)->getFd();
                                    std::string message = "EVT ROOM PRESENCE LEAVE "+ name + "\r\n";
                                    send(fd, message.c_str(), message.size(), 0);
                                }
                            }
                            for (size_t j = 0; j < locations.size(); ++j) {
                                if (locations[j]["name"].getString() == destination) {
                                    for (int player = 0; player < locations[j]["players"].size(); player++) {
                                        int fd = get_client_by_nickname(locations[j]["players"][player].getString(), clients)->getFd();
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
                        }
                        else if (line.substr(0, 9) == "INVENTORY") {
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
                                break;
                            }
                            if (line.size() != 9) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
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
                        }
                        else if (line.substr(0, 4) == "TAKE") {
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
                                break;
                            }
                            if (line.size() <= 5) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            std::string item = line.substr(5);
                            JsonValue& locations = world["world"]["locations"];
                            JsonValue* current_location = nullptr;
                            bool is_found = false;
                            for (size_t j = 0; j < locations.size(); j++) {
                                if (locations[j]["name"].getString() == client->get_current_room())
                                {
                                    current_location = &(locations[j]);
                                    for (size_t k = 0; k < (*current_location)["items"].size(); k++) {
                                        if (item == (*current_location)["items"][k].getString()) {
                                            client->inventory.push_back((*current_location)["items"][k].getString());
                                            (*current_location)["items"].erase(k);
                                            is_found = true;
                                            std::string message = "OK taken=items." + item;
                                            JsonObject log = JsonObject();
                                            log["timestamp"] = timestamp();
                                            log["level"] = "INFO";
                                            log["event"] = line;
                                            log["response"] = message;
                                            log["player"] = client->getNickname();
                                            printJson(log);
                                            std::cout << std::endl;
                                            message += "\r\n";
                                            send(fds[i].fd, message.c_str(), message.size(), 0);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!is_found) {
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "WARN";
                                log["event"] = "ITEM_NOT_FOUND";
                                log["response"] = "ERR 404 ITEM_NOT_FOUND";
                                log["code"] = 404;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string error_msg = "ERR 404 ITEM_NOT_FOUND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                is_found = false;
                            }
                            if (world["world"]["items"]["weapons"].size() > 0 && item == world["world"]["items"]["weapons"][0]["name"].getString()) {
                                system("paplay assets/ultimate_weapon.wav");
                            }
                        }
                        else if (line.substr(0, 4) == "DROP") {
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
                                break;
                            }
                            if (line.size() <= 5) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            std::string item = line.substr(5);
                            JsonValue& locations = world["world"]["locations"];
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
                                    JsonObject log = JsonObject();
                                    log["timestamp"] = timestamp();
                                    log["level"] = "INFO";
                                    log["event"] = line;
                                    log["response"] = message;
                                    log["player"] = client->getNickname();
                                    printJson(log);
                                    std::cout << std::endl;
                                    message += "\r\n";
                                    send(fds[i].fd, message.c_str(), message.size(), 0);
                                    break;
                                }
                            }
                            if (!is_found) {
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "WARN";
                                log["event"] = "ITEM_NOT_FOUND";
                                log["response"] = "ERR 404 ITEM_NOT_FOUND";
                                log["code"] = 404;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string error_msg = "ERR 404 ITEM_NOT_FOUND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                is_found = false;
                            }
                        }
                        else if (line.substr(0, 6) == "STATUS") {
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
                                break;
                            }
                            if (line.size() != 6) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            std::string reply = "OK ";
                            JsonValue stats = JsonValue(JsonObject());
                            stats["hp"] = client->getHp();
                            stats["max_hp"] = client->getMaxHp();
                            stats["attack_power"] = client->getAttackPower() * client->attack_multiplier;
                            stats["defense_ratio"] = client->defense_ratio;
                            stats["coins"] = client->getCoins();
                            stats["level"] = client->getLevel();
                            stats["xp"] = client->getXp();
                            reply += ReturnJson(stats);
                            reply += "\r\n";
                            send(fds[i].fd, reply.c_str(), reply.size(), 0);
                        }
                        else if (line.substr(0, 6) == "ATTACK") {
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
                                break;
                            }
                            if (line.size() <= 7) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            std::string npc_name = line.substr(7);
                            JsonValue& locations = world["world"]["locations"];
                            std::string current_location;
                            for (size_t j = 0; j < locations.size(); j++) {
                                if (locations[j]["name"].getString() == client->get_current_room()) {
                                    current_location = locations[j]["name"].getString();
                                    break;
                                }
                            }
                            LocationNpcsNode *tmp = npcs->head;
                            while (tmp && tmp->location != current_location) {
                                tmp = tmp->next;
                            }
                            Node *npc_node = tmp->npcs->head;
                            while (npc_node) {
                                if (npc_node->npc->getNickname() == npc_name && npc_node->npc->timer <= time(0))
                                    break;
                                npc_node = npc_node->next;
                            }
                            if (!npc_node) {
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "WARN";
                                log["event"] = "NPC_NOT_FOUND";
                                log["response"] = "ERR 404 NPC_NOT_FOUND";
                                log["code"] = 404;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string error_msg = "ERR 404 NPC_NOT_FOUND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            npc_node->npc->setHp(npc_node->npc->getHp() - (client->getAttackPower() * client->attack_multiplier));
                            if (npc_node->npc->getHp() > 0) {
                                if (dynamic_cast<Boss*>(npc_node->npc)) {
                                    Boss* boss = dynamic_cast<Boss*>(npc_node->npc);
                                    client->setHp(client->getHp() - (boss->getAttackPower() - (boss->getAttackPower() * client->defense_ratio) / 100));
                                    if (static_cast<int>(client->defense_ratio) != 0) {
                                        std::uniform_int_distribution<> distrib(1, static_cast<int>(client->armor_durability));
                                        if (static_cast<int>(distrib(gen)) == 1) {
                                            client->current_armor = "";
                                            client->defense_ratio = 0;
                                            system("paplay assets/armor_break.wav");
                                        }
                                    }
                                }
                                if (client->getHp() > 0)
                                {
                                    JsonObject combat_status = JsonObject{};
                                    combat_status["attacker_hp"] = client->getHp();
                                    combat_status["target_hp"] = npc_node->npc->getHp();
                                    combat_status["damage"] = client->getAttackPower() * client->attack_multiplier;
                                    combat_status["status"] = "combat";
                                    std::string status = ReturnJson(combat_status);
                                    JsonObject log = JsonObject();
                                    log["timestamp"] = timestamp();
                                    log["level"] = "INFO";
                                    log["event"] = line;
                                    log["response"] = "OK";
                                    log["data"] = combat_status;
                                    log["player"] = client->getNickname();
                                    printJson(log);
                                    std::cout << std::endl;
                                    status += "\r\n";
                                    send(fds[i].fd, (success + status).c_str(), (success + status).size() + 1, 0);
                                }
                                else {
                                    std::string death_message = "You were eliminated by " + npc_node->npc->getNickname();
                                    client->setHp(client->getMaxHp() / 2);
                                    client->set_current_room(world["world"]["locations"][0]["name"].getString());
                                    std::cout << client->getAttackPower() << std::endl;
                                    JsonObject log = JsonObject();
                                    log["timestamp"] = timestamp();
                                    log["level"] = "INFO";
                                    log["event"] = line;
                                    log["response"] = death_message;
                                    log["player"] = client->getNickname();
                                    printJson(log);
                                    std::cout << std::endl;
                                    death_message += "\r\n";
                                    send(fds[i].fd, death_message.c_str(), death_message.size(), 0);
                                    break;
                                }
                            }
                            else {
                                std::string elimination_message = "You killed " + npc_node->npc->getNickname() + "\n";
                                if (dynamic_cast<Boss*>(npc_node->npc))
                                {

                                    elimination_message += "You gained " + ReturnJson(npc_node->npc->drops["drops"]);
                                    client->setCoins(client->getCoins() + npc_node->npc->drops["drops"]["coins"].getDouble());
                                    client->setXp(client->getXp() + npc_node->npc->drops["drops"]["exp"].getDouble());
                                    JsonObject& drops = std::get<JsonObject>(
                                        npc_node->npc->drops["drops"].value
                                    );
                                    if (drops.find("weapon") != drops.end()){
                                        for(int loc = 0; loc < world["world"]["locations"].size(); loc++) {
                                            if(world["world"]["locations"][loc]["name"].getString() == client->get_current_room()) {
                                                world["world"]["locations"][loc]["items"].push_back(drops["weapon"]);
                                            }
                                        }
                                    }
                                    if (drops.find("consumable") != drops.end()){
                                        for(int loc = 0; loc < world["world"]["locations"].size(); loc++) {
                                            if(world["world"]["locations"][loc]["name"].getString() == client->get_current_room()) {
                                                world["world"]["locations"][loc]["items"].push_back(drops["consumable"]);
                                            }
                                        }
                                    }
                                }
                                for (int index = 0; index < client->quests.size(); index++) {
                                    if (client->quests[index]["type"].getString() == "eliminate" && client->quests[index]["target"].getString() == npc_node->npc->getNickname()) {
                                        client->quests[index]["progress"] = client->quests[index]["progress"].getDouble() + 1;
                                        JsonObject log = JsonObject();
                                        log["timestamp"] = timestamp();
                                        log["level"] = "INFO";
                                        log["event"] = "QUEST_PROGRESS";
                                        log["progress"] = client->quests[index]["progress"].getDouble();
                                        log["target"] = client->quests[index]["required"].getDouble();
                                        log["player"] = client->getNickname();
                                        printJson(log);
                                        std::cout << std::endl;
                                    }
                                }
                                if (client->getXp() >= 50 * client->lvlup_buffer) {
                                    client->setXp(client->getXp() - (50 * client->lvlup_buffer));
                                    client->setLevel(client->getLevel() + 1);
                                    client->setAttackPower(client->getAttackPower() + 2);
                                    client->setMaxHp(client->getMaxHp() + 20);
                                    client->setHp(client->getMaxHp());
                                    client->lvlup_buffer += 0.2;
                                    JsonObject log = JsonObject();
                                    log["timestamp"] = timestamp();
                                    log["level"] = "INFO";
                                    log["event"] = "LEVEL_UP";
                                    log["new_level"] = client->getLevel();
                                    log["player"] = client->getNickname();
                                    printJson(log);
                                }
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "INFO";
                                log["event"] = line;
                                log["response"] = elimination_message;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                elimination_message += "\r\n";
                                send(fds[i].fd, elimination_message.c_str(), elimination_message.size(), 0);
                                npc_node->npc->timer = time(nullptr) + 60;
                                npc_node->npc->setHp(npc_node->npc->getMaxHp());
                                if (npc_node != tmp->npcs->tail) {
                                    if (npc_node->prev)
                                        npc_node->prev->next = npc_node->next;
                                    else
                                        tmp->npcs->head = npc_node->next;
                                    if (npc_node->next)
                                        npc_node->next->prev = npc_node->prev;
                                    npc_node->prev = tmp->npcs->tail;
                                    npc_node->next = nullptr;

                                    if (tmp->npcs->tail)
                                        tmp->npcs->tail->next = npc_node;
                                    else
                                        tmp->npcs->head = npc_node;

                                    tmp->npcs->tail = npc_node;
                                }
                            
                            }
                            if (static_cast<double>(client->attack_multiplier) != 1) {
                                std::uniform_int_distribution<> distrib(1, static_cast<int>(client->weapon_durability));
                                if (static_cast<int>(distrib(gen)) == 1) {
                                    client->current_weapon = "";
                                    client->attack_multiplier = 1;
                                    system("paplay assets/weapon_break.wav");
                                }
                            }
                        }
                        else if (line.substr(0, 3) == "USE") {
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
                                break;
                            }
                            if (line.size() <= 4) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }

                            std::string item = line.substr(4);
                            bool item_found = false;
                            for (std::string i: client->inventory) {
                                if (i == item) {
                                    item_found = true;
                                    for (int j = 0; j < world["world"]["items"]["consumables"].size(); j++) {
                                        if (item == world["world"]["items"]["consumables"][j]["name"].getString())
                                        {
                                            client->setHp(
                                                ((client->getHp() + world["world"]["items"]["consumables"][j]["heal"].getDouble()) < client->getMaxHp())
                                                ? client->getHp() + world["world"]["items"]["consumables"][j]["heal"].getDouble()
                                                : client->getMaxHp()

                                            );
                                            for (int index = 0; index < client->inventory.size(); index++) {
                                                if (client->inventory[index] == item) {
                                                    client->inventory.erase(client->inventory.begin() + index);
                                                    break;
                                                }
                                            }
                                            system("paplay assets/eating.wav");
                                            JsonObject log = JsonObject();
                                            log["timestamp"] = timestamp();
                                            log["level"] = "INFO";
                                            log["event"] = line;
                                            log["response"] = "OK";
                                            log["player"] = client->getNickname();
                                            printJson(log);
                                            std::cout << std::endl;
                                            send(client->getFd(), success.c_str(), success.size(), 0);
                                        }
                                    }
                                    for (int j = 0; j < world["world"]["items"]["weapons"].size(); j++) {
                                        if (item == world["world"]["items"]["weapons"][j]["name"].getString())
                                        {
                                            if (client->current_weapon != "") {
                                                client->inventory.push_back(client->current_weapon);
                                            }
                                            client->current_weapon = world["world"]["items"]["weapons"][j]["name"].getString();
                                            client->weapon_durability = world["world"]["items"]["weapons"][j]["durability"].getDouble();
                                            client->attack_multiplier = world["world"]["items"]["weapons"][j]["attack"].getDouble();
                                            for (int index = 0; index < client->inventory.size(); index++) {
                                                if (client->inventory[index] == item) {
                                                    client->inventory.erase(client->inventory.begin() + index);
                                                    break;
                                                }
                                            }
                                            JsonObject log = JsonObject();
                                            log["timestamp"] = timestamp();
                                            log["level"] = "INFO";
                                            log["event"] = line;
                                            log["response"] = "OK";
                                            log["player"] = client->getNickname();
                                            printJson(log);
                                            std::cout << std::endl;
                                            send(client->getFd(), success.c_str(), success.size(), 0);
                                        }
                                    }
                                    for (int j = 0; j < world["world"]["items"]["armors"].size(); j++) {
                                        if (item == world["world"]["items"]["armors"][j]["name"].getString())
                                        {
                                            if (client->current_armor != "") {
                                                client->inventory.push_back(client->current_armor);
                                            }
                                            client->current_armor = world["world"]["items"]["armors"][j]["name"].getString();
                                            client->armor_durability = world["world"]["items"]["armors"][j]["durability"].getDouble();
                                            client->defense_ratio = world["world"]["items"]["armors"][j]["defense"].getDouble();
                                            for (int index = 0; index < client->inventory.size(); index++) {
                                                if (client->inventory[index] == item) {
                                                    client->inventory.erase(client->inventory.begin() + index);
                                                    break;
                                                }
                                            }
                                            JsonObject log = JsonObject();
                                            log["timestamp"] = timestamp();
                                            log["level"] = "INFO";
                                            log["event"] = line;
                                            log["response"] = "OK";
                                            log["player"] = client->getNickname();
                                            printJson(log);
                                            std::cout << std::endl;
                                            send(client->getFd(), success.c_str(), success.size(), 0);
                                        }
                                    }
                                    break;
                                }
                            }
                            if (!item_found) {
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "ERROR";
                                log["event"] = "ITEM_NOT_FOUND";
                                log["response"] = "ERR 404 ITEM_NOT_FOUND";
                                log["code"] = 404;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string fail = "ERR 404 ITEM_NOT_FOUND\r\n";
                                send(client->getFd(), fail.c_str(), fail.size(), 0);
                                break;
                            }
                        }
                        else if (line.substr(0, 5) == "UNUSE") {
                            if (!client->is_connected) {
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "ERROR";
                                log["event"] = "UNAUTHORIZED_ACTION";
                                log["response"] = "ERR 710 UNAUTHORIZED_ACTION";
                                log["code"] = 710;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string error_msg = "ERR 710 UNAUTHORIZED_ACTION\r\n";
                                send(client->getFd(), error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            if (line.size() <= 6) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            std::string item = line.substr(6);
                            bool item_found = false;
                            if (client->current_weapon != "" && client->current_weapon == item) {
                                item_found = true;
                                client->inventory.push_back(client->current_weapon);
                                client->current_weapon = "";
                                client->attack_multiplier = 1;
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "INFO";
                                log["event"] = line;
                                log["response"] = "OK";
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                send(client->getFd(), success.c_str(), success.size(), 0);
                            }
                            else if (client->current_armor != "" && client->current_armor == item) {
                                item_found = true; 
                                client->inventory.push_back(client->current_armor);
                                client->current_armor = "";
                                client->defense_ratio = 0;
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "INFO";
                                log["event"] = line;
                                log["response"] = "OK";
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                send(client->getFd(), success.c_str(), success.size(), 0);       
                            }
                            if (!item_found) {
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "WARN";
                                log["event"] = "ITEM_NOT_FOUND";
                                log["response"] = "ERR 404 ITEM_NOT_FOUND";
                                log["code"] = 404;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string fail = "ERR 404 ITEM_NOT_FOUND\r\n";
                                send(client->getFd(), fail.c_str(), fail.size(), 0);
                                break;
                            }
                        }
                        else if (line.substr(0, 6) == "QUESTS") {
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
                                break;
                            }
                            if (line.size() != 6) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            JsonObject log = JsonObject();
                            log["timestamp"] = timestamp();
                            log["level"] = "INFO";
                            log["event"] = "LOOK";
                            log["response"] = "OK";
                            log["data"] = client->quests;
                            log["player"] = client->getNickname();
                            printJson(log);
                            std::cout << std::endl;
                            std::string client_quests = "OK " +  ReturnJson(client->quests) + "\r\n";
                            send(client->getFd(), client_quests.c_str(), client_quests.size(), 0);
                        }
                        else if (line.substr(0, 4) == "TALK") {
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
                                break;
                            }
                            if (line.size() <= 5) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            bool npc_found = false;
                            std::string npc_name = line.substr(5);
                            for (int index = 0; index < world["world"]["locations"].size(); index++) {
                                if (client->get_current_room() == world["world"]["locations"][index]["name"].getString()) {
                                    for (int mosafa = 0; mosafa < world["world"]["locations"][index]["spawns"].size(); mosafa++) {
                                        if (world["world"]["locations"][index]["spawns"][mosafa]["npc_type"].getString() == npc_name) {
                                            npc_found = true;
                                            break;
                                        }
                                    }
                                }
                                if (npc_found)
                                    break;
                            }
                            bool quest = false;
                            if (npc_found) {
                                for (int index = 0; index < world["world"]["npcs"]["traders"].size(); index++) {
                                    if (world["world"]["npcs"]["traders"][index]["name"].getString() == npc_name) {
                                        for (int j = 0; j < client->quests.size(); j++) {
                                            if (client->quests[j]["provider"].getString() == npc_name && client->quests[j]["type"].getString() == "eliminate") {
                                                if (client->quests[j]["progress"].getDouble() >= client->quests[j]["required"].getDouble()) {
                                                    quest = true;
                                                    JsonObject& rewards = std::get<JsonObject>(world["world"]["npcs"]["traders"][index]["rewards"].value);
                                                    std::string drops[3] = {"weapon", "armor", "consumable"};
                                                    for (int k = 0; k < 3; k++) {
                                                        auto v = rewards.find(drops[k]);
                                                        if (v != rewards.end()) {
                                                            client->inventory.push_back(v->second.getString());
                                                        }
                                                    }
                                                    auto v_coins = rewards.find("coins");
                                                    if (v_coins != rewards.end()) {
                                                        client->setCoins(client->getCoins() + v_coins->second.getDouble());
                                                    }
                                                    auto v_exp = rewards.find("exp");
                                                    if (v_exp != rewards.end()) {
                                                        client->setXp(client->getXp() + v_exp->second.getDouble());
                                                    }
                                                    std::string message = ("OK " +
                                                        world["world"]["npcs"]["traders"][index]["qc_dialogue"].getString() + "\n"
                                                        + ReturnJson(world["world"]["npcs"]["traders"][index]["rewards"])
                                                    );
                                                    client->quests.erase(client->quests.begin() + j);
                                                    JsonObject log = JsonObject();
                                                    log["timestamp"] = timestamp();
                                                    log["level"] = "INFO";
                                                    log["event"] = line;
                                                    log["response"] = message;
                                                    log["player"] = client->getNickname();
                                                    printJson(log);
                                                    std::cout << std::endl;
                                                    message += "\r\n";
                                                    send(client->getFd(), message.c_str(), message.size(), 0);
                                                }
                                            }
                                            else if (client->quests[j]["provider"].getString() == npc_name && client->quests[j]["type"].getString() == "fetch") {
                                                if (client->quests[j]["target"].getString() != "coins") {
                                                    int count_obj = 0;
                                                    for (int k = 0; k < client->inventory.size(); k++) {
                                                        if (client->inventory[k] == world["world"]["npcs"]["traders"][index]["target"].getString()) {
                                                            count_obj += 1;
                                                        }
                                                    }
                                                    if (count_obj >= client->quests[j]["count"].getDouble()) {
                                                        std::string message = ("OK " +
                                                            world["world"]["npcs"]["traders"][index]["qc_dialogue"].getString() + "\n"
                                                            + ReturnJson(world["world"]["npcs"]["traders"][index]["rewards"])
                                                            + "\r\n"
                                                        );
                                                        quest = true;
                                                    }
                                                }
                                                else if (client->quests[j]["target"].getString() == "coins") {
                                                    if (client->getCoins() >= client->quests[j]["required"].getDouble()) {
                                                        std::string message = ("OK " +
                                                            world["world"]["npcs"]["traders"][index]["qc_dialogue"].getString() + "\n"
                                                            + ReturnJson(world["world"]["npcs"]["traders"][index]["rewards"])
                                                            + "\r\n"
                                                        );
                                                        client->setCoins(client->getCoins() - client->quests[j]["required"].getDouble());
                                                        quest = true;
                                                    }
                                                }
                                                if (quest)
                                                {

                                                    JsonObject& rewards = std::get<JsonObject>(world["world"]["npcs"]["traders"][index]["rewards"].value);
                                                    std::string drops[3] = {"weapon", "armor", "consumable"};
                                                    for (int k = 0; k < 3; k++) {
                                                        auto v = rewards.find(drops[k]);
                                                        if (v != rewards.end()) {
                                                            client->inventory.push_back(v->second.getString());
                                                        }
                                                    }
                                                    auto v_coins = rewards.find("coins");
                                                    if (v_coins != rewards.end()) {
                                                        client->setCoins(client->getCoins() + v_coins->second.getDouble());
                                                    }
                                                    auto v_exp = rewards.find("exp");
                                                    if (v_exp != rewards.end()) {
                                                        client->setXp(client->getXp() + v_exp->second.getDouble());
                                                    }
                                                    std::string message = ("OK " +
                                                        world["world"]["npcs"]["traders"][index]["qc_dialogue"].getString() + "\n"
                                                        + ReturnJson(world["world"]["npcs"]["traders"][index]["rewards"])
                                                    );
                                                    client->quests.erase(client->quests.begin() + j);
                                                    JsonObject log = JsonObject();
                                                    log["timestamp"] = timestamp();
                                                    log["level"] = "INFO";
                                                    log["event"] = line;
                                                    log["response"] = message;
                                                    log["player"] = client->getNickname();
                                                    printJson(log);
                                                    std::cout << std::endl;
                                                    message += "\r\n";
                                                    send(client->getFd(), message.c_str(), message.size(), 0);
                                                }
                                            }
                                            
                                        }
                                        if (!quest) {
                                            std::string message = "OK " + world["world"]["npcs"]["traders"][index]["dialogue"].getString() + "\r\n";
                                            JsonObject log = JsonObject();
                                            log["timestamp"] = timestamp();
                                            log["level"] = "INFO";
                                            log["event"] = line;
                                            log["response"] = message;
                                            log["player"] = client->getNickname();
                                            printJson(log);
                                            std::cout << std::endl;
                                            send(client->getFd(), message.c_str(), message.size(), 0);

                                        }
                                    }
                                }
                            }
                            else {
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "WARN";
                                log["event"] = "NPC_NOT_FOUND";
                                log["response"] = "ERR 404 NPC_NOT_FOUND";
                                log["code"] = 404;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string error_msg = "ERR 404 NPC_NOT_FOUND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                        }
                        else if (line.substr(0, 5) == "QUEST") {
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
                                break;
                            }
                            if (line.size() <= 6) {
                                log_error(client, line);
                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                            std::string npc_name = line.substr(6);
                            bool npc_found = false;
                            for (int index = 0; index < world["world"]["locations"].size(); index++) {
                                if (client->get_current_room() == world["world"]["locations"][index]["name"].getString()) {
                                    for (int mosafa = 0; mosafa < world["world"]["locations"][index]["spawns"].size(); mosafa++) {
                                        if (world["world"]["locations"][index]["spawns"][mosafa]["npc_type"].getString() == npc_name) {
                                            npc_found = true;
                                        }
                                    }
                                }
                            }
                            if (npc_found) {
                                for (int index = 0; index < world["world"]["npcs"]["traders"].size(); index++) {
                                    if (world["world"]["npcs"]["traders"][index]["name"].getString() == npc_name) {
                                        JsonObject quest = JsonObject();
                                        JsonObject q = JsonObject();
                                        if (world["world"]["npcs"]["traders"][index]["quest"]["status"].getString() == "available") {
                                            quest["quest_id"] = world["world"]["npcs"]["traders"][index]["quest"]["type"].getString() + "_" + world["world"]["npcs"]["traders"][index]["quest"]["target"].getString();
                                            quest["description"] = (
                                                world["world"]["npcs"]["traders"][index]["quest"]["type"].getString() + " " +
                                                std::to_string((int)world["world"]["npcs"]["traders"][index]["quest"]["count"].getDouble()) + " " +
                                                world["world"]["npcs"]["traders"][index]["quest"]["target"].getString()
                                            );
                                            q["type"] = world["world"]["npcs"]["traders"][index]["quest"]["type"].getString();
                                            q["target"] = world["world"]["npcs"]["traders"][index]["quest"]["target"].getString();
                                            q["required"] = world["world"]["npcs"]["traders"][index]["quest"]["count"].getDouble();
                                            q["quest_id"] = quest["quest_id"];
                                            q["provider"] = world["world"]["npcs"]["traders"][index]["name"].getString();
                                            if (q["type"].getString() == "eliminate")
                                                q["progress"] = 0.0;
                                            else
                                            {
                                                std::string target = q["target"].getString();
                                                double progress = 0;
                                                if (target == "coins") {
                                                    progress = client->getCoins();
                                                }
                                                else {
                                                    for (int k = 0; k < client->inventory.size(); k++) {
                                                        if (client->inventory[k] == target) {
                                                            progress += 1;
                                                        }
                                                    }
                                                }
                                                q["progress"] = progress;
                                            }
                                            std::cout << std::endl;
                                            quest["rewards"] = world["world"]["npcs"]["traders"][index]["rewards"];
                                            client->quests.push_back(q);
                                            world["world"]["npcs"]["traders"][index]["quest"]["status"] = "unavailable";
                                            std::string quest_message = "OK " + ReturnJson(quest) + "\r\n";
                                            JsonObject log = JsonObject();
                                            log["timestamp"] = timestamp();
                                            log["level"] = "INFO";
                                            log["event"] = line;
                                            log["response"] = "OK";
                                            log["data"] = quest;
                                            log["player"] = client->getNickname();
                                            printJson(log);
                                            std::cout << std::endl;
                                            send(client->getFd(), quest_message.c_str(), quest_message.size(), 0);
                                        }
                                        else {
                                            JsonObject log = JsonObject();
                                            log["timestamp"] = timestamp();
                                            log["level"] = "ERROR";
                                            log["event"] = "NO_QUEST_AVAILABLE";
                                            log["response"] = "ERR 406 NO_QUEST_AVAILABLE";
                                            log["code"] = 406;
                                            log["player"] = client->getNickname();
                                            printJson(log);
                                            std::cout << std::endl;
                                            std::string message = "ERR 406 NO_QUEST_AVAILABLE\r\n";
                                            send(client->getFd(), message.c_str(), message.size(), 0);
                                            break;
                                        }
                                    }
                                }
                            }
                            else {
                                JsonObject log = JsonObject();
                                log["timestamp"] = timestamp();
                                log["level"] = "WARN";
                                log["event"] = "NPC_NOT_FOUND";
                                log["response"] = "ERR 404 NPC_NOT_FOUND";
                                log["code"] = 404;
                                log["player"] = client->getNickname();
                                printJson(log);
                                std::cout << std::endl;
                                std::string error_msg = "ERR 404 NPC_NOT_FOUND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
                        }
                        else {
                            
                            if (client->getNickname() != "")
                                log_error(client, line);
                            else
                                log_error_ip(client, line);
                            std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                            send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                            break;
                        }
                    }
                }
                else {
                    std::string room = world["world"]["locations"][0]["name"].getString();
                    std::string name = "";
                    if (client) {
                        room = client->get_current_room();
                        name = client->getNickname();
                    }
                    for (size_t j = 0; j < world["world"]["locations"].size(); j++) {
                        if (world["world"]["locations"][j]["name"].getString() == room) {
                            for (size_t k = 0; k < world["world"]["locations"][j]["players"].size(); k++) {
                                if (world["world"]["locations"][j]["players"][k].getString() == name) {
                                    world["world"]["locations"][j]["players"].erase(k);
                                    break;
                                }
                            }
                        }
                    }
                    if (name != "") {
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
                        int fd = fds[i].fd;
                        for (std::vector<Client>::iterator it = clients.begin(); it != clients.end();) {
                            if (it->getFd() == fds[i].fd)
                            it = clients.erase(it);
                            else
                            ++it;
                        }
                        close(fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                }
            }
        }
    }
    for (const pollfd& pfd : fds)
        close(pfd.fd);
    return 0;
}
