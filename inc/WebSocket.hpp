// Include Guard
#ifndef WEBSOCKET_HPP
#define WEBSOCKET_HPP

// C++ includes
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include "Sha1Algorithm.hpp"
#include "BaseEncode.hpp"

// C includes
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <unistd.h>

#define DEBUGLOG(...) {          \
    printf("==%d== ", getpid()); \
    printf(__VA_ARGS__);         \
} 

class WebSocket {

typedef struct incompleteHeader {
    bool fin;
    unsigned int rsv;
    unsigned int opcode;
    bool mask;
    unsigned long payload_len;
    unsigned char maskingKey[4];
} IH;

public:
    /**
     * Variable to help distinguish the role of the instance in order
     * to know if the messages received/sent should be masked or not.
     * 
     * For now we just implement the server-side of things.
     */
    enum class Role { CLIENT, SERVER };

    /**
     * Variable to save the state of the server/client
     */
    enum class State { CLOSED, OPEN, CONNECTED, FAIL };

    // Default constructor
    WebSocket();
    ~WebSocket();

    // 
    bool isValid() { return isProperlyInitialized; }
    bool start();

    std::string receiveMessage();

    void sendMessage(std::string data);
    void closeWebSocket();

    void sendPing();
    void sendPong();

private:
    /**
     * Constant variables
     */
    const int PORT = 9001;
    const std::string MAGICSTRING = std::string("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"); 

    enum OPCODE { Continuation, Text, Binary, Connection_Close = 8, Ping, Pong };

    /**
     * Auxiliary variables
     */
    int server_fd;
    /**
     * This should probably be an array or vector of some sort, however,
     * we only accept one client at the time, so there should be not problem. 
     */
    int client_fd;

    State state = State::CLOSED;

    bool isProperlyInitialized = false;

    struct sockaddr_in address;

    /**
     * Private Methods
     */
    bool waitForClient();

    std::map<std::string, std::string> parseHeaders(std::string data);

    std::string hexToASCII(std::string data);
    std::string receiveData();

    void sendData(std::string data);
    void closeTCPConnection();
};


#endif // WEBSOCKET_HPP