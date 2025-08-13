#pragma once

#include <string>
#include <map>
#include <vector>
#include <clang/AST/RecursiveASTVisitor.h>
#include "../include/json.hpp"

namespace functionanalysis{
    class FunctionInstance {
    public:
        bool isDeclaration;
        std::string name;
        std::string qualifiedName;
        std::string returnType;
        // [type, [name, default value]]
        std::vector<std::pair<std::string, std::pair<std::string, std::string>>> params;
        std::string body;
        std::vector<std::string> location;
        std::string filePosition;
        std::vector<FunctionInstance> declarations;
        std::string filename;
        std::string scope;
        std::string storageClass;
        std::string memberFunctionSpecifier;
        std::string fullHeader;
        bool isConst;
        bool isTemplateDecl;
        bool isTemplateSpec;
        std::vector<FunctionInstance> templateSpecializations;
        // [type, [name, default value]]
        std::vector<std::pair<std::string, std::pair<std::string, std::string>>> templateParams;
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
                    if (params.at(i).first != decl.params.at(i).first) {
                        return false;
                    }
                }
            } else {
                return false;
            }

            // check if the rest of the functions match (qualified name check includes the location check)
            return (decl.qualifiedName == qualifiedName && decl.returnType == returnType && decl.scope == scope &&
                    storageClass == decl.storageClass && memberFunctionSpecifier == decl.memberFunctionSpecifier);
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
}