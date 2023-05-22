#include <stdio.h>
#include "headers/Test.h"
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

static int rename(){
    int test = 0;
    int random = 4;
    return 0;
}

extern const void paramChange(int x, bool z, int p = 10){
    int fe = x*x;
}


void overloaded(int x, int y){

}

void overloaded(int x, int y, int z){
    int v = x + y + z;
    bool u = true;
}

void overloaded(int x){

}

int overloaded(int w, bool z, bool y){
    int b = w + z + y;
    return b;
}

static int secTest(){return 11;}

class test {
public:
    void somethingPublic() {
        int i = 10;
    }
protected:
    void somethingPrivate(){
        int x = 10;
    }
    virtual void virtualFunc();

};
class switcher{
    public:
        bool switchFileButNotClass (int x, int z, int j) const{
            return true;
        }
};

namespace nspace
{
    void Func(int i, bool x) {
        int b = i + b;
        int v = b + i;
    }
    void switchedNamespaceTest(){
                int x = 10+10;
    }
    class movingNamespace{
    public:
        int moveNamespace() {return 0;}
    };
}

int main() {
    differentNumberParams();
    bool test = false;
    bool* testPtr = &test;
    int smth = newReturn(testPtr, 20);
}
