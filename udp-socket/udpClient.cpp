#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define DEFAULT_BUFLEN 512

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <list>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <regex>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;



SOCKET initializeSocket() {
  int iResult;
  SOCKET ConnectSocket;

  struct addrinfo *result = NULL,
                *ptr = NULL,
                hints;

  ZeroMemory( &hints, sizeof(hints) );
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  #define DEFAULT_PORT "27015"

  // Resolve the server address and port
  iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
      printf("getaddrinfo failed: %d\n", iResult);
      WSACleanup();
      return ConnectSocket;
  }
  // Attempt to connect to the first address returned by
  // the call to getaddrinfo
  ptr=result;

  // Create a SOCKET for connecting to server
  ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
      ptr->ai_protocol);
  if (ConnectSocket == INVALID_SOCKET) {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return ConnectSocket;
  }

  freeaddrinfo(result);

  return ConnectSocket;
}


void sendMessage(SOCKET &socket, string message) {
  char sendbuf[message.length()];
  strcpy(sendbuf, message.c_str());
  struct sockaddr_in ClientAddr;
  int clientAddrSize = (int)sizeof(ClientAddr);
  short port = 27015;
  const char* local_host = "127.0.0.1";
  ClientAddr.sin_family = AF_INET;
  ClientAddr.sin_port = htons(port);
  ClientAddr.sin_addr.s_addr = inet_addr(local_host);

  cout << "Sending message..." << endl;
      
  int clientResult = sendto(socket,
                      sendbuf, message.length(), 0, (SOCKADDR *) & ClientAddr, clientAddrSize);

  if (clientResult == SOCKET_ERROR) {
    printf("send failed: %d\n", WSAGetLastError());
    closesocket(socket);
    WSACleanup();
  }
}

string takeInput() {
  bool inputIsValid = false;
  string input;
  while (!inputIsValid) {
    cout << "Write an expression on the form [number] [+/-] [number]. Type \"quit\" to quit." << endl;
    getline(cin, input);
    if (!input.compare("quit")) {
      return input;
    }
    regex requiredFormat("\\d+\\s+[+-]?\\s+\\d+");
    if (regex_match(input, requiredFormat)) {
      inputIsValid = true;
    } else {
      cout << "That input does not match the required format. Please try again." << endl;
    }
  }
  return input;
}

int receiveResult(SOCKET &socket, int *result) {
  char recvbuf[DEFAULT_BUFLEN];
  struct sockaddr_in ClientAddr;
  int clientAddrSize = (int)sizeof(ClientAddr);
  short port = 27015;
  const char* local_host = "127.0.0.1";
  ClientAddr.sin_family = AF_INET;
  ClientAddr.sin_port = htons(port);
  ClientAddr.sin_addr.s_addr = inet_addr(local_host);

  int bytesReceived = recvfrom(socket, recvbuf, DEFAULT_BUFLEN, 0, (SOCKADDR *) &ClientAddr, &clientAddrSize);
    if (bytesReceived > 0) {
      string temp = recvbuf;
      if (!temp.compare("INVALID_EXPRESSION")) {
        cout << "Response from server: " << temp << endl;
        return -1;
      }
      istringstream ss(temp);
      ss >> *result;
      return 1;
    } else if (bytesReceived == 0) {
      cout << "Connection has been closed from the server side." << endl;
      return 0;
    } else {
      cout << "An error occurred: " << WSAGetLastError() << endl;
      return 0;
    }
}

int main() {
  WSADATA wsaData;
  int iResult;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
      printf("WSAStartup failed: %d\n", iResult);
      return 1;
  }

  SOCKET ConnectSocket = initializeSocket(); 
  if (ConnectSocket == INVALID_SOCKET) {
    cout << "Socket initialization failed, shutting down..." << endl;
    return 1;
  }

  int recvbuflen = DEFAULT_BUFLEN;

  bool running = true;
  while (running) {
    string message = takeInput();
    if (!message.compare("quit")) {
      iResult = shutdown(ConnectSocket, SD_SEND);
      if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
      }
      running = false;
    } else {
      sendMessage(ConnectSocket, message);
      int result = 0;
      int status = receiveResult(ConnectSocket, &result);
      switch(status) {
        case 0:
          running = false;
          break;
        case 1:
          cout << message << " = " << result << endl;
          break;
        case -1:
          cout << "Invalid input. Please try again." << endl;
          break;
      }
    }
  }

  // shutdown the connection for sending since no more data will be sent
  // the client can still use the ConnectSocket for receiving data
  /* iResult = shutdown(ConnectSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
      printf("shutdown failed: %d\n", WSAGetLastError());
      closesocket(ConnectSocket);
      WSACleanup();
      return 1;
  }

  // Receive data until the server closes the connection
  do {
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
      printf("Bytes received: %d\n", iResult);
    else if (iResult == 0)
      printf("Connection closed\n");
    else
      printf("recv failed: %d\n", WSAGetLastError());
  } while (iResult > 0); */




  return 0;
}