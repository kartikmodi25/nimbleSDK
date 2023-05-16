#include <iostream>
#include <string>
#include <vector>
using namespace std;

int main(int argc, char* argv[]) {
  string secretKey = argv[1];
  if (secretKey.size() % 8 != 0) {
    cout << "secretKey " << secretKey << " should be a multiple of 8" << endl;
    return 0;
  }
  int numLongs = secretKey.size() / 8;
  vector<int64_t> secVec;
  cout << "///////////////" << endl;
  cout << "vector<int64_t> secVec = {";
  for (int i = 0; i < numLongs; i++) {
    secVec.push_back(*((int64_t*)&secretKey[i * 8]));
    cout << secVec[i] << ",";
  }
  cout << "};" << endl;

  char* defaultSecretKey = new char[32];
  int64_t* d1 = (int64_t*)defaultSecretKey;
  for (int i = 0; i < numLongs; i++) {
    *d1 = secVec[i];
    ++d1;
  }
  cout << "///////////////" << endl;
  cout << defaultSecretKey << endl;
  cout << secretKey << endl;
}
