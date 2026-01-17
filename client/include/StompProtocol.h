#pragma once

#include "../include/ConnectionHandler.h"
#include "../include/event.h"

// TODO: implement the STOMP protocol

struct GameMemory { //this struct is desgined to save the updates we are getting for each game 
    std::string team_a;
    std::string team_b;
    std::map<std::string, std::string> general_stats;
    std::map<std::string, std::string> team_a_stats;
    std::map<std::string, std::string> team_b_stats;
    std::vector<Event> events; 
    
 
    GameMemory() {} 
};


class StompProtocol
{
private:
std::map<std::string, int> activeSubs; //to know the active subscriptions
int numOfSubs;
int numOfReciptes;
std::mutex lock; //we need a lock to synchrtinize the 2 threads working in the stompclient.cpp 
std::string whosent; 
std::map<std::string, GameMemory> AllGamesInfo; //a map which maps games to their information startcture above
int logoutReceiptId;
public:
StompProtocol();

std::vector<std::string> process(std::string userLine); //process might return sevrale messegess like report does
void processAnswer(std::string serverResponse);

};
