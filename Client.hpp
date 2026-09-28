#pragma once
#include "json_parser.hpp"
#include <iostream>
#include <sys/socket.h>
#include <cstring>

class Client {
private:
    std::string ip;
    std::string nickname = "";
    std::string recv_buffer;
    std::string current_room;
    int fd;
    int group_id = -1;
    int level = 1;
    int hp = 100;
    int max_hp = 100;
    int attack_power = 5; 
    int coins = 50;
    double xp = 0;
    
public:
    int port;
    double lvlup_buffer = 1;
    bool is_connected = false;
    double attack_multiplier = 1;
    int weapon_durability = 1;
    int defense_ratio = 0;
    int armor_durability = 1;
    std::string current_weapon = "";
    std::string current_armor = "";
    JsonArray quests = JsonArray();
    std::vector<std::string> inventory = {};

    int getFd() const;
    int getGroupId() const;
    std::string getIp() const;
    std::string& getBuffer();
    const std::string& get_current_room() const;
    const std::string& getNickname() const;
    double getXp();
    int getHp();
    int getMaxHp();
    int getAttackPower();
    int getCoins();
    int getLevel();
    
    void setFd(const int fd);
    void setGroupId(const int group_id);
    void setIp(std::string ip);
    void set_current_room(std::string current_room);
    void setNickname(const std::string& nickname);
    void setXp(double xp);
    void setHp(double hp);
    void setMaxHp(double hp);
    void setAttackPower(double attack_power);
    void setCoins(double coins);
    void setLevel(int level);
    
    void sendMessage(const std::string& message);
    bool in_group() const;



    Client(int fd, std::string nickname)
    {
        this->fd = fd;
        this->nickname = nickname;
    }
};
