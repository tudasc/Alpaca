#include "headers/Header1.h"
#include "headers/Header2.h"

int someValue = 500;

template <typename T, class B> T specAdded(T x, B y)
{
    return (x > y) ? x : y;
}

// overloading possible
template <typename T> T specAdded(T x, T y, T z){
    return x;
}

template <> int specAdded(int x, int y){
    return 20;
}

template <typename T, typename C> T specDeleted (T x, C y){
    return x;
}

void differentNumberParams(int check);

void differentNumberParams(int check){
    int i=0;
    i=i+10;
}

void newReturn(bool* one, int two) {
    two += 10;
    *one = false;
}

double switchHeaderFile(int a){
    return 0.0;
}

static int itsRenamed(){
    int two = 0;
    int random = 4;
    return 0;
}

void paramChange(bool x, bool z, int w, int p){
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

int test::newDefinition = 20;

namespace nspace
{
    void test::moveIntoClass(int i) {
        int b = i + b;
        int v = b + i;}
}

void test::somethingPrivate(){
    int x = 10;
}
void test::somethingPublic(){
    int i = 10;
}

void test::virtualFunc() {
    int x=10;
}

namespace newNspace{
    const inline extern unsigned int namespaceVariable = 200;
    void switchedNamespaceTest(){
        int x = 10+10;
    }
    int movingNamespace::moveNamespace() const {return 0;}
}

int main() {
    auto smt = specAdded<int,int>(1, 3);
    differentNumberParams(5);
    bool test = false;
    bool* testPtr = &test;
    newReturn(testPtr, 20);
}
