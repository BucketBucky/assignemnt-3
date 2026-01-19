#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

int main(int argc, char *argv[]) {
    while (true) {
        const short bufsize = 1024;
        char buf[bufsize];
        std::stringstream keyboardStream;
        std::string line;
        std::string command;
        std::string hostPort;
        std::string hostIP;
        short port;
        bool loginHappend=false;
        std::cout << "Login:" << std::endl;
        //the loop runs while we dont have enough argumnets for a proper login, when it does it breaks
        while (std::cin.getline(buf, bufsize)) {
            line = buf;
            if(line.empty()){
                continue;
            }
            keyboardStream.clear(); 
            keyboardStream.str(line); 
            keyboardStream >> command >> hostPort; //we enter the command into the keyboardstream variable 
            if (command == "login" && hostPort.find(':') != std::string::npos) {
                int sepearotr = hostPort.find(':');
                hostIP = hostPort.substr(0, sepearotr);
                port = stoi(hostPort.substr(sepearotr + 1));
                loginHappend=true;
                break; 
            }
            std::cout << "Error: invalid login arguments" << std::endl;
        }
        if(!loginHappend){
            break;
        }
        ConnectionHandler myHandler(hostIP, port);
        if (!myHandler.connect()) {
            std::cerr << "connecting failed" << std::endl;
            continue; 
        }

        StompProtocol myProtocol;
        bool connectedToServer = true; //we keep this variable so each one of the threads will know if the there is still work 
        //we take the login line and use the connection handler o turn into frames
        for (auto &frame : myProtocol.process(line)){
        myHandler.sendFrameAscii(frame, '\0');
        } 
        //we create the thread which listens to the server socket. the main thread will be listening to the keyboard input
        std::thread mySocketThread([&myHandler, &myProtocol, &connectedToServer]() {
            std::string ans;
            while (connectedToServer && myHandler.getFrameAscii(ans, '\0')) {
                myProtocol.processAnswer(ans); // הפרוטוקול מטפל בלוגיקה ובהדפסות
            }
            connectedToServer = false;
            std::cout << "disconnected from server" << std::endl;
        });
        // the main thread (keyboard)
        while (connectedToServer) {
            std::cin.getline(buf, bufsize);
            if (!connectedToServer){ //if we got input from the keybord and the client-server connection lost we will know from 'connectedToServer'
                break;
            }  
            std::vector<std::string> frames = myProtocol.process(buf);
            for (auto &frame : frames) {
                if (!myHandler.sendFrameAscii(frame, '\0')) {
                    connectedToServer = false; 
                    break;
                }
            }
        }
        myHandler.close();
        if (mySocketThread.joinable()){
            mySocketThread.join();
        } 
    }
    return 0;
}