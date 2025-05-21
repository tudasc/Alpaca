#include "headers/Test.h"

void differentNumberParams(int check){
    int i=0;
    i=i+10;
}

void newReturn(bool* one, int two) {
    two += 10;
    *one = false;
}

static int itsRenamed(){
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
        void Func(int i) {
            int b = i + b;
            int v = b + i;}
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
    virtual void virtualFunc();
};

namespace newNspace{
    void switchedNamespaceTest(){
        int x = 10+10;
    }
    class movingNamespace{
    public:
        int moveNamespace() const {return 0;}
    };
}

int main() {
    differentNumberParams(5);
    bool test = false;
    bool* testPtr = &test;
    newReturn(testPtr, 20);
}
