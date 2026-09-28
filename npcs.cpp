#include "npcs.hpp"

void Npc::setNickname(const std::string& nickname) {
    this->nickname = nickname;
}

const std::string& Npc::getNickname() const {
    return nickname;
}

double Boss::getXp() {
    return xp;
}

void Boss::setXp(double xp) {
    this->xp = xp;
}

int Npc::getHp() {
    return hp;
}

int Npc::getMaxHp() {
    return max_hp;
}

void Npc::setMaxHp(double hp) {
    this->max_hp = hp;
}

void Npc::setHp(double hp) {
    this->hp = hp;
}

int Boss::getAttackPower() {
    return attack_power;
}

void Boss::setAttackPower(double attack_power) {
    this->attack_power = attack_power;
}

int Boss::getCoins() {
    return coins;
}

void Boss::setCoins(double coins) {
    this->coins = coins;
}
