#include <WebSocket.hpp>

#include <thread>

WebSocket * ws;

void getMessages() {
    for(;;) {
        std::string message = ws->receiveMessage();
        if (message.empty()) break;
        DEBUGLOG("Message received: %s\n", message.c_str());
    }
}

int main(int argc, char ** argv) {

    ws = new WebSocket();

    DEBUGLOG("Server Started\n");

    ws->start();

    DEBUGLOG("Client Connected\n");

    std::thread first(getMessages);

    for (;;) {
        std::string msg;
        std::getline(std::cin, msg);

        if (msg == "CLOSE SOCKET") {
            ws->closeWebSocket();
            continue;
        }

        if (msg == "PING") {
            ws->sendPing();
            continue;
        }

        ws->sendMessage(msg);
    }

    first.join();

    ws->closeWebSocket();
}