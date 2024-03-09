#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <iphlpapi.h>
#include <list>
#include <map>
#include <mutex>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <thread>
#include <vector>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/sha.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

class WebSocketServer {
private:
    int threads;
    int port;
    string host_port;
    vector<thread> thread_list;
    list<function<void()>> clients;
    mutex clients_mutex;
    condition_variable cv;
    atomic<bool> running;

    void post(function<void()> func) {
        unique_lock<mutex> lock(clients_mutex);
        clients.emplace_back(func);
        cv.notify_one();
    }

    void initializeWorkers() {
        running = true;
        for (int i = 0; i < threads; i++) {
            thread_list.emplace_back([this] {
                while (running) {
                    function<void()> client;
                    {
                        unique_lock<mutex> lock(clients_mutex);
                        while (clients.empty()) {
                            cv.wait(lock);
                            if (!running) {
                                break;
                            }
                        }
                        if (!clients.empty()) {
                            client = *clients.begin();
                            clients.pop_front();
                        } else {
                            break;
                        }
                    }
                    if (client) {
                        client();
                    }
                    cv.notify_all();
                }
            });
        }
    }

    SOCKET initializeListenSocket() {
        // Initializing winsock
        SOCKET ListenSocket = INVALID_SOCKET;
        int iResult;

        struct addrinfo *result = NULL, *ptr = NULL, hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the local address and port to be used by the server
        iResult = getaddrinfo("localhost", host_port.c_str(), &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed: %d\n", iResult);
            WSACleanup();
            return INVALID_SOCKET;
        }

        // Create a SOCKET for the server to listen for client connections
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            printf("Error at socket(): %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return INVALID_SOCKET;
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("bind failed with error: %d\n", WSAGetLastError());
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return INVALID_SOCKET;
        }

        freeaddrinfo(result);
        return ListenSocket;
    }

    bool requestIsValid(char *request, map<string, string> lines) {
        string req = request;
        stringstream reqStream(req);

        //Checking method, this does not map to a key, so it needs to be handles separately.
        string actual;
        string expected = "GET / HTTP/1.1";
        getline(reqStream, actual);
        trim(actual);

        if (actual != expected) {
            return false;
        }
        map<string, string> requirements = {
            {"Upgrade", "websocket"},
            {"Host", "localhost:3001"},
            {"Connection", "Upgrade"},
            {"Sec-WebSocket-Version", "13"}};
        auto it = requirements.begin();
        while (it != requirements.end()) {
            string expected = it->second;
            string actual = lines.find(it->first)->second;
            if (expected != actual) {
                return false;
            }
            it++;
        }
        return actual == expected;
    }

    void terminateConnection(SOCKET socket) {
        int shutdownStatus = shutdown(socket, SD_BOTH);
        if (shutdownStatus == SOCKET_ERROR) {
            closesocket(socket);
        }
    }

    void extractHeaders(char *request, map<string, string> &map) {
        stringstream reqStream(request);
        string currentHeader;
        getline(reqStream, currentHeader); // Consumes the GET-line
        while (getline(reqStream, currentHeader)) {
            trim(currentHeader);
            if (currentHeader.empty()) break;
            int delimiter = currentHeader.find(":");
            string key = currentHeader.substr(0, delimiter);
            string value = currentHeader.substr(delimiter + 1);
            trim(value);
            trim(key);
            map.insert({key, value});
        }
    }

    /*
        Trims leading and tailing whitespace from the string. 
    */
    void trim(string &s) {
        int start = 0;
        int end = s.length() - 1;

        for (; isspace(s[start]); start++) continue;
        for (; isspace(s[end]); end--) continue;
        s = s.substr(start, end - start + 1);
    }

    static inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

    std::string base64_encode(unsigned char const *bytes_to_encode, unsigned int in_len) {
        const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; (i < 4); i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

            while ((i++ < 3))
                ret += '=';
        }

        return ret;
    }

    string unmaskMessage(unsigned char* message) {
        unsigned char mode = message[0];
        int length = message[1] & 127;
        int maskStart = 2;
        int dataStart = maskStart + 4;
        
        char unmasked[length];
        for (int i = dataStart; i < dataStart + length; i++) {
            unsigned char c = message[i] ^ message[maskStart + (i-dataStart)%4];
            unmasked[i - dataStart] = (char) c;
        }
        return unmasked;
    }

    void frameMessage(string message, unsigned char* framed) {
        framed[0] = 0x81;
        int len = message.length();
        int next = 1;
        if (len <= 125) {
            framed[next++] = len;
        } else if (len <= 65535) {
            //Add two bytes representing the length of the message
        } else {
            //Add 8 extra bytes to represent the length
        }
        for (int i = 0; i < message.length(); i++) {
            framed[next++] = message[i];
        }
    }

