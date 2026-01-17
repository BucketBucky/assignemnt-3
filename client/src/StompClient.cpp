#include <stdlib.h>
#include "../include/ConnectionHandler.h"
#include <thread>
#include <iostream>
#include "../include/StompProtocol.h"
#include <vector>

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " host port" << std::endl
                  << std::endl;
        return -1;
    }

    std::string host = argv[1];
    short port = atoi(argv[2]);

    ConnectionHandler connectionHandler(host, port);

    if (!connectionHandler.connect())
    {
        std::cerr << "Cannot connect to " << host << ":" << port << std::endl;
        return 1;
    }
    std::cout << "Connected to the server!" << std::endl;

    StompProtocol protocol;

    std::thread keyboardListeningThread([&connectionHandler, &protocol]() { // this is the 'read from keyboard' thread, the  main thread will be listening from the socket connected to the server
        while (1)
        {
            const short bufsize = 1024;
            char buf[bufsize];
            std::cin.getline(buf, bufsize);
            std::string line(buf);
            std::vector<std::string> framesToSend = protocol.process(line);

            for (std::string &frame : framesToSend)
            {
                if (frame.length() > 0)
                {
                    if (!connectionHandler.sendFrameAscii(frame, '\0'))
                    {
                        std::cout << "Disconnected (Error sending)!" << std::endl;
                        exit(1);
                    }
                }
            }
        }
    });

    while (1) // here is the main thread which is listening to the socket
    {
        std::string answer;
        if (!connectionHandler.getFrameAscii(answer, '\0'))
        {
           
            std::cout << "Disconnected (Error receiving)!" << std::endl;
            break;
        }
        protocol.processAnswer(answer);
    }
    if (keyboardListeningThread.joinable())
    {
        keyboardListeningThread.join();
    }
    return 0;
}



