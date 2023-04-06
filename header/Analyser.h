#pragma once

#include <string>
#include <map>
#include <vector>

namespace analyse{
    struct FunctionInstance {
        std::string name;
        std::string returnType;
        std::vector<std::string> params;
        std::vector<std::string> stmts;
        // TO-DO: include the current namespace of the function
    };

    class Analyser{

        public:
            void compareVersions();
            Analyser(std::map<std::string, FunctionInstance> oldProgram, std::map<std::string, FunctionInstance> newProgram);

        private:
            void compareFunctionHeader(FunctionInstance func);
            std::string findBody(FunctionInstance oldBody);
            std::string compareParams(FunctionInstance func, FunctionInstance newFunc);
            std::string compareReturnType(FunctionInstance func, FunctionInstance newFunc);

            std::map<std::string, FunctionInstance> oldProgram;
            std::map<std::string, FunctionInstance> newProgram;

    };
}