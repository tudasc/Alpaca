#include "header/FunctionAnalyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/CodeMatcher.h"
#include "header/HelperFunctions.h"
#include "header/OutputHandler.h"
#include <vector>


using namespace llvm;
using namespace functionanalysis;
using namespace std;

namespace functionanalysis{

    class Analyser {
        std::vector<FunctionInstance> oldProgram;
        std::vector<FunctionInstance> newProgram;
        OutputHandler* outputHandler;

        const double percentageCutOff = 90;

        bool checkIfADeclarationMatches(const FunctionInstance& oldDecl, const FunctionInstance& newDecl){
            for (const auto &oldItem: oldDecl.declarations){
                for (const auto &newItem: newDecl.declarations){
                    if(oldItem.filename == newItem.filename){
                        return true;
                    }
                }
                //if the given FunctionInstances are declarations it has to lack a viable definition which is a separate case
                if(newDecl.isDeclaration && oldItem.filename == newDecl.filename){
                    return true;
                }
            }
            return false;
        }

        bool isFunctionOverloaded(const FunctionInstance& oldFunc, const FunctionInstance& newFunc){
            // overloaded Functions have to have the same qualified name (i.e. be in the same namespace) and share at least one declaration position (definitions don´t have to be in the same file) : if one of the declarations is empty, the definition is the declaration
            return oldFunc.qualifiedName == newFunc.qualifiedName && checkIfADeclarationMatches(oldFunc, newFunc);
        }

        int findFunction(const std::vector<FunctionInstance>& set, const std::string& qualifiedName){
            int tempSafe = -1;
            for(int i=0;i<set.size();i++){
                if(set.at(i).qualifiedName == qualifiedName){
                    // prefer definitions to declarations if one is found
                    if(!set.at(i).isDeclaration){
                        return i;
                    }else{
                        tempSafe = i;
                    }
                }
            }

            return tempSafe;
        }

        int countFunctions(const std::vector<FunctionInstance>& set, const FunctionInstance& func){
            int counter=0;
            for(const auto & i : set){
                if(isFunctionOverloaded(i, func)){
                    counter++;
                }
            }
            return counter;
        }

    public:
        Analyser(const std::vector<FunctionInstance>& oldProgram,
                 const std::vector<FunctionInstance>& newProgram,
                 OutputHandler* outputHandler) {
            this->oldProgram = oldProgram;
            this->newProgram = newProgram;
            this->outputHandler = outputHandler;
        }

