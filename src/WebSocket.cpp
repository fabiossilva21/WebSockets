#include <WebSocket.hpp>

WebSocket::WebSocket() {

    int opt = 1;
    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (this->server_fd == 0) {
        perror("socket");
        state = State::FAIL;
        return;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        state = State::FAIL;
        return;
    }

    this->address.sin_family = AF_INET;
    this->address.sin_addr.s_addr = INADDR_ANY; 
    this->address.sin_port = htons(this->PORT); 

    if (bind(this->server_fd, (struct sockaddr *)&(this->address), sizeof(this->address)) < 0) {
        perror("bind");
        state = State::FAIL;
        return;
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        state = State::FAIL;
        return;
    }

    this->isProperlyInitialized = true;
}

WebSocket::~WebSocket() {
    closeWebSocket();
}

bool WebSocket::waitForClient() {
    if (state == State::FAIL) return false;

    int addrlen = sizeof(this->address);

    if ((client_fd = accept(server_fd, (struct sockaddr *)&(this->address), (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        state = State::FAIL;
        return false; 
    } 

    state = State::OPEN;
    return true;
}

std::string WebSocket::receiveData() {
    std::string data;

    if ((state == State::FAIL || state == State::CLOSED) || client_fd < 0) { return data; }

    int readBytes = 0;
    int readSoFar = 0;
    int mallocBuffer = 512;

    char * buffer = (char *)malloc(mallocBuffer); 

    while((readBytes = read(this->client_fd, buffer+readSoFar, mallocBuffer)) == mallocBuffer) {
        readSoFar += readBytes;

        buffer = (char *)realloc(buffer, readSoFar + mallocBuffer);
    }

    readSoFar += readBytes;
    buffer[readSoFar] = 0;

    data = std::string(buffer, readSoFar);

    free(buffer);

    return data;
}

void WebSocket::sendData(std::string data) {
    write(this->client_fd, data.c_str(), data.size());
}

std::map<std::string, std::string> WebSocket::parseHeaders(std::string data) {

    std::map<std::string, std::string> map;

    // Last New Line
    int lastNL = 0;
    // Last Carrige Return
    int currCR = 0;

    // Find the first "Carrige Return" character located in every header line
    currCR = data.find('\r');

    // This line should contain "GET / HTTP/1.1"
    std::string firstLine = data.substr(lastNL, currCR);
    if (firstLine != "GET / HTTP/1.1") return map;

    lastNL = currCR + 1;

    while ((currCR = data.find('\r', lastNL)) > 0) {
        std::string line = data.substr(lastNL+1, currCR - lastNL - 1);

        int colonIdx = line.find(':');

        // Invalid Line as all should be "Name: Value"
        if (colonIdx <= 0) break;

        std::string name = line.substr(0, colonIdx);
        std::string value = line.substr(colonIdx+2);

        lastNL = currCR+1;

        map.insert({name, value});
    }

    return map;
}

std::string WebSocket::hexToASCII(std::string data) {
    std::string ascii;

    for (int i = 0; i < data.size(); i += 2) {
        int v = strtol((data.substr(i, 2)).c_str(), nullptr, 16);
        ascii.push_back(v);
    }

    return ascii;
}

bool WebSocket::start() {
    if(state == State::FAIL || !waitForClient()) return false;

    std::string data = receiveData();

    auto map = parseHeaders(data);

    // Do we have a "Connection" value?
    std::string connectionValue = map["Connection"];
    if (connectionValue == "") state = State::FAIL;

    // Do we have a "Upgrade" value?
    std::string upgradeValue = map["Upgrade"];
    if (upgradeValue == "") state = State::FAIL;

    // Do we have a "Sec-WebSocket-Key" value?
    std::string WSKeyValue = map["Sec-WebSocket-Key"];
    if (WSKeyValue == "") state = State::FAIL;

    if (state == State::FAIL) return false;

    std::string sha1(Sha1Algorithm::Sha1Hash(std::string(WSKeyValue + MAGICSTRING)));

    std::string WSAccept = BaseEncode::Base64Encode(hexToASCII(sha1));

    DEBUGLOG("Received challenge: %s\n", WSKeyValue.c_str());
    DEBUGLOG("Replied with: %s\n", WSAccept.c_str());

    // Build the response for the client
    std::string response;
    response.append("HTTP/1.1 101 Switching Protocols\r\n");
    response.append("Upgrade: websocket\r\n");
    response.append("Connection: Upgrade\r\n");
    response.append("Sec-WebSocket-Accept: " + WSAccept + "\r\n\r\n");

    sendData(response);

    state = State::CONNECTED;

    return true;
}

std::string WebSocket::receiveMessage() {

    std::string unmasked_payload;
    std::string data;

    if (state != State::CONNECTED) return unmasked_payload;
    // TODO: Verify what type of message is it!
    // TODO: Probably send this to a specific delegate

    data = receiveData();

    const char * c_data = data.c_str();

    IH tmp;
    tmp.fin         = (c_data[0] & 0x80) >> 7;
    tmp.rsv         = (c_data[0] & 0x70) >> 4;
    tmp.opcode      = (c_data[0] & 0x0f);
    tmp.mask        = (c_data[1] & 0x80) >> 7;
    tmp.payload_len = (c_data[1] & 0x7f);

    switch (tmp.opcode) {
        case WebSocket::Ping:
            sendPong();
            DEBUGLOG("Got a Ping request\n");
            return unmasked_payload;
            break;
        case WebSocket::Pong:
            DEBUGLOG("Got a Pong reply\n");
            return unmasked_payload;
            break;
        case WebSocket::Connection_Close:
            closeWebSocket();
            DEBUGLOG("Got a close request\n");
            return unmasked_payload;
            break;
        case WebSocket::Text:
            DEBUGLOG("Got a text message\n");
            break;
        default:
            DEBUGLOG("Got a unknown opcode (%d). Dropping client and closing socket.\n", tmp.opcode);
            closeWebSocket();
            return unmasked_payload;
            break;
    }

    int maskingIdx = 2;
    if (tmp.payload_len == 126) {
        tmp.payload_len = ((c_data[2] & 0xff) << 8) + (c_data[3] & 0xff);
        maskingIdx = 4;
    } else if (tmp.payload_len == 127) {
        tmp.payload_len = ((c_data[2] & 0xff) << 56) + ((c_data[3] & 0xff) << 48)
                        + ((c_data[4] & 0xff) << 40) + ((c_data[5] & 0xff) << 32)
                        + ((c_data[6] & 0xff) << 24) + ((c_data[7] & 0xff) << 16)
                        + ((c_data[8] & 0xff) << 8) + ((c_data[9] & 0xff));
        maskingIdx = 10;
    }

    tmp.maskingKey[0] = (c_data[maskingIdx++] & 0xff);
    tmp.maskingKey[1] = (c_data[maskingIdx++] & 0xff);
    tmp.maskingKey[2] = (c_data[maskingIdx++] & 0xff);
    tmp.maskingKey[3] = (c_data[maskingIdx++] & 0xff);

    DEBUGLOG("FIN: %x\nRSV: 0x%x\nOpcode: 0x%x\nMASK: %d\nPayload Length: %d\nMasking Key: 0x%x\n", tmp.fin, tmp.rsv, tmp.opcode, tmp.mask, tmp.payload_len, tmp.maskingKey);

    int keyIdx = 0;

    for (char c : data.substr(maskingIdx)) {
        unmasked_payload.push_back(c ^ tmp.maskingKey[keyIdx % 4]);
        keyIdx++;
    }

    return unmasked_payload;
}

void WebSocket::sendMessage(std::string data) {
    if(state != State::CONNECTED) return;

    std::string message;

    // FIN + RSV + Opcode (1 byte)
    // FIN = 1; Opcode = 1 (Text Message)
    message.push_back(0x81);

    if (data.size() <= 125) {
        // Mask + Payload Length (1 byte)
        message.push_back(data.size());

    } else if (data.size() <= 65535) {
        // Mask + Payload Length (1 byte)
        message.push_back(126);

        // Extended Payload Length (2 bytes)
        message.push_back((data.size() & 0xff00) >> 16);
        message.push_back(data.size() & 0xff);
    } else {
        // Mask + Payload Length (1 byte)
        message.push_back(127);

        // Extended Payload Length (2 bytes)
        message.push_back((data.size() & 0xff00000000000000) >> 56);
        message.push_back((data.size() & 0x00ff000000000000) >> 48);

        // Extended Payload Length Continued (6 bytes)
        message.push_back((data.size() & 0x0000ff0000000000) >> 40);
        message.push_back((data.size() & 0x000000ff00000000) >> 32);
        message.push_back((data.size() & 0x00000000ff000000) >> 24);
        message.push_back((data.size() & 0x0000000000ff0000) >> 16);
        message.push_back((data.size() & 0x000000000000ff00) >> 8);
        message.push_back((data.size() & 0x00000000000000ff) >> 0);
    }

    message.append(data);

    sendData(message);
}

void WebSocket::closeWebSocket() {
    std::string message;

    // FIN + RSV + Opcode (1 byte)
    message.push_back(0x80 + WebSocket::Connection_Close);

    /** Mask + Payload Length (1 byte)
     *  TODO: We do not support sending a message while closing
     */ 
    message.push_back(0x00);

    sendData(message);

    state = State::CLOSED;
    closeTCPConnection();
}

void WebSocket::sendPing() {
    std::string message;

    // FIN + RSV + Opcode (1 byte)
    message.push_back(0x80 + WebSocket::Ping);

    /** Mask + Payload Length (1 byte)
     */ 
    message.push_back(0x00);

    sendData(message);
}

void WebSocket::sendPong() {
    std::string message;

    // FIN + RSV + Opcode (1 byte)
    message.push_back(0x80 + WebSocket::Pong);

    /** Mask + Payload Length (1 byte)
     */ 
    message.push_back(0x00);

    sendData(message);
}

void WebSocket::closeTCPConnection() {
    shutdown(this->server_fd, SHUT_RDWR);
    shutdown(this->client_fd, SHUT_RDWR);

    close(this->server_fd);
    close(this->client_fd);
}