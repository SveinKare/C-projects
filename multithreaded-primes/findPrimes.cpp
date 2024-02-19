#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <bits/stdc++.h>
#include <stdexcept>

using namespace std;

//Finds all primes between two numbers, including the from value itself (but not the to value)
void find_primes(int from, int to, vector<int> &primes, mutex &primes_lock) {
  for (int current = from; current < to; current++) {
    if (current == 1) { //Special case that is not considered prime.
      continue;
    }
    if (current % 2 != 0 || current == 2) { // No point in checking if the number is even, and 2 is an edge case of an "even" prime number
      bool isPrime = true;
      for (int i = 3; i < current; i += 2) {
        if (current % i == 0) {
          isPrime = false; // A factor has been found, number cannot be prime.
          break; 
        }
      }
      if (isPrime) {
        primes_lock.lock();
        primes.push_back(current);
        primes_lock.unlock();
      }
    }
  }
}

//Does not include the from or to values. 
void find_all_primes(int from, int to, int threads, vector<int> &primes, mutex &primes_lock) {
  from++;
  vector<thread> allThreads;

  int workLoad = (to-from)/threads;
  int rest = (to-from)%threads;

  if (workLoad == 0) {
    throw invalid_argument("Amount of threads cannot exceed the from-to range.");
  }

  for (int k = from, i = 0; k + rest < to; k += workLoad, i++) {
    if (k + workLoad + rest == to) {
      allThreads.push_back(thread(find_primes, k, k+workLoad+rest, ref(primes), ref(primes_lock)));
    } else {
      allThreads.push_back(thread(find_primes, k, k+workLoad, ref(primes), ref(primes_lock)));
    }
  }
  for (int i = 0; i < threads; i++) {
    allThreads[i].join();
  }
}

int main() {
  vector<int> primes;
  mutex primes_lock;

  find_all_primes(0, 100, 5, primes, primes_lock);

  sort(primes.begin(), primes.end());

  for(int i = 0; i+1 < primes.size(); i++) {
    if (!(primes[i] < primes[i+1])) {
      cout << "Problem" << endl;
    }
  }

  for(const auto prime : primes) {
    cout << prime << endl;
  }
}
