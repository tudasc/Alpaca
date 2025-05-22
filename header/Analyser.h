#pragma once

#include <string>
#include <map>
#include <vector>
#include <clang/AST/RecursiveASTVisitor.h>
#include "../include/json.hpp"

using json = nlohmann::json;

namespace analyse{
    struct FunctionInstance {
        bool isDeclaration;
        std::string name;
        std::string qualifiedName;
        std::string returnType;
        // [type, [name, default value]]
        std::vector<std::pair<std::string, std::pair<std::string, std::string>>> params;
        std::string body;
        std::vector<std::string> location;
        std::vector<FunctionInstance> declarations;
        std::string filename;
        std::string scope;
        std::string storageClass;
        std::string memberFunctionSpecifier;
        std::string getAsString(){
            std::string output;
            output += "\n-----------------" + name + "-----------------\n";
            output += "Is a Declaration: " + isDeclaration;
            output += "Return Type:   " + returnType;
            output += "\nParams:        ";
            for (const auto &item: params){
                output += item.first + " " + item.second.first + " ";
                if(item.second.second != ""){
                    output += "= " + item.second.second + " ";
                }
            }
            output += "\nFull Location: ";
            for (const auto &item: location){
                output += item + "::";
            }
            output += "\nFilename:      " + filename;

            if(!isDeclaration){
                output += "\nHas declarations in:  ";
                for (const auto &item: declarations){
                    output += item.filename + "--";
                }
            }
            output += "\n";
            return output;
        };
        [[nodiscard]] bool isCorrectDeclaration(const FunctionInstance& decl) const {

            if (!decl.isDeclaration) {
                return false;
            }

            // check if the params match
            if (params.size() == decl.params.size()) {
                for (int i = 0; i < params.size(); ++i) {
                    if (params.at(i) != decl.params.at(i)) {
                        return false;
                    }
                }
            } else {
                return false;
            }

            // check if the rest of the functions match (qualified name check includes the location check)
            return (decl.qualifiedName == qualifiedName && decl.returnType == returnType && decl.scope == scope &&
                    storageClass == decl.storageClass);
        }

        [[nodiscard]] bool equals(FunctionInstance func) const{
            // check if the params match
            if (params.size() == func.params.size()) {
                for (int i = 0; i < params.size(); ++i) {
                    if (params.at(i) != func.params.at(i)) {
                        return false;
                    }
                }
            } else {
                return false;
            }

            return (func.qualifiedName == qualifiedName && func.returnType == returnType && func.scope == scope &&
                    storageClass == func.storageClass && isDeclaration == func.isDeclaration && memberFunctionSpecifier == func.memberFunctionSpecifier);
        }

    };

    class Analyser{

        public:
            void compareVersionsWithDoc(bool docEnabled, bool includePrivate);
            Analyser( const std::vector<FunctionInstance>& oldProgram, const std::vector<FunctionInstance>& newProgram, bool JSONOutput);

        private:
            std::pair<std::string, double> findBody(const FunctionInstance& oldBody, bool docEnabled);
            bool compareOverloadedFunctionHeader(const FunctionInstance& func);
            static bool compareFunctionHeader(const FunctionInstance&, const FunctionInstance&, bool internalUse);
            static bool compareFunctionHeaderExceptParams(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);
            static std::pair<FunctionInstance, double>findBody(const FunctionInstance& oldFunc, const std::vector<FunctionInstance>& funcSubset);
            static bool compareParams(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);
            static bool compareReturnType(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);
            static bool compareScope(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);
            static bool compareFile(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);
            static bool compareNamespaces(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);
            static bool compareDeclarations(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);
            static bool compareStorageClass(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);
            static bool compareFunctionSpecifier(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse);


            std::vector<FunctionInstance> oldProgram;
        std::vector<FunctionInstance> newProgram;

    };
}