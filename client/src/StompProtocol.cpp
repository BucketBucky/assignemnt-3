#include "../include/StompProtocol.h"
#include <iostream>
#include <sstream>
#include "../include/event.h"
#include <fstream>

//helper function to sort events by time
bool compareEventsByTime(const Event& a, const Event& b) {
    return a.get_time() < b.get_time();
}

StompProtocol::StompProtocol() : numOfSubs(0), numOfReciptes(0), activeSubs(), logoutReceiptId(-1) {}

std::vector<std::string> StompProtocol::process(std::string userLine)
{
    std::lock_guard<std::mutex> guard(lock); // in the stompClient we have the 2threads running together. they keyboardinput thread using process and the socket thread is using processAnswer.
    std::stringstream keyboardInputstream(userLine);

    std::string command;
    keyboardInputstream >> command;
    std::vector<std::string> multiFrames;

    if (command == "login")
    {
        std::string hostPort;
        std::string username;
        std::string password;
        
        if (!(keyboardInputstream >> hostPort >> username >> password))
        { // we check if one of the 3 arguments needed for a proper login is missing
            std::cout << "hostport username or password is missing" << std::endl;
            return multiFrames;
        }
        whosent = username; // we save the username so later when we need to use a SEND stomp we will know the name of the user who is sending it
        std::string frame = "CONNECT\n";
        frame = frame + "accept-version:1.2\n";
        frame = frame + "host:stomp.cs.bgu.ac.il\n";
        frame = frame + "login:" + username + "\n";
        frame = frame + "passcode:" + password + "\n";
        frame = frame + "\n";
        multiFrames.push_back(frame);
        return multiFrames;
    }

    if (command == "join")
    {
        std::string gameName; // the name of the game we want to join its channel
        if (!(keyboardInputstream >> gameName))
        { //
            std::cout << "missing game name" << std::endl;
            return multiFrames;
        }
        if (activeSubs.count(gameName))
        { // activeSubs is the map from the header file. we check if the user is alreay subscribed to the channel he is trying to join
            std::cout << "you are already subscribed to this channel " << std::endl;
            return multiFrames;
        }
        // in case we are not subscribed to the channel we create a unique id for the user-channel map
        numOfSubs = numOfSubs + 1;
        int id = numOfSubs;
        numOfReciptes = numOfReciptes + 1;
        int myrecipt = numOfReciptes;
        activeSubs[gameName] = id;
        std::string frame = "SUBSCRIBE\n";
        frame = frame + "destination:/" + gameName + "\n";
        frame = frame + "id:" + std::to_string(id) + "\n";
        frame = frame + "receipt:" + std::to_string(myrecipt) + "\n";
        frame = frame + "\n";
        multiFrames.push_back(frame);
        return multiFrames;
    }

    if (command == "exit")
    {
        std::string gameName;
        if (!(keyboardInputstream >> gameName))
        { // a check to see if we got the name of the channel we want to unsubscribe from
            std::cout << "no name" << std::endl;
            return multiFrames;
        }
        // we check if the user is subscribed to this game chanel
        auto subPointer = activeSubs.find(gameName);
        if (subPointer == activeSubs.end())
        {
            std::cout << "you are not subscribed to this channel " << std::endl;
            return multiFrames;
        }

        int id = subPointer->second; // each pointer in the map is has its key and value. the value is the id so we are getting the second parameter
        activeSubs.erase(subPointer);
        std::string frame = "UNSUBSCRIBE\n";
        frame = frame + "id:" + std::to_string(id) + "\n";
        frame = frame + "receipt:" + std::to_string(id) + "\n";
        frame = frame + "\n";
        multiFrames.push_back(frame);
        return multiFrames;
    }

    if (command == "logout")
    {
        numOfReciptes = numOfReciptes + 1; // logout requires reciptes as well
        int myRecipt = numOfReciptes;
        logoutReceiptId = myRecipt; // since we need to remove ourself from all the channels and erase our reports we need to save this id as a lougout id to handle that later on
        std::string frame = "DISCONNECT\n";
        frame = frame + "receipt:" + std::to_string(myRecipt) + "\n";
        frame = frame + "\n";
        whosent = "";
        multiFrames.push_back(frame);
        return multiFrames;
    }

    if (command == "report")
    {
        std::string jsonFile;
        if (!(keyboardInputstream >> jsonFile))
        {
            std::cout << "no json file" << std::endl; // we check if there is a json file to parse through
            return multiFrames;
        }
        if (whosent.empty())
        {
            std::cout << "Error: You must be logged in to send a report." << std::endl; // we we did not login before trying to report it wont be possible
            return multiFrames;
        }
        names_and_events data;
        try
        {
            data = parseEventsFile(jsonFile);
        }
        catch (std::exception &e)
        {
            std::cout << "did not parse file" << std::endl;
            return multiFrames;
        }
        std::string gameName = data.team_a_name + "_" + data.team_b_name; // we get the names of the two teams playing. that will be the name of the channel
        if (activeSubs.find(gameName) == activeSubs.end())
        {
            std::cout << "you are not subscribed to this channel-cannot report" << std::endl;
        }
        for (Event &event : data.events)
        {
            std::string frame = "SEND\n";
            frame = frame + "destination:/" + gameName + "\n";
            frame = frame + "file: " + jsonFile + "\n"; //we add this for the SQL so the server will get the name of the json file 
            frame = frame + "\n";
            frame = frame + "user: " + whosent + "\n";
            frame = frame + "team a: " + data.team_a_name + "\n";
            frame = frame + "team b: " + data.team_b_name + "\n";
            frame = frame + "event name: " + event.get_name() + "\n";
            frame = frame + "time: " + std::to_string(event.get_time()) + "\n";
            frame = frame + "general game updates:\n";

            for (auto &pair : event.get_game_updates())
            {
                frame = frame + pair.first + ":" + pair.second + "\n";
            }
            frame = frame + "team a updates:\n";
            for (auto &pair : event.get_team_a_updates())
            {
                frame = frame + pair.first + ":" + pair.second + "\n";
            }
            frame = frame + "team b updates:\n";
            for (auto &pair : event.get_team_b_updates())
            {
                frame = frame + pair.first + ":" + pair.second + "\n";
            }
            frame = frame + "description:\n" + event.get_discription() + "\n";
            multiFrames.push_back(frame);
        }
        std::cout << "Generated " << multiFrames.size() << " frames from report." << std::endl;
        return multiFrames;
    }

    if (command == "summary"){
        std::string gameName, user, file;
        // we check if we get the 3 argumentes required for a summary command 
        if (!(keyboardInputstream >> gameName >> user >> file))
        {
            std::cout << "one of the 3 arguments is missing" << std::endl;
            return multiFrames;
        }

        if (AllGamesInfo.find(gameName) == AllGamesInfo.end()) //we check if there is any info on the gmae. only then we can perform a proper summary
        {
            std::cout << "Error: no info exitrs " << std::endl;
            return multiFrames;
        }
        GameMemory &game = AllGamesInfo[gameName]; // we go to the GameMemory data stracture and acess the specipic game info
        std::ofstream summaryFile(file, std::ios::trunc);
        if (!summaryFile.is_open())
        {
            std::cout << "Error: file did not " << file << std::endl;
            return multiFrames;
        }
        // now we write all the info statistics into the file we have opend
         summaryFile << game.team_a << " vs " << game.team_b << "\n";
         summaryFile << "Game stats:\n";
 
         summaryFile << "General stats:\n";
         for ( auto &pair : game.general_stats)
         {
             summaryFile << pair.first << ": " << pair.second << "\n";
         }
 
         summaryFile << game.team_a << " stats:\n";
         for ( auto &pair : game.team_a_stats)
         {
             summaryFile << pair.first << ": " << pair.second << "\n";
         }
 
         summaryFile << game.team_b << " stats:\n";
         for ( auto &pair : game.team_b_stats)
         {
             summaryFile << pair.first << ": " << pair.second << "\n";
         }
        summaryFile << "Game event reports:\n";

        std::vector<Event> onlyUserEvents; //this is a temp events list which contains olny the specfic user we got as an arguemnts reported
        for ( Event& event : game.events) {
            if (event.getUserSender() == user) {
                onlyUserEvents.push_back(event);
            }
        }

        // we order the events by the time they happend
       std::sort(onlyUserEvents.begin(), onlyUserEvents.end(), compareEventsByTime);

        for (const Event& event : onlyUserEvents) { //we print from the already ordered list
            summaryFile << event.get_time() << " - " << event.get_name() << ":\n\n";
            summaryFile << event.get_discription() << "\n\n\n\n"; //for a big gap between events
        }
        summaryFile.close();
        std::cout << "suammary finished"<< std::endl;
        return multiFrames;
    }

    std::cout << "this command is not defined" << std::endl;
    return multiFrames;
    ;
}

