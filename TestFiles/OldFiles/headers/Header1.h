#pragma once

template <typename T, class B> T specAdded(T x, B y);
template <typename T> T specAdded(T x, T y, T z);
template <typename T, typename C> T specDeleted (T x, C y);
template <typename T> T templateParamAdded(T x, T y);
template <typename T, typename Z> T templateParamDeleted(T x, Z y);

void differentNumberParams();

int newReturn(bool* one, int two);

int removed();

static int rename();

extern const void paramChange(int x, bool z, int p = 10);

double switchHeaderFile(int a);

void overloaded(int x, int y);

void overloaded(int x, int y, int z);

void overloaded(int x);

int overloaded(int w, bool z, bool y);

static int secTest();

class test {
public:
    int newType;
    int newClass;
    int newScope;
    int newDefinition;
    int newConst = 10;
    void somethingPublic();
protected:
    void somethingPrivate();
    virtual void virtualFunc();

};

class switcher{
public:
    static int definitionFileChange;
    bool switchFileButNotClass (int x, int z, int j) const;
};

namespace nspace {
    int sameName;
    void moveIntoClass(int i, bool x);
    int static switchingFromStatic = 10;
    int extern switchingToStatic;
    void switchedNamespaceTest();

    class movingNamespace {
    public:
        int moveNamespace();
    };
}
struct testStruct{
    int deleted = 10;
};


