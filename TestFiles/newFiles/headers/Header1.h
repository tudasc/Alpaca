#include <stdbool.h>

template <typename T, class B> T specAdded(T x, B y);
template <typename T, typename C> T specDeleted (T x, C y);
template <typename T, typename X> X templateParamAdded(T x, X y);
template <typename T> T templateParamDeleted(T x);

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
    int ignoredPrivate;
public:
    int newClass;
    static int definitionFileChange;
    bool switchFileButNotClass (int x, int z, int j) const;
};

class test {
public:
private:
    bool newType;
    const int newConst = 20;
    int newScope;
    int static newDefinition;
    void somethingPrivate();
    void somethingPublic();
    virtual void virtualFunc();
};

namespace newNspace{
    void switchedNamespaceTest();
    class movingNamespace{
    public:
        int sameName;
        int moveNamespace() const;
    };
}

struct testStruct{
    int deleted = 10;
};