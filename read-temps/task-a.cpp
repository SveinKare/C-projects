#include <iostream>
#include <string>

using namespace std;

int main() {
  const int length = 5;

  int lessThanTen = 0;
  int tenToTwenty = 0;
  int greaterThanTwenty = 0;

  cout << "Du skal skrive inn " << length << " temperaturer." << endl;
  for (int i = 1; i <= length; i++) {
    double temperature;
    string garbage;

    cout << "Temperatur nr " << i << ": ";
    cin >> temperature;

    if (!cin.good()) {
      cout << "Skriv inn et tall." << endl;
      cin.clear();
      getline(cin, garbage);
      i--;
      continue;
    }

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
