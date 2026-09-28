#include "Client.hpp"

void Client::setNickname(const std::string& nickname) {
    this->nickname = nickname;
}

void Client::setFd(const int fd) {
    this->fd = fd;
}

void Client::setIp(std::string ip) {
    this->ip = ip;
}

std::string Client::getIp() const {
    return ip;
}

const std::string& Client::getNickname() const {
    return nickname;
}

int Client::getFd() const {
    return fd;
}

void Client::set_current_room(std::string current_room) {
    this->current_room = current_room;
}

const std::string& Client::get_current_room() const {
    return current_room;
}

void Client::sendMessage(const std::string& message) {
    send(this->getFd(), message.c_str(), strlen(message.c_str()), 0);
}

std::string& Client::getBuffer()
{
    return recv_buffer;
}

int Client::getGroupId() const {
    return group_id;
}

void Client::setGroupId(const int group_id){
    this->group_id = group_id;
}

bool Client::in_group() const {
    return group_id > 0;
}

double Client::getXp() {
    return xp;
}

void Client::setXp(double xp) {
    this->xp = xp;
}

int Client::getHp() {
    return hp;
}

int Client::getMaxHp() {
    return max_hp;
}

void Client::setMaxHp(double hp) {
    this->max_hp = hp;
}

void Client::setHp(double hp) {
    this->hp = hp;
}

int Client::getAttackPower() {
    return attack_power;
}

void Client::setAttackPower(double attack_power) {
    this->attack_power = attack_power;
}

int Client::getCoins() {
    return coins;
}

void Client::setCoins(double coins) {
    this->coins = coins;
}

int Client::getLevel() {
    return level;
}

void Client::setLevel(int level) {
    this->level = level;
}
