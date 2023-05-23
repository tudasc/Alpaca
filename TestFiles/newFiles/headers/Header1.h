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
    class test{
        void moveIntoClass(int i);
    };
}

class switcher{
public:
    bool switchFileButNotClass (int x, int z, int j) const;
};

class test {
private:
    void somethingPrivate();
    void somethingPublic();
    virtual void virtualFunc();
};

namespace newNspace{
    void switchedNamespaceTest();
    class movingNamespace{
    public:
        int moveNamespace() const;
    };
}