#pragma once
#include "json_parser.hpp"
#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <ctime>

class Npc {
private:
    std::string nickname;
    int hp;
    int max_hp;
    
public:
    
    time_t timer = time(nullptr);
    JsonObject drops = JsonObject{};
    Npc(std::string nickname, int hp) :
        nickname(nickname),
        hp(hp),
        max_hp(hp)
    {}

    virtual ~Npc() = default;
    
    const std::string& getNickname() const;
    int getHp();
    int getMaxHp();
    
    void setNickname(const std::string& nickname);
    void setHp(double hp);
    void setMaxHp(double hp);

};

class Boss : public Npc {
private:
    int attack_power; 
    int coins;
    double xp;

    
public:   
    Boss(
        std::string nickname,
        int hp,
        int attack_power,
        int coins,
        double xp
    )
        : Npc(nickname, hp),
          attack_power(attack_power),
          coins(coins),
          xp(xp)
    {}

    int getAttackPower();
    int getCoins();
    double getXp();

    void setAttackPower(double attack_power);
    void setCoins(double coins);
    void setXp(double xp);
};

class Trader : public Npc {
public:
    Trader(std::string nickname, int hp) : Npc(nickname, hp) {}
};

struct Node {
    Npc *npc = nullptr;
    Node *next = nullptr;
    Node *prev = nullptr;

    Node(Npc *npc) {this->npc = npc;}
};

struct NpcList {
    Node *head = nullptr;
    Node *tail = nullptr;

    void push_back(Npc *n) {
        Node *tmp = new Node(n);
        if (!head) {
            head = tmp;
            tail = tmp;
            return ;
        }
        tmp->prev = tail;
        tail->next = tmp;
        tail = tmp;
    }

    void push_front(Npc *n) {
        Node *tmp = new Node(n);
        if (!head) {
            head = tmp;
            tail = tmp;
            return ;
        }
        tmp->next = head;
        head->prev = tmp;
        head = tmp;
    }

};

struct LocationNpcsNode {
    NpcList *npcs = new NpcList();
    std::string location = "";
    LocationNpcsNode *next = nullptr;
    LocationNpcsNode *prev = nullptr;
    LocationNpcsNode(std::string name) {
        this->location = name;
    }
};

struct Locations {
    LocationNpcsNode *head = nullptr;
    LocationNpcsNode *tail = nullptr;

    void push_back(std::string name) {
        LocationNpcsNode *tmp = new LocationNpcsNode(name);
        if (!head) {
            head = tmp;
            tail = tmp;
            return ;
        }
        tail->next = tmp;
        tmp->prev = tail;
        tail = tmp;
    }

    void push_front(std::string name) {
        LocationNpcsNode *tmp = new LocationNpcsNode(name);
        if (!head) {
            head = tmp;
            tail = tmp;
            return ;
        }
        head->prev = tmp;
        tmp->next = head;
        head = tmp;
    }
};
