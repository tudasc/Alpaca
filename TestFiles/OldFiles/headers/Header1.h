#pragma once

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
    int newType;
    int newClass;
    int newScope;
    int newDefinition;
    int newConst = 10;
public:
    void somethingPublic();
protected:
    void somethingPrivate();
    virtual void virtualFunc();

};

class switcher{
    static int declarationFileChange;
public:
    bool switchFileButNotClass (int x, int z, int j) const;
};

namespace nspace {
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


