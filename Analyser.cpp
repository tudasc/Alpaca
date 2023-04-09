#include "header/Analyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/CodeMatcher.h"
#include "header/HelperFunctions.h"
#include <cmath>


using namespace llvm;
using namespace analyse;

const double percentageCutOff = 90;

namespace analyse{

    Analyser::Analyser(const std::multimap<std::string, FunctionInstance>& oldProgram,
                       const std::multimap<std::string, FunctionInstance>& newProgram) {
        this->oldProgram = oldProgram;
        this->newProgram = newProgram;
    }

    void Analyser::compareVersions() {
        for (auto const &x: oldProgram) {
            FunctionInstance func = x.second;
            // skip this function if the old instance was private, because it couldnt have been used by anyone
            if(func.scope == "private" || func.name == "main"){
                continue;
            }

            if (newProgram.count(func.qualifiedName) <= 0) {
                auto bodyStatus = findBody(func);
                // only output a renaming, if the similar function is not private, because knowing about a private function is not useful to the user
                if (bodyStatus.first == ""){
                    outs() << "The function \"" + func.name + "\" was most likely deleted\n";
                } else {
                    // use the function found during the statistical analysis
                    FunctionInstance newFunc = newProgram.find(bodyStatus.first)->second;
                    if(newFunc.name != func.name){
                        // further checks on the Header to ensure, that this is indeed a renamed function
                        if(compareFunctionHeader(func, newFunc) == ""){
                            outs() << "The function \"" + func.name + "\" was renamed to \"" + newFunc.name +
                                      "\" with a code similarity of " + std::to_string(bodyStatus.second) + "%\n";
                        } else {
                            outs() << "The function \"" + func.name + "\" was most likely deleted\n";
                        }
                    } else{
                        outs() << compareFunctionHeader(func, newFunc);
                    }
                }
            } else if (newProgram.count(func.qualifiedName) == 1){
                FunctionInstance newFunc = newProgram.find(func.qualifiedName)->second;
                outs() << compareFunctionHeader(func, newFunc);
            } else {
                outs() << compareOverloadedFunctionHeader(func);
            }
            outs() << "----------------------------------------------------------\n";
        }
    }


    std::pair<std::string, double> Analyser::findBody(FunctionInstance oldFunc) {
        std::string currentHighest = "";
        double currentHighestValue = 0;
        for(auto const &newFunc : newProgram){
            double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc.second);

            // prioritize functions that have the exact same name and header
            // TODO: ask to make sure this is the correct way to handle this
            if (oldFunc.name == newFunc.second.name){
                return std::make_pair(newFunc.second.qualifiedName, percentageDifference);
            }

            if(percentageDifference >= percentageCutOff){
                if(percentageDifference > currentHighestValue){
                    currentHighestValue = percentageDifference;
                    currentHighest = newFunc.second.qualifiedName;
                }
            }
        }

