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



int initializeServer(SOCKET &ConnectSocket) {
  int iResult;

  struct addrinfo *result = NULL,
                *ptr = NULL,
                hints;

  ZeroMemory( &hints, sizeof(hints) );
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  #define DEFAULT_PORT "27015"

  // Resolve the server address and port
  iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
      printf("getaddrinfo failed: %d\n", iResult);
      WSACleanup();
      return 1;
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
    return 1;
  }
  // Connect to server.
  iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
      closesocket(ConnectSocket);
      ConnectSocket = INVALID_SOCKET;
  }

  // Should really try the next address returned by getaddrinfo
  // if the connect call failed
  // But for this simple example we just free the resources
  // returned by getaddrinfo and print an error message

  freeaddrinfo(result);

  if (ConnectSocket == INVALID_SOCKET) {
      printf("Unable to connect to server!\n");
      WSACleanup();
      return 1;
  }
  return 0;
}


void sendMessage(SOCKET &socket, string message) {
  char sendbuf[message.length()];
  strcpy(sendbuf, message.c_str());

  // Send an initial buffer
  int iResult = send(socket, sendbuf, (int) strlen(sendbuf), 0);
  if (iResult == SOCKET_ERROR) {
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
  int bytesReceived = recv(socket, recvbuf, DEFAULT_BUFLEN, 0);
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

  SOCKET ConnectSocket; 
  int error = initializeServer(ConnectSocket);
  if (error) {
    cout << "An error occurred." << endl;
    return 1;
  }


  int recvbuflen = DEFAULT_BUFLEN;


  bool running = true;
  while (running) {
    string message = takeInput();
    if (!message.compare("quit")) {
      iResult = shutdown(ConnectSocket, SD_BOTH);
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
  return 0;
}