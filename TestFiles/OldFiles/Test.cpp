#include <stdio.h>
#include "headers/Header1.h"
#include "headers/Header2.h"

int someValue = 200;

bool definitionFileChange = true;

void differentNumberParams(){
    int i=0;
    i=i+10;
}

int newReturn(bool* one, int two) {
    two += 10;
    //random comment
    // something something // hello //*
    *one = false;
    /*
     gigantischer Code Abschnitt



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

extern const void paramChange(int x, bool z, int p){
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

void test::somethingPublic() {
    int i = 10;
}

void test::somethingPrivate(){
    int x = 10;
}

bool switcher::switchFileButNotClass (int x, int z, int j) const{
    return true;
}

double switchHeaderFile(int a){
    return 0.0;
}

int switcher::definitionFileChange = 200;

namespace nspace
{
    const inline extern unsigned int namespaceVariable = 200;
    void moveIntoClass(int i, bool x) {
        int b = i + b;
        int v = b + i;
    }
    void switchedNamespaceTest(){
                int x = 10+10;
    }
    int movingNamespace::moveNamespace() {return 0;}
}

class otherClass{
    const int otherClass=20;
};

int main() {
    differentNumberParams();
    bool test = false;
    bool* testPtr = &test;
    int smth = newReturn(testPtr, 20);
}
