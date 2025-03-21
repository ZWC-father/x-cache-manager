#include <iostream>
using namespace std;
int a[20] = {10, 11, 12, 14, 13, 12, 9, 10, 8, 6, 4, 3, 3, 2};
int main(){
    double wtf = 0.0f;
    double wtf2 = 0.0f;
    for(int i = 0; i < 14; i++){
        wtf += a[i] / 10.0f;
        cout << wtf << " ";
        wtf2 += a[i];
        cout << wtf2 / (10.0f * (i + 1)) << std::endl;
    }
    
    return 0;
}
