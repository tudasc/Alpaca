#pragma once

#include <string>
#include <map>
#include <vector>
#include <clang/AST/RecursiveASTVisitor.h>


namespace analyse{
    struct FunctionInstance {
        bool isDeclaration;
        std::string name;
        std::string qualifiedName;
        std::string returnType;
        std::vector<std::string> params;
        std::string body;
        std::vector<std::string> location;
        std::vector<const FunctionInstance*> declarations;
        std::string filename;
        std::string scope;
        std::string getAsString(){
            std::string output;
            output += "\n-----------------" + name + "-----------------\n";
            output += "Is a Declaration: " + isDeclaration;
            output += "Return Type:   " + returnType;
            output += "\nParams:        ";
            for (const auto &item: params){
                output += item + " ";
            }
            output += "\nFull Location: ";
            for (const auto &item: location){
                output += item + "::";
            }
            output += "\nFilename:      " + filename;

            if(!isDeclaration){
                output += "\nHas declarations in:  ";
                for (const auto &item: declarations){
                    output += item->filename + "--";
                }
            }
            output += "\n";
            return output;
        };
        bool isCorrectDeclaration(const FunctionInstance& decl) const {

            if(!decl.isDeclaration){
                return false;
            }

            // check if the params match
            if(params.size() == decl.params.size()){
                for (int i = 0; i < params.size(); ++i) {
                    if(params.at(i) != decl.params.at(i)){
                        return false;
                    }
                }
            } else {
                return false;
            }

            // check if the rest of the functions match (qualified name check includes the location check)
            return (decl.qualifiedName == qualifiedName && decl.returnType == returnType && decl.scope == scope);
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