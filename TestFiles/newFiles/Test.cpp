#include "headers/Test.h"

void differentNumberParams(int check){
    int i=0;
    i=i+10;
}

void newReturn(bool* one, int two) {
    two += 10;
    *one = false;
}

int itsRenamed(){
    int two = 0;
    int random = 4;
    return 0;
}

void paramChange(bool x, bool z, int w, int p = 20){
    bool fe = x;
}

void overloaded(int x, int y){
    int combine = x + y;
    combine = combine * 2;
}

void overloaded(int x, int y, int z){

}

int overloaded(int w, bool z, int y, bool i){
    int b = w + z + y + i;
    return b;
}

namespace nspace
{
    class test{
        void Func(int i) {}
    };
}

class test {
private:
    void somethingPrivate(){
        int x = 10;
    }
    void somethingPublic(){
        int i = 10;
    }
};

namespace newNspace{
    class movingNamespace{
    public:
        void moveNamespace() {}
    };
}

int main() {
    differentNumberParams(5);
    bool test = false;
    bool* testPtr = &test;
    newReturn(testPtr, 20);
}
