#pragma once

#include <string>
#include <map>
#include <vector>

namespace analyse{
    struct FunctionInstance {
        std::string name;
        std::string qualifiedName;
        std::string returnType;
        std::vector<std::string> params;
        std::string body;
        std::vector<std::string> location;
        std::string filenameOfDeclaration;
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
        bool isCorrectDeclaration(const FunctionInstance& decl){
            // TODO: implement logic that checks if the given FunctionInstance is the Declaration of this Instance
            return false;
        }
    };

    struct file {
        std::string name;
        std::string fullPath;
        std::vector <FunctionInstance> functions;
    };

    class Analyser{

        public:
            void compareVersionsWithDoc(bool docEnabled);
            Analyser( const std::multimap<std::string, FunctionInstance>& oldProgram, const std::multimap<std::string, FunctionInstance>& newProgram);

        private:
            std::pair<std::string, double> findBody(const FunctionInstance& oldBody, bool docEnabled);
            std::string compareOverloadedFunctionHeader(const FunctionInstance& func);
            static std::string compareFunctionHeader(const FunctionInstance&, const FunctionInstance&);
            static std::string compareFunctionHeaderExceptParams(const FunctionInstance& func, const FunctionInstance& newFunc);
            static std::pair<FunctionInstance, double>findBody(const FunctionInstance& oldFunc, const std::vector<FunctionInstance>& funcSubset);
            static std::string compareParams(const FunctionInstance& func, const FunctionInstance& newFunc);
            static std::string compareReturnType(const FunctionInstance& func, const FunctionInstance& newFunc);
            static std::string compareScope(const FunctionInstance& func, const FunctionInstance& newFunc);
            static std::string compareFile(const FunctionInstance& func, const FunctionInstance& newFunc);
            static std::string compareNamespaces(const FunctionInstance& func, const FunctionInstance& newFunc);

            std::multimap<std::string, FunctionInstance> oldProgram;
            std::multimap<std::string, FunctionInstance> newProgram;

    };
}