        void compareVersionsWithDoc(bool docEnabled, bool includePrivate) {
            int i = 0;
            //for (auto const &func: oldProgram) {
            while (i < oldProgram.size()) {
                FunctionInstance func = oldProgram.at(i);

                if (func.scope == "private" && !includePrivate) {
                    i++;
                    continue;
                }

                // skip this function if the old instance was private, because it couldn't have been used by anyone
                if (func.name == "main") {
                    i++;
                    continue;
                }
                auto test = countFunctions(newProgram, func);
                if (countFunctions(newProgram, func) <= 0) {
                    outputHandler->initialiseFunctionInstance(func);
                    auto bodyStatus = findBody(func, docEnabled);
                    // only output a renaming, if the similar function is not private, because knowing about a private function is not useful to the user
                    if (bodyStatus.first.empty()) {
                        outputHandler->outputDeletedFunction(func, false);
                    } else {
                        // use the function found during the statistical functionanalysis
                        FunctionInstance newFunc = newProgram.at(findFunction(newProgram, bodyStatus.first));
                        if (newFunc.name != func.name) {
                            // further checks on the Header to ensure, that this is indeed a renamed function
                            if (!compareFunctionHeader(func, newFunc, true)) {
                                if (docEnabled) {
                                    outputHandler->outputRenamedFunction(newFunc, func.name,
                                                                         std::to_string(bodyStatus.second));
                                } else {
                                    // perfect match, so using 100 is accurate
                                    outputHandler->outputRenamedFunction(newFunc, func.name, "100");
                                }
                            } else {
                                outputHandler->outputDeletedFunction(func, false);
                            }
                        } else {
                            compareFunctionHeader(func, newFunc, false);
                        }
                    }
                    outputHandler->endOfCurrentFunction();
                    ++i;
                } else if (countFunctions(newProgram, func) == 1) {
                    outputHandler->initialiseFunctionInstance(func);
                    FunctionInstance newFunc = newProgram.at(findFunction(newProgram, func.qualifiedName));
                    compareFunctionHeader(func, newFunc, false);
                    outputHandler->endOfCurrentFunction();
                    ++i;
                } else {
                    compareOverloadedFunctionHeader(func);
                }
            }
        }
    private:
        std::pair<std::string, double> findBody(const FunctionInstance &oldFunc, bool docEnabled) {
            std::string currentHighest = "";
            double currentHighestValue = -1;
            for (int i = 0; i < newProgram.size(); i++) {
                FunctionInstance newFunc = newProgram.at(i);
                if (newFunc.isDeclaration) {
                    continue;
                }
                if (docEnabled) {
                    double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc);

                    // prioritize functions that have the exact same name
                    if (oldFunc.name == newFunc.name && percentageDifference >= percentageCutOff) {
                        return std::make_pair(newFunc.qualifiedName, percentageDifference);
                    }

                    if (percentageDifference >= percentageCutOff) {
                        if (percentageDifference > currentHighestValue) {
                            currentHighestValue = percentageDifference;
                            currentHighest = newFunc.qualifiedName;
                        }
                    }
                } else {
                    // strip code of comments / empty spaces and then compares them
                    if (helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(newFunc.body)) ==
                        helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(oldFunc.body))) {
                        currentHighestValue = 100;
                        currentHighest = newFunc.qualifiedName;
                        break;
                    }
                }
            }

            return std::make_pair(currentHighest, currentHighestValue);
        }

        std::pair<FunctionInstance, double> findBody(const FunctionInstance &oldFunc, const std::vector<FunctionInstance> &funcSubset) {
            FunctionInstance currentHighest;
            double currentHighestValue = -1;
            for (auto const &newFunc: funcSubset) {
                if (newFunc.isDeclaration) {
                    continue;
                }
                double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc);

                if (percentageDifference >= percentageCutOff) {
                    if (percentageDifference > currentHighestValue) {
                        currentHighestValue = percentageDifference;
                        currentHighest = newFunc;
                    }
                }
            }

            return std::make_pair(currentHighest, currentHighestValue);
        }


        bool compareFunctionHeader(const FunctionInstance &func, const FunctionInstance &newFunc,
                                             bool internalUse) {
            int output = 0;
            // compare the params without overloading
            output += compareParams(func, newFunc, internalUse);

            output += compareFunctionHeaderExceptParams(func, newFunc, internalUse);

            return output;
        }

        bool compareOverloadedFunctionHeader(const FunctionInstance &func) {
            std::vector<FunctionInstance> overloadedFunctions;
            std::vector<FunctionInstance> oldOverloadedInstances;

            bool matchFound = false;
            int i = 0;
            while (i < oldProgram.size()) {
                matchFound = false;
                // check if there is an exact match
                int j = 0;
                while (j < newProgram.size()) {
                    if (newProgram.at(j).isDeclaration) {
                        j++;
                        continue;
                    }

                    // function header is an exact match
                    if (oldProgram.at(i).qualifiedName == func.qualifiedName &&
                        isFunctionOverloaded(oldProgram.at(i), newProgram.at(j))
                        && oldProgram.at(i).returnType == newProgram.at(j).returnType &&
                        !compareParams(oldProgram.at(i), newProgram.at(j), true)) {
                        outputHandler->initialiseFunctionInstance(func);
                        // proceed normally
                        bool analysisResult = compareFunctionHeaderExceptParams(oldProgram.at(i), newProgram.at(j),
                                                                                false);
                        oldProgram.erase(oldProgram.begin() + i);
                        newProgram.erase(newProgram.begin() + j);
                        matchFound = true;
                        outputHandler->endOfCurrentFunction();
                        break;
                    } else {
                        j++;
                    }
                }
                if (!matchFound) {
                    i++;
                }
            }

            for (auto &item: newProgram) {
                if (isFunctionOverloaded(func, item)) {
                    overloadedFunctions.push_back(item);
                }
            }

            i = 0;
            while (i < oldProgram.size()) {
                if (isFunctionOverloaded(oldProgram.at(i), func)) {
                    oldOverloadedInstances.push_back(oldProgram.at(i));
                    oldProgram.erase(oldProgram.begin() + i);
                } else {
                    i++;
                }
            }
            for (const auto &item: oldOverloadedInstances) {
                outputHandler->initialiseFunctionInstance(item);
                if (overloadedFunctions.empty()) {
                    outputHandler->outputDeletedFunction(func, true);
                    outputHandler->endOfCurrentFunction();
                    continue;
                }

                // if there isn't an exact match, find the nearest match
                auto closest = findBody(item, overloadedFunctions);
                if (closest.second != -1) {
                    // there is another function that fits, proceed to compare it normally
                    outputHandler->outputOverloadedDisclaimer(item, std::to_string(closest.second));
                    compareParams(item, closest.first, false);
                    compareFunctionHeaderExceptParams(item, closest.first, false);
                    // TODO: block for multiple old functions to be mapped to a single new function? (i.e. delete the here found function as well)
                } else {
                    outputHandler->outputDeletedFunction(item, true);
                }
                outputHandler->endOfCurrentFunction();
            }

            return true;
        }

        bool compareFunctionHeaderExceptParams(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            int output = 0;

            // TODO: Evaluate which of these should be included in the Header Checks
            output += compareReturnType(func, newFunc, internalUse);

            output += compareScope(func, newFunc, internalUse);

            output += compareNamespaces(func, newFunc, internalUse);

            output += compareFile(func, newFunc, internalUse);

            // Declarations are not part of the function header and should therefore not be included in the check of function header similarity
            compareDeclarations(func, newFunc, internalUse);

            output += compareStorageClass(func, newFunc, internalUse);

            output += compareFunctionSpecifier(func, newFunc, internalUse);

            compareConst(func, newFunc, internalUse);

            return output;
        }

        bool compareParams(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            bool output = false;
            vector<matcher::Operation> operations = matcher::getOptimalParamConversion(func.params, newFunc.params);

            if (!operations.empty()) {
                if (internalUse) {
                    return true;
                }
                output = true;
                for (const auto &item: operations) {
                    if (item.type == matcher::Operation::Types::REPLACEMENT) {
                        if (item.oldParam.first != item.newParam.first) {
                            outputHandler->outputParamChange(item.positionInOldParam, func, item.newParam, newFunc);
                        } else {
                            // if the types are the same, the change is in the defaults
                            if (item.newParam.second.second.empty()) {
                                outputHandler->outputNewParam(item.positionInOldParam, func, item.newParam, newFunc);
                            } else {
                                // both have a default, that itself changed OR there is a new default
                                outputHandler->outputParamDefaultChange(item.positionInOldParam, func, item.newParam,
                                                                        newFunc);
                            }
                        }
                    } else if (item.type == matcher::Operation::Types::INSERTION) {
                        outputHandler->outputNewParam(item.positionInOldParam, func, item.newParam, newFunc);
                    } else if (item.type == matcher::Operation::Types::DELETION) {
                        outputHandler->outputDeletedParam(item.positionInOldParam, func.params, newFunc);
                    } else {
                        throw std::invalid_argument(
                                "An invalid argument was passed by the matcher::getOptimalParamConversion function");
                    }
                }
            }
            return output;
        }

        bool compareReturnType(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            if (newFunc.returnType != func.returnType) {
                if (!internalUse) outputHandler->outputNewReturn(newFunc, func.returnType);
                return true;
            }
            return false;
        }

        bool compareScope(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            if (func.scope != newFunc.scope) {
                if (!internalUse) outputHandler->outputNewScope(newFunc, func);
                return true;
            }
            return false;
        }

        bool compareFile(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            if (func.isDeclaration && newFunc.isDeclaration) {
                return false;
            } else if (func.isDeclaration) {
                // no action taken, because a function was first defined and so the definition cant change files
                // TODO: add an output for "first defined" ?
                return false;
            } else if (newFunc.isDeclaration) {
                // the definition was deleted and only the declarations remain
                if (!internalUse) outputHandler->outputDeletedFunction(func, false);
                return true;
            } else {
                if (func.filename != newFunc.filename) {
                    if (!internalUse) outputHandler->outputNewFilename(newFunc, func);
                    return true;
                }
            }
            return false;
        }

        bool compareNamespaces(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            bool output = false;
            if (func.location.size() != newFunc.location.size()) {
                if (!internalUse) outputHandler->outputNewNamespaces(newFunc, func);
                return true;
            }
            for (int i = 0; i < func.location.size(); i++) {
                if (func.location.at(i) != newFunc.location.at(i)) {
                    if (!internalUse) outputHandler->outputNewNamespaces(newFunc, func);
                    output = true;
                }
            }
            return output;
        }

        std::vector<std::string> compareFilenameVectors(const std::vector<FunctionInstance> &decl1, const std::vector<FunctionInstance> &decl2) {
            std::vector<std::string> output;
            for (const auto &oldDecl: decl1) {
                if (!oldDecl.isDeclaration) continue;

                bool found = false;
                for (const auto &newDecl: decl2) {
                    if (!newDecl.isDeclaration) continue;

                    if (oldDecl.filename == newDecl.filename) {
                        found = true;
                    }
                }
                if (!found) {
                    output.push_back(oldDecl.filename);
                }
            }
            return output;
        }

        bool compareDeclarations(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            auto deletedDecl = compareFilenameVectors(func.declarations, newFunc.declarations);
            auto newDecl = compareFilenameVectors(newFunc.declarations, func.declarations);

            bool output = false;

            if (!deletedDecl.empty()) {
                if (!internalUse) outputHandler->outputDeletedDeclPositions(newFunc, deletedDecl, func);
                output = true;
            }

            if (!newDecl.empty()) {
                if (!internalUse) outputHandler->outputNewDeclPositions(newFunc, newDecl);
                output = true;
            }
            return output;
        }

        bool compareStorageClass(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            if (func.storageClass != newFunc.storageClass) {
                if (!internalUse) outputHandler->outputStorageClassChange(newFunc, func);
                return true;
            }
            return false;
        }

        bool compareFunctionSpecifier(const FunctionInstance &func, const FunctionInstance &newFunc,
                                                bool internalUse) {
            // TODO: changes here often come with one of the two not having definitions -> should there be a disclaimer for declaration only functions?
            if (func.memberFunctionSpecifier != newFunc.memberFunctionSpecifier) {
                if (!internalUse) outputHandler->outputFunctionSpecifierChange(newFunc, func);
                return true;
            }
            return false;
        }

        bool compareConst(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse){
            if(func.isConst != newFunc.isConst){
                if (!internalUse) outputHandler->outputFunctionConstChange(newFunc, func);
            }
            return false;
        }
    };
}
