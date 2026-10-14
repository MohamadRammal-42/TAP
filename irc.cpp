#include <poll.h>
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
        "%Y-%m-%dT%H:%M:%SZ", t
    );

    return std::string(buffer);
}

std::vector<std::string> split_commands(const std::string& line)
{
    std::vector<std::string> commands;
    std::size_t start = 0;
    std::size_t pos;

    while ((pos = line.find("&&", start)) != std::string::npos)
    {
        std::string command = line.substr(start, pos - start);

        // Trim leading and trailing whitespace
        std::size_t first = command.find_first_not_of(" \t\r\n");
        if (first != std::string::npos)
        {
            std::size_t last = command.find_last_not_of(" \t\r\n");
            commands.push_back(command.substr(first, last - first + 1));
        }

        start = pos + 2;
    }

    std::string command = line.substr(start);
    std::size_t first = command.find_first_not_of(" \t\r\n");

    if (first != std::string::npos)
    {
        std::size_t last = command.find_last_not_of(" \t\r\n");
        commands.push_back(command.substr(first, last - first + 1));
    }

    return commands;
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
    for (size_t i = 0; i < world["world"]["locations"].size(); i++) {
        world["world"]["locations"][i]["items"] = json["world"]["locations"][i]["items"];
    }
}

int main() {
    JsonValue world = get_world();
    if (!parseworld(world))
        return 0;
    JsonValue world_2 = get_world();
    Locations *npcs = add_npcs(world);
    std::vector<Client> clients;
    std::vector<Client> candidate_clients;
    std::vector<pollfd> fds;
    time_t clear_lag_timer = time(0);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int group_constant = 42;
    // bool stop_server = false;

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
        // if (stop_server)
            // break;
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
                ssize_t bytes = recv((*client).getFd(), buffer, sizeof(buffer), 0);

                if (bytes > 0) {
                    (*client).getBuffer().append(buffer, bytes);

                    std::size_t pos;

                    while ((pos = client->getBuffer().find('\n')) != std::string::npos)
                    {
                        std::string line = client->getBuffer().substr(0, pos);
                        client->getBuffer().erase(0, pos + 1);

                        if (!line.empty() && line.back() == '\r')
                            line.pop_back();

                        // Split the line into individual commands
                        std::vector<std::string> commands = split_commands(line);

                        // Execute commands in order
                        for (const std::string& cmd : commands)
                        {
                            if (cmd.substr(0, 7) == "CONNECT") {
                                if (!handle_connect(client, cmd, &clients,
                                                    &candidate_clients, &world))
                                    break;
                            }
                            else if (cmd.substr(0, 4) == "QUIT") {
                                if (!handle_quit(client, cmd, &clients, &world, &fds, i))
                                    break;
                            }
                            else if (cmd.substr(0, 5) == "GROUP") {
                                if (!handle_group(client, cmd, &clients, &group_constant))
                                    break;
                            }
                            else if (cmd.substr(0, 4) == "CHAT") {
                                if (!handle_chat(client, cmd, &clients))
                                    break;
                            }
                            else if (cmd.substr(0, 3) == "WHO") {
                                if (!handle_who(client, cmd, &clients))
                                    break;
                            }
                            else if (cmd.substr(0, 4) == "LOOK") {
                                if (!handle_look(client, cmd, &world))
                                    break;
                            }
                            else if (cmd.substr(0, 4) == "MOVE") {
                                if (!handle_move(client, cmd, &clients, &world))
                                    break;
                            }
                            else if (cmd.substr(0, 9) == "INVENTORY") {
                                if (!handle_inventory(client, cmd))
                                    break;
                            }
                            else if (cmd.substr(0, 4) == "TAKE") {
                                if (!handle_take(client, cmd, &world))
                                    break;
                            }
                            else if (cmd.substr(0, 4) == "DROP") {
                                if (!handle_drop(client, cmd, &world))
                                    break;
                            }
                            else if (cmd.substr(0, 6) == "STATUS") {
                                if (!handle_status(client, cmd))
                                    break;
                            }
                            else if (cmd.substr(0, 6) == "ATTACK") {
                                if (!handle_attack(client, cmd, &world, npcs))
                                    break;
                            }
                            else if (cmd.substr(0, 3) == "USE") {
                                if (!handle_use(client, cmd, &world))
                                    break;
                            }
                            else if (cmd.substr(0, 5) == "UNUSE") {
                                if (!handle_unuse(client, cmd))
                                    break;
                            }
                            else if (cmd.substr(0, 6) == "QUESTS") {
                                if (!handle_quests(client, cmd))
                                    break;
                            }
                            else if (cmd.substr(0, 4) == "TALK") {
                                if (!handle_talk(client, cmd, &world))
                                    break;
                            }
                            else if (cmd.substr(0, 5) == "QUEST") {
                                if (!handle_quest(client, cmd, &world))
                                    break;
                            }
                            else {
                                if (client->getNickname() != "")
                                    log_error(client, cmd);
                                else
                                    log_error_ip(client, cmd);

                                std::string error_msg = "ERR 911 INVALID_COMMAND\r\n";
                                send(fds[i].fd, error_msg.c_str(), error_msg.size(), 0);
                                break;
                            }
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
                        for (std::vector<Client>::iterator it = candidate_clients.begin(); it != candidate_clients.end();) {
                            if (it->getFd() == fds[i].fd)
                                it = candidate_clients.erase(it);
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
    LocationNpcsNode *temp = npcs->head;
    LocationNpcsNode *prev_temp = temp;
    while (temp)
    {
        Node *cur = temp->npcs->head;
        Node *prev = cur;
        while (prev)
        {
            cur = cur->next;
            delete prev->npc;
            delete prev;
            prev = cur;
        }
        delete temp->npcs;
        temp = temp->next;
        delete prev_temp;
        prev_temp = temp;
    }
    delete npcs;
    return 0;
}
