#pragma once

#include <string>
#include <map>
#include <vector>

namespace analyse{
    struct FunctionInstance {
        std::string name;
        std::string returnType;
        std::vector<std::string> params;
        std::string body;
        std::vector<std::string> location;
        std::string filename;
        std::string scope;
        std::string getAsString(){
            std::string output;
            output += "\n-----------------" + name + "-----------------\n";
            output += "Return Type:   " + returnType;
            output += "\nParams:        ";
            for (const auto &item: params){
                output += item + " ";
            }
            output += "\nFull Location: ";
            for (const auto &item: location){
                output += item + "::";
            }
            output += "\nFilename:      " + filename + "\n";
            return output;
        };
    };

    class Analyser{

        public:
            void compareVersions();
            Analyser( const std::multimap<std::string, FunctionInstance>& oldProgram, const std::multimap<std::string, FunctionInstance>& newProgram);

        private:
            void compareFunctionHeader(FunctionInstance func);
            void compareOverloadedFunctionHeader(FunctionInstance func);
            void compareFunctionHeaderExceptParams(FunctionInstance func, FunctionInstance newFunc);
            static std::pair<FunctionInstance, double>findBody(FunctionInstance oldFunc, const std::vector<FunctionInstance> funcSubset);
            std::pair<std::string, double> findBody(FunctionInstance oldBody);
            std::string compareParams(FunctionInstance func, FunctionInstance newFunc);
            std::string compareReturnType(FunctionInstance func, FunctionInstance newFunc);
            std::string compareScope(FunctionInstance func, FunctionInstance newFunc);
            std::string compareFile(FunctionInstance func, FunctionInstance newFunc);
            std::string compareNamespaces(FunctionInstance func, FunctionInstance newFunc);

            std::multimap<std::string, FunctionInstance> oldProgram;
            std::multimap<std::string, FunctionInstance> newProgram;

    };
}