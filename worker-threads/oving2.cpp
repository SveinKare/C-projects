#include <thread>
#include <vector>
#include <mutex>
#include <list>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <iostream>


using namespace std;

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
        while(!tasks.empty()) {
          cv.wait(lock);
        }
        running = false;
      }
      cv.notify_all();
      for(int i = 0; i < threads; i++) {
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
};

function<void()> funcBuilder(string text) {
  return [text]() {
    cout << text << endl;
  };
}

int main() {
    Workers w(4);
    Workers event_loop(1);
    for (int i = 0; i < 10; i++) {
      w.post([] {
        this_thread::sleep_for(chrono::milliseconds(5000));
        cout << "Done (w)\n";
      });
    }
    for (int i = 0; i < 10; i++) {
      event_loop.post([] {
        this_thread::sleep_for(chrono::milliseconds(1000));
        cout << "Done (event_loop)\n";
      });
    }

    w.post_timeout(funcBuilder("Hello from Thread 1"), 5000);
    w.post_timeout(funcBuilder("Hello from Thread 2"), 1000);
    w.post_timeout(funcBuilder("Hello from Thread 3"), 2000);
    w.post_timeout(funcBuilder("Hello from Thread 4"), 3000);

    
    w.start();
    event_loop.start();

    w.stop();
    cout << "Stopped w" << endl;
    event_loop.stop();
    cout << "Stopped event_loop" << endl;

    cout << "Finished successfully!" << endl;
    return 0;
}