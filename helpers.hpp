#include "Client.hpp"
#include "npcs.hpp"
#include <vector>
#include <poll.h>
#include <random>

extern std::string success;
extern std::random_device rd;
extern std::mt19937 gen;

Client *get_client_by_fd(int fd, std::vector<Client>& clients);

Client *get_client_by_nickname(std::string nickname, std::vector<Client>& clients);

bool validNickname(const std::string& name);

bool parseworld(const JsonValue& json);

void log_unauthorized_action(Client *client);

std::string timestamp();

Locations *add_npcs(JsonValue world);

void log_error(Client *client, std::string line);

void log_warning(Client *client, int code, std::string line, std::string event, std::string response);

void log_error_ip(Client *client, std::string line);

void log_connection_warning(Client *client, int code, std::string event, std::string response);

void log_connection_error(Client *client, int code, std::string event, std::string response);

void log_connection_success(Client *client);

int handle_connect(Client *client, std::string line, std::vector<Client> *clients, std::vector<Client> *candidate_clients, JsonValue *world);

int handle_quit(Client *client, std::string line, std::vector<Client> *clients, JsonValue *world, std::vector<pollfd> *fds, int i);

int handle_group(Client *client, std::string line, std::vector<Client> *clients, int *group_constant);

int handle_chat(Client *client, std::string line, std::vector<Client> *clients);

int handle_who(Client *client, std::string line, std::vector<Client> *clients);

int handle_look(Client *client, std::string line, JsonValue *world);

int handle_move(Client *client, std::string line, std::vector<Client> *clients, JsonValue *world);

int handle_inventory(Client *client, std::string line);

int handle_take(Client *client, std::string line, JsonValue *world);

int handle_drop(Client *client, std::string line, JsonValue *world);

int handle_status(Client *client, std::string line);

int handle_attack(Client *client, std::string line, JsonValue *world, Locations *npcs);

int handle_use(Client *client, std::string line, JsonValue *world);

int handle_unuse(Client *client, std::string line);

int handle_quests(Client *client, std::string line);

int handle_talk(Client *client, std::string line, JsonValue *world);

int handle_quest(Client *client, std::string line, JsonValue *world);