void StompProtocol::processAnswer(std::string serverResponse)
{
    std::lock_guard<std::mutex> guard(lock); // a lock so the keyboard thread wont interupt when we acess shared memory data stractues like AllGamesInfo

    std::stringstream socketInput(serverResponse);
    std::string line;
    std::string command;
    std::getline(socketInput, command); //we read the command from the socket

    if (command == "MESSAGE")
    {
        //varibale preperation to later on be inserted into the AllGamesInfo memory
        std::string destination;
        std::string user;
        std::string team_a;
        std::string team_b;
        std::string event_name;
        int time = 0;
        std::string description;
        std::map<std::string, std::string> general_updates;
        std::map<std::string, std::string> a_updates;
        std::map<std::string, std::string> b_updates;

       
        while (std::getline(socketInput, line) && line != "") //reading the headers of the stomp untill we reach the destination string
        {
            if (line.find("destination:") == 0)
            {
                destination = line.substr(12); // we cut the string and get the destination value which comes right after 
            }
        }
        
        std::string gameName = destination.substr(1); //we cut the '/' from the begining of the game name
        std::string stringPart = ""; //a varaible to remember where are we in the current string
        while (std::getline(socketInput, line)){
            //how this works: each time we read a line from the socket. since we know how the report frame looks like, each of the argument we want is in a different line
            // we go down each line and since we cut the string each time, the string we want needs to start from 0, then we cut the unwamted information like 'user: Lion' ,we need onlu Lion
            // so we will cut user using substr method.
            if (line.find("user:") == 0)
                user = line.substr(6);
            else if (line.find("team a:") == 0)
                team_a = line.substr(8);
            else if (line.find("team b:") == 0)
                team_b = line.substr(8);
            else if (line.find("event name:") == 0)
                event_name = line.substr(12);
            else if (line.find("time:") == 0)
                time = std::stoi(line.substr(6)); //string to integer
                //from here we decide to which section the information we will get will have to got, like a memory switch
            else if (line == "general game updates:")
                stringPart = "general";
            else if (line == "team a updates:")
                stringPart = "team_a";
            else if (line == "team b updates:")
                stringPart = "team_b";
            else if (line == "description:")
                stringPart = "description";
            else
            {
                if (stringPart == "description")
                {
                    description = description + line + "\n";
                }
                else if (line.find(":") != std::string::npos) //we check if a ':'exits. npos means no position, (no position = false then enter the if)
                {
                    int split = line.find(":");
                    std::string key = line.substr(0, split);
                    std::string val = line.substr(split + 1);

                    if (stringPart == "general")
                        general_updates[key] = val;
                    if (stringPart == "team_a")
                        a_updates[key] = val;
                    if (stringPart == "team_b")
                        b_updates[key] = val;
                }
            }
        }

        //if the game does not exits in the memory we create it
        if (AllGamesInfo.find(gameName) == AllGamesInfo.end())
        {
            AllGamesInfo[gameName].team_a = team_a;
            AllGamesInfo[gameName].team_b = team_b;
        }
        Event event(team_a, team_b, event_name, time, general_updates, a_updates, b_updates, description); //adding the event
        event.setUserSender(user);
        AllGamesInfo[gameName].events.push_back(event);
        
        //update all the current info
       for (auto &pair : general_updates) {
            AllGamesInfo[gameName].general_stats[pair.first] = pair.second;
        }
        for (auto &pair : a_updates) {
            AllGamesInfo[gameName].team_a_stats[pair.first] = pair.second;
        }
        for (auto &pair : b_updates) {
            AllGamesInfo[gameName].team_b_stats[pair.first] = pair.second;
        }
    }

    else if (command == "CONNECTED")
    {
        std::cout << "Login successful" << std::endl;
    }
    else if (command == "ERROR")
    {
        std::cout << "Error:\n" << serverResponse << std::endl;
    }
    else if (command == "RECEIPT") {
       std::string receiptIdStr;
       //searcing for the id string 
       while (std::getline(socketInput, line) && line != "")
        {
            if (line.find("receipt-id:") == 0)
            {
                receiptIdStr = line.substr(11);
            }
        }
        std::cout << "recipet id: " << receiptIdStr << std::endl; 

        if (!receiptIdStr.empty() && std::stoi(receiptIdStr) == logoutReceiptId) //stringn to int + checking it is a logout recipet
        {
            //cleaning all the data from the user subsriptions
            std::cout << "clearing data" << std::endl;
            activeSubs.clear();
            numOfSubs = 0;
            AllGamesInfo.clear();
            whosent = "";
        }
    }
}


    
