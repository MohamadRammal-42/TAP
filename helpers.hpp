#include "Client.hpp"
#include "npcs.hpp"
#include <vector>
#include <poll.h>

extern std::string success;

Client *get_client_by_fd(int fd, std::vector<Client>& clients);

Client *get_client_by_nickname(std::string nickname, std::vector<Client>& clients);

bool validNickname(const std::string& name);

bool parseworld(const JsonValue& json);

void log_unauthorized_action(Client& client);

std::string timestamp();

Locations *add_npcs(JsonValue world);

void log_error(Client *client, std::string line);

void log_error_ip(Client *client, std::string line);

void log_connection_warning(Client *client, int code, std::string event, std::string response);

void log_connection_error(Client *client, int code, std::string event, std::string response);

void log_connection_success(Client *client);

int handle_connect(Client *client, std::string line, std::vector<Client> *clients, std::vector<Client> *candidate_clients, JsonValue *world);

int handle_quit(Client *client, std::string line, std::vector<Client> *clients, JsonValue *world, std::vector<pollfd> *fds, int i);

int handle_group(Client *client, std::string line, std::vector<Client> *clients, int *group_constant);