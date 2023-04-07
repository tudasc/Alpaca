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
            findBody(func);
            if (newProgram.count(func.name) <= 0) {
                auto bodyStatus = findBody(func);
                if (bodyStatus.first == "") {
                    outs() << "The function \"" + func.name + "\" was most likely deleted\n";
                } else {
                    outs() << "The function \"" + func.name + "\" was renamed to \"" + bodyStatus.first + "\" with a code similarity of " + std::to_string(bodyStatus.second) + "%\n";
                }
            } else if (newProgram.count(func.name) == 1){
                compareFunctionHeader(func);
            } else {
                compareOverloadedFunctionHeader(func);
            }
        }
    }


    std::pair<std::string, double> Analyser::findBody(FunctionInstance oldFunc) {
        std::string currentHighest = "";
        double currentHighestValue = 0;
        for(auto const &newFunc : newProgram){
            double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc.second);

            if(percentageDifference >= percentageCutOff){
                if(percentageDifference > currentHighestValue){
                    currentHighestValue = percentageDifference;
                    currentHighest = newFunc.second.name;
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


    void Analyser::compareFunctionHeader(FunctionInstance func) {
        FunctionInstance newFunc = newProgram.find(func.name)->second;

        // compare the params without overloading
        outs() << compareParams(func, newFunc);

        compareFunctionHeaderExceptParams(func, newFunc);
    }

    void Analyser::compareOverloadedFunctionHeader(FunctionInstance func) {
        std::vector<FunctionInstance> overloadedFunctions;
        // check if there is an exact match
        for (const auto &item: newProgram){
            if(item.second.name == func.name){
                // function header is an exact match
                if(compareParams(func, item.second) == ""){
                    // proceed normally
                    compareFunctionHeaderExceptParams(func, item.second);
                    return;
                }
                overloadedFunctions.push_back(item.second);
            }
        }
        // if there isnt an exact match, find the nearest match
        auto closest = findBody(func, overloadedFunctions);
        if(closest.second != 0){
            // there is another function that fits, proceed to compare it normally
            outs()<< "DISCLAIMER: There is code similarity of " + std::to_string(closest.second) + "% of the old overloaded function and the in the following analyzed instance of the overloaded function\n";
            outs()<< compareParams(func, closest.first);
            compareFunctionHeaderExceptParams(func, closest.first);
        }else{
            outs()<<"The overloaded function " + func.name + " with the params " + helper::getAllParamsAsString(func.params) + " was deleted";
        }

    }

    void Analyser::compareFunctionHeaderExceptParams(FunctionInstance func, FunctionInstance newFunc){
        // compare the return type
        outs() << compareReturnType(func, newFunc);

        // TO-DO Compare the scope of the functions

    }

    std::string Analyser::compareParams(FunctionInstance func, FunctionInstance newFunc) {
        int numberOldParams = func.params.size();
        int numberNewParams = newFunc.params.size();
        std::string output = "";
        if (numberOldParams == numberNewParams) {
            for (std::vector<std::string>::iterator newIt = newFunc.params.begin(), oldIt = func.params.begin();
                 newIt != newFunc.params.end() && oldIt != func.params.end();
                 ++newIt, ++oldIt) {
                // TO-DO: Special Message if the order was simply changed
                if ((*oldIt) != (*newIt)) {
                    //TO-DO: maybe insert the parameter names additionally to the types?
                    output.append("The parameter type \"" + (*oldIt) + "\" of the function \"" + newFunc.name +
                                  "\" has changed to \"" + (*newIt) + "\"\n");
                }
            }
        } else {
            //TO-DO: insert a message, that shows which new Parameters where added / what the current stand is
            output.append("The function \"" + func.name + "\" has a new number of parameters. Instead of " +
                          std::to_string(numberOldParams) + " it now has " + std::to_string(numberNewParams) +
                          "\n");
        }
        return output;
    }

    std::string Analyser::compareReturnType(FunctionInstance func, FunctionInstance newFunc) {
        return newFunc.returnType != func.returnType ? "The function \"" + func.name +
                                                       "\" has a new return Type. Instead of \"" + func.returnType +
                                                       "\" it is now: \"" + newFunc.returnType + "\"\n" : "";
    }
}