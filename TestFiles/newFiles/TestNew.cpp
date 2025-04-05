
void differentNumberParams(int check){
    int i=0;
    i=i+10;
}

void newReturn(bool* one, int two) {
    two += 10;
    *one = false;
}

int itsRenamed(){
    int test = 0;
    int random = 4;
    return 0;
}

void paramChange(bool x){
    bool z = x;
}

namespace nspace
{
    class test{
        void Func(int i) {}
    };
}

int main() {
    differentNumberParams(5);
    bool test = false;
    bool* testPtr = &test;
    newReturn(testPtr, 20);
}
