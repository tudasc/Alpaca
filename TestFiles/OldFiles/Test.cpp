#include<stdio.h>
void differentNumberParams(){
    int i=0;
    i=i+10;
}

int newReturn(bool* one, int two) {
    two += 10;
    //random comment
    // something something // hallo // was // passiert // hier /*
    *one = false;
    /*
     gigantischer Code Abschnitt
     meine
     Freund
     */
    return two;
}

int removed(){
    return 0;
}

int rename(){
    int test = 0;
    int random = 4;
    return 0;
}

void paramChange(int x){
    int z = x*x;
}


void overloaded(int x, int y){

}

void overloaded(int x, int y, int z){

}

void overloaded(int x){

}

class test {
public:
    void somethingPublic(){
        int i = 10;
    }
private:
    void somethingPrivate(){
        int x = 10;
    }
};

namespace nspace
{
    void Func(int i) {}
}

int main() {
    differentNumberParams();
    bool test = false;
    bool* testPtr = &test;
    int smth = newReturn(testPtr, 20);
}
