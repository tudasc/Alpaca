#include "header/Analyser.h"
#include <llvm/Support/CommandLine.h>


using namespace llvm;
using namespace analyse;

namespace analyse{

    Analyser::Analyser(const std::multimap<std::string, FunctionInstance>& oldProgram,
                       const std::multimap<std::string, FunctionInstance>& newProgram) {
        this->oldProgram = oldProgram;
        this->newProgram = newProgram;
    }

    void Analyser::compareVersions() {
        for (auto const &x: oldProgram) {
            FunctionInstance func = x.second;
            if (newProgram.count(func.name) <= 0) {
                std::string bodyStatus = findBody(func);
                if (bodyStatus == "") {
                    outs() << "The function \"" + func.name + "\" was deleted\n";
                } else {
                    outs() << "The function \"" + func.name + "\" was renamed to \"" + bodyStatus + "\"\n";
                }
            } else {
                compareFunctionHeader(func);
            }
        }
    }

    std::string Analyser::findBody(FunctionInstance oldFunc) {
        for (auto const &x: newProgram) {
            FunctionInstance func = x.second;
            if (func.stmts.size() != oldFunc.stmts.size()) continue;
            std::string matchingFunction = func.name;
            for (std::vector<std::string>::iterator newIt = func.stmts.begin(), oldIt = oldFunc.stmts.begin();
                 newIt != func.stmts.end() && oldIt != oldFunc.stmts.end();
                 ++newIt, ++oldIt) {
                if ((*newIt) != (*oldIt)) {
                    matchingFunction = "";
                }
            }
            if (matchingFunction != "" && compareReturnType(oldFunc, func) == "" &&
                compareParams(oldFunc, func) == "") {
                return matchingFunction;
            }
        }
        return "";
    }

    void Analyser::compareFunctionHeader(FunctionInstance func) {
        FunctionInstance newFunc = newProgram.find(func.name)->second;

        std::vector<FunctionInstance> overloadedFuncs;
        for (auto const &f: newProgram) {
            if (f.first == func.name) {
                overloadedFuncs.push_back(f.second);
            }
        }

        if (overloadedFuncs.size() > 1) {
            outs()<<"\n Here an overloaded function was handled\n";
        } else {
            // compare the Params
            outs() << compareParams(func, newFunc);
        }

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