        return std::make_pair(currentHighest, currentHighestValue);
    }

    std::pair<FunctionInstance, double> Analyser::findBody(FunctionInstance oldFunc, const std::vector<FunctionInstance> funcSubset){
        FunctionInstance currentHighest;
        double currentHighestValue = 0;
        for(auto const &newFunc : funcSubset){
            double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc);

            if(percentageDifference >= percentageCutOff){
                if(percentageDifference > currentHighestValue){
                    currentHighestValue = percentageDifference;
                    currentHighest = newFunc;
                }
            }
        }

        return std::make_pair(currentHighest, currentHighestValue);
    }


    std::string Analyser::compareFunctionHeader(FunctionInstance func, FunctionInstance newFunc) {
        std::string output = "";
        // compare the params without overloading
        output += compareParams(func, newFunc);

        output += compareFunctionHeaderExceptParams(func, newFunc);
        return output;
    }

    std::string Analyser::compareOverloadedFunctionHeader(FunctionInstance func) {
        std::vector<FunctionInstance> overloadedFunctions;
        std::string output = "";
        // check if there is an exact match
        for (const auto &item: newProgram){
            if(item.second.qualifiedName == func.qualifiedName){
                // function header is an exact match
                if(compareParams(func, item.second) == ""){
                    // proceed normally
                    return compareFunctionHeaderExceptParams(func, item.second);
                }
                overloadedFunctions.push_back(item.second);
            }
        }
        // if there isnt an exact match, find the nearest match
        auto closest = findBody(func, overloadedFunctions);
        if(closest.second != 0){
            // there is another function that fits, proceed to compare it normally
            output += "DISCLAIMER: There is code similarity of " + std::to_string(closest.second) + "% between the old overloaded function and the in the following analyzed instance of the overloaded function\n";
            output += compareParams(func, closest.first);
            output += compareFunctionHeaderExceptParams(func, closest.first);
        }else{
            output += "The overloaded function " + func.name + " with the params " + helper::getAllParamsAsString(func.params) + " was deleted";
        }
        return output;

    }

    std::string Analyser::compareFunctionHeaderExceptParams(FunctionInstance func, FunctionInstance newFunc){
        std::string output = "";

        output += compareReturnType(func, newFunc);

        output += compareScope(func, newFunc);

        output += compareNamespaces(func, newFunc);

        output += compareFile(func, newFunc);

        return output;
    }

    std::string Analyser::compareParams(FunctionInstance func, FunctionInstance newFunc) {
        int numberOldParams = func.params.size();
        int numberNewParams = newFunc.params.size();
        std::string output = "";
        if (numberOldParams == numberNewParams) {
            for (std::vector<std::string>::iterator newIt = newFunc.params.begin(), oldIt = func.params.begin();
                 newIt != newFunc.params.end() && oldIt != func.params.end();
                 ++newIt, ++oldIt) {
                if ((*oldIt) != (*newIt)) {
                    output.append("The parameter type \"" + (*oldIt) + "\" of the function \"" + newFunc.name +
                                  "\" has changed to \"" + (*newIt) + "\"\n");
                }
            }
        } else {
            output.append("The function \"" + func.name + "\" has a new number of parameters. Instead of " +
                          std::to_string(numberOldParams) + " it now has " + std::to_string(numberNewParams) +
                          "--> " + helper::getAllParamsAsString(newFunc.params) +
                          "\n");
        }
        return output;
    }

    std::string Analyser::compareReturnType(FunctionInstance func, FunctionInstance newFunc) {
        return newFunc.returnType != func.returnType ? "The function \"" + func.name +
                                                       "\" has a new return Type. Instead of \"" + func.returnType +
                                                       "\" it is now: \"" + newFunc.returnType + "\"\n" : "";
    }

    std::string Analyser::compareScope(FunctionInstance func, FunctionInstance newFunc){
        return (func.scope != newFunc.scope) ? "The function " + func.name + " is now " + newFunc.scope + "\n" : "";
    }

    std::string Analyser::compareFile(FunctionInstance func, FunctionInstance newFunc){
        return (func.filename != newFunc.filename) ? "The function " + func.name + " has moved from the file " +  func.filename + " to the file " + newFunc.filename + "\n" : "";
    }

    std::string Analyser::compareNamespaces(FunctionInstance func, FunctionInstance newFunc){
        if(func.location.size() != newFunc.location.size()){
            return "The function " + func.name + " moved from " + helper::getAllNamespacesAsString(func.location) + " to " + helper::getAllNamespacesAsString(newFunc.location) + "\n";
        }
        for(int i=0;i<func.location.size();i++){
            if(func.location.at(i) != newFunc.location.at(i)){
                return "The function " + func.name + " moved from " + helper::getAllNamespacesAsString(func.location) + " to " + helper::getAllNamespacesAsString(newFunc.location) + "\n";
            }
        }
        return "";
    }
}