public:
    WebSocketServer(int port_param) {
        port = port_param;
        host_port = to_string(port_param);
        threads = 4;
    }

    WebSocketServer(int port_param, int worker_threads) {
        port = port_param;
        host_port = to_string(port_param);
        threads = worker_threads;
    }

    void start() {
        initializeWorkers();

        WSADATA wsaData;
        int status;

        status = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (status) {
            cout << "An error occurred during WSA startup." << endl;
            return;
        }

        SOCKET ListenSocket = initializeListenSocket();

        status = listen(ListenSocket, SOMAXCONN);
        if (status == SOCKET_ERROR) {
            cout << "An error occurred while listening for connections:" << WSAGetLastError() << endl;
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        SOCKET ClientSocket = INVALID_SOCKET;
        do {
            ClientSocket = accept(ListenSocket, NULL, NULL);
            if (ClientSocket == INVALID_SOCKET) {
                cout << "An error occurred while accepting a new client socket: " << WSAGetLastError() << endl;
                closesocket(ListenSocket);
                WSACleanup();
                return;
            }

            // From here the worker thread handles the handshake
            post([ClientSocket, this] {
                // Receive handshake and check for errors.
                char request[2048];
                int bytesReceived = recv(ClientSocket, request, 2048, 0);
                if (bytesReceived == 0) {
                    cout << "Connection closed." << endl;
                    terminateConnection(ClientSocket);
                    return;
                }

                //Map the header lines into key-value pairs. 
                map<string, string> headerLines;
                extractHeaders(request, headerLines);

                // Verify that the request is in accordance with standard RFC 6455
                if (!requestIsValid(request, headerLines)) {
                    cout << "Received request does not meet the requirements of RFC 6455. Closing socket." << endl;
                    const char* error = "HTTP/1.1 400 Bad Request\r\n\r\n";
                    send(ClientSocket, error, sizeof(error), 0);
                    terminateConnection(ClientSocket);
                    return;
                }

                string webSockAccept = headerLines.find("Sec-WebSocket-Key")->second + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                unsigned char hash[SHA_DIGEST_LENGTH]; // Buffer for the hash

                // Compute the SHA1 hash
                SHA1(reinterpret_cast<const unsigned char *>(webSockAccept.c_str()), webSockAccept.length(), hash);
                string encodedAccept = base64_encode(hash, 20);

                string response =
                    "HTTP/1.1 101 Switching Protocols\r\n"
                    "Connection: Upgrade\r\n"
                    "Upgrade: websocket\r\n"
                    "Sec-WebSocket-Accept: " +
                    encodedAccept + "\r\n";
                response += "Sec-WebSocket-Version-Server: " + headerLines.find("Sec-WebSocket-Version")->second + "\r\n\r\n";
                {
                    const char* res = response.c_str();
                    int bytesSent = send(ClientSocket, res, response.length(), 0);
                    if (bytesSent == 0) {
                        cout << "An error occurred during sending. Shutting down connection..." << endl;
                        terminateConnection(ClientSocket);
                        return;
                    }
                }

                char message[2048];
                bytesReceived = 0;
                bytesReceived = recv(ClientSocket, message, 2048, 0);
                if (bytesReceived == 0) {
                    cout << "Connection closed." << endl;
                    terminateConnection(ClientSocket);
                    return;
                }
                unsigned char* bytes = (unsigned char*) message;
                string parsedMessage = unmaskMessage(bytes);
                cout << "Received from client: " << parsedMessage << endl;
                string res = "Hello to you, sir";
                unsigned char tl[19];
                memset(tl, 0, sizeof(tl));
                frameMessage(res, tl);
                const char* toClient = (char*) tl;
                send(ClientSocket, toClient, 19, 0);
            });
        } while (true);
    }

    /*
      Stops all currently running threads.
    */
    void stop() {
        {
            unique_lock<mutex> lock(clients_mutex);
            while (!clients.empty()) {
                cv.wait(lock);
            }
            running = false;
        }
        cv.notify_all();
        for (int i = 0; i < threads; i++) {
            thread_list[i].join();
        }
    }
};

void test(char* message) {
    message[0] = 'h';
}

int main() {
    WebSocketServer server(3001);
    cout << "Starting server..." << endl;
    server.start();


    return 0;
}