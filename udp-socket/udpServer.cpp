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
#include <sstream>
#include <regex>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
atomic<int> connections = 0;

class Workers {
private:
  int threads;
  vector<thread> thread_list;
  list<function<void()>> tasks;
  mutex tasks_mutex;
  condition_variable cv;
  atomic<bool> running;

public:
  Workers(int arg) {
    threads = arg;
  }

  void start() {
    running = true;
    for (int i = 0; i < threads; i++) {
      thread_list.emplace_back([this] {
          while (running) {
            function<void()> task;
            {
              unique_lock<mutex> lock(tasks_mutex);
              while(tasks.empty()) {
                cv.wait(lock);
                if (!running) {
                  break;
                }
              }
              if(!tasks.empty()) {
                task = *tasks.begin();
                tasks.pop_front();
              } else {
                break;
              }
            }
            if(task) {
              task();
            }
            cv.notify_all();
          } 
      });
    }
  }

  void post(function<void()> func) {
    unique_lock<mutex> lock(tasks_mutex);
    tasks.emplace_back(func);
    cv.notify_one();
  }

  void stop() {
    {
      unique_lock<mutex> lock(tasks_mutex);
      while (!tasks.empty()) {
        cv.wait(lock);
      }
      running = false;
    }
    cv.notify_all();
    for (int i = 0; i < threads; i++) {
      thread_list[i].join();
    }
  }

  void post_timeout(function<void()> func, int timeout) {
    thread sleepThread([timeout, func = move(func), this]() {
      this_thread::sleep_for(chrono::milliseconds(timeout));
      unique_lock<mutex> lock(tasks_mutex);
      tasks.emplace_back(func);
      cv.notify_one(); 
    });
    sleepThread.detach();
  }

  void startSocketHandlers(SOCKET &socket) {
    for(int i = 0; i < threads; i++) {
      post([&socket] {
        while(true) {
          char recvbuf[512];
          int bytesReceived;
          sockaddr_in clientAddr;
          int clientAddrSize = sizeof(clientAddr);

          bytesReceived = recvfrom(socket, recvbuf, sizeof(recvbuf), 0, (sockaddr*)&clientAddr, &clientAddrSize);
          if (bytesReceived == SOCKET_ERROR) {
              printf("recvfrom failed: %d\n", WSAGetLastError());
              closesocket(socket);
              WSACleanup();
              return;
          }
          string expression = recvbuf;

          string response;
          try {
            istringstream ss(expression);
            int number1 = 0;
            int number2 = 0;
            int result = 0;
            char op;
            ss >> number1 >> op >> number2;
            switch(op) {
              case '+':
                result = number1 + number2;
                break;
              case '-':
                result = number1 - number2;
                break;
            }
            response = to_string(result);
          } catch (...) {
            response = "INVALID_EXPRESSION";
          }

          char sendbuf[response.length()];
          strcpy(sendbuf, response.c_str());

          int bytesSent = sendto(socket, sendbuf, response.length(), 0, (SOCKADDR *) &clientAddr, clientAddrSize);
          if (bytesSent == SOCKET_ERROR) {
            cout << "An error occurred during sending: " << WSAGetLastError() << endl;
            return;
          }
        }
      });
    }
  }
};


int main() {
  WSADATA wsaData;
  Workers workers(4);
  workers.start();

  int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult) {
    cout << "An error occurred." << endl;
  }

  #define DEFAULT_PORT "27015"

  struct addrinfo *result = NULL, *ptr = NULL, hints;

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_flags = AI_PASSIVE;

  // Resolve the local address and port to be used by the server
  iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
    printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    return 1;
  }

  SOCKET ListenSocket = INVALID_SOCKET;
  // Create a SOCKET for the server to listen for client connections

  ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (ListenSocket == INVALID_SOCKET) {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  // Setup the UDP listening socket
  iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  freeaddrinfo(result);

  workers.startSocketHandlers(ListenSocket);

  workers.stop();

  return 0;
}