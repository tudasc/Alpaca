#include <stdbool.h>

void differentNumberParams(int check);

void newReturn(bool* one, int two);

static int itsRenamed();

void paramChange(bool x, bool z, int w, int p = 20);

void overloaded(int x, int y);

void overloaded(int x, int y, int z);

int overloaded(int w, bool z, int y, bool i);

namespace nspace
{

    int extern switchingFromStatic;
    int static switchingToStatic;

    class test{
        void moveIntoClass(int i);
    };
}

class switcher{
    int newClass;
    static int definitionFileChange;
public:
    bool switchFileButNotClass (int x, int z, int j) const;
};

class test {
    bool newType;
    const int newConst = 20;
public:
    int newScope;
    int static newDefinition;
private:
    void somethingPrivate();
    void somethingPublic();
    virtual void virtualFunc();
};

namespace newNspace{
    int sameName;
    void switchedNamespaceTest();
    class movingNamespace{
    public:
        int moveNamespace() const;
    };
}

struct testStruct{
    int deleted = 10;
};