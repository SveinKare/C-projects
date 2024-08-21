#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std;

void read_temperatures(double temperatures[], int length);

int main() {
  const int length = 5;

  int lessThanTen = 0;
  int tenToTwenty = 0;
  int greaterThanTwenty = 0;

  double temperatures[length];
  read_temperatures(temperatures, length);

  for (int i = 0; i < length; i++) {
    double temperature = temperatures[i];

    if (temperature < 10) {
      lessThanTen++;
    } else if (temperature > 20) {
      greaterThanTwenty++;
    } else {
      tenToTwenty++;
    }
  }

  cout << "Antall under 10 er " << lessThanTen << endl;
  cout << "Antall mellom 10 og 20 er " << tenToTwenty << endl;
  cout << "Antall over 20 er " << greaterThanTwenty << endl;

  return 0;
}

void read_temperatures(double temperatures[], int length) {
  const char filename[] = "../test_data.txt";
  ifstream file;
  file.open(filename);
  if (!file) {
    cout << "Filen test_data.txt må være til stede i prosjektmappen, og må ha " 
      << length 
      << " tall separert med mellomrom eller linjeskift." << endl;
    exit(EXIT_FAILURE);
  }

  int numbersRead = 0;
  double temperature;
  while (file >> temperature) {
    if (numbersRead+1 > length) { // Read numbers exceed array length
      break;
    }
    temperatures[numbersRead] = temperature;
    numbersRead++;
  }
  file.close();

  if (numbersRead < length) {
    cout << "Fant ikke nok tall. Forventet " << length << ", men fant bare " << numbersRead << endl;
    exit(EXIT_FAILURE);
  }
}
