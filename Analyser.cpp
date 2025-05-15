#include "header/Analyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/CodeMatcher.h"
#include "header/HelperFunctions.h"
#include "ConsoleOutputHandler.cpp"
#include "JSONOutputHandler.cpp"


using namespace llvm;
using namespace analyse;

const double percentageCutOff = 90;

namespace analyse{

    OutputHandler* outputHandler;

    Analyser::Analyser(const std::vector<FunctionInstance>& oldProgram,
                       const std::vector<FunctionInstance>& newProgram,
                       bool JSONOutput) {
        this->oldProgram = oldProgram;
        this->newProgram = newProgram;
        if(!JSONOutput){
            outputHandler = new ConsoleOutputHandler();
        }else{
            outputHandler = new JSONOutputHandler();
        }
    }

    int findFunction(const std::vector<FunctionInstance>& set, std::string qualifiedName){
        for(int i=0;i<set.size();i++){
            if(set.at(i).qualifiedName == qualifiedName){
                return i;
            }
        }
        return -1;
    }

    int countFunctions(const std::vector<FunctionInstance>& set, std::string qualifiedName){
        int counter=0;
        for(const auto & i : set){
            if(i.qualifiedName == qualifiedName){
                counter++;
            }
        }
        return counter;
    }


    void Analyser::compareVersionsWithDoc(bool docEnabled, bool includePrivate) {
        int i=0;
        //for (auto const &func: oldProgram) {
        while(i<oldProgram.size()){
            FunctionInstance func = oldProgram.at(i);

            if(func.scope == "private" && !includePrivate){
                i++;
                continue;
            }

            // skip this function if the old instance was private, because it couldn't have been used by anyone
            if(func.name == "main" || func.isDeclaration){
                i++;
                continue;
            }

            if (countFunctions(newProgram, func.qualifiedName) <= 0) {
                outputHandler->initialiseFunctionInstance(func);
                auto bodyStatus = findBody(func, docEnabled);
                // only output a renaming, if the similar function is not private, because knowing about a private function is not useful to the user
                if (bodyStatus.first.empty()){
                    outputHandler->outputDeletedFunction(func, false);
                } else {
                    outs()<<findFunction(newProgram, bodyStatus.first)<<"\n";
                    // use the function found during the statistical analysis
                    FunctionInstance newFunc = newProgram.at(findFunction(newProgram, bodyStatus.first));
                    if(newFunc.name != func.name){
                        // further checks on the Header to ensure, that this is indeed a renamed function
                        if(!compareFunctionHeader(func, newFunc)){
                            if(docEnabled) {
                                outputHandler->outputRenamedFunction(newFunc, func.name, std::to_string(bodyStatus.second));
                            }else{
                                // perfect match, so using 100 is accurate
                                outputHandler->outputRenamedFunction(newFunc, func.name, "100");
                            }
                        } else {
                            outputHandler->outputDeletedFunction(func, false);
                        }
                    } else{
                        compareFunctionHeader(func, newFunc);
                    }
                }
                outputHandler->endOfCurrentFunction();
            } else if (countFunctions(newProgram, func.qualifiedName) == 1){
                outputHandler->initialiseFunctionInstance(func);
                FunctionInstance newFunc = newProgram.at(findFunction(newProgram, func.qualifiedName));
                compareFunctionHeader(func, newFunc);
                outputHandler->endOfCurrentFunction();
            } else {
                compareOverloadedFunctionHeader(func);
            }
            ++i;
        }
        outputHandler->printOut();
    }

    std::pair<std::string, double> Analyser::findBody(const FunctionInstance& oldFunc, bool docEnabled) {
        std::string currentHighest = "";
        double currentHighestValue = 0;
        for(int i=0;i<newProgram.size();i++){
            FunctionInstance newFunc = newProgram.at(i);
            if(newFunc.isDeclaration){
                continue;
            }
            if(docEnabled) {
                double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc);

                // prioritize functions that have the exact same name and header
                // TODO: ask to make sure this is the correct way to handle this
                if (oldFunc.name == newFunc.name) {
                    return std::make_pair(newFunc.qualifiedName, percentageDifference);
                }

                if (percentageDifference >= percentageCutOff) {
                    if (percentageDifference > currentHighestValue) {
                        currentHighestValue = percentageDifference;
                        currentHighest = newFunc.qualifiedName;
                    }
                }
            }else{
                // strip code of comments / empty spaces and then compares them
                if(helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(newFunc.body)) == helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(oldFunc.body))){
                    currentHighestValue = 100;
                    currentHighest = newFunc.qualifiedName;
                    break;
                }
            }
        }

        return std::make_pair(currentHighest, currentHighestValue);
    }

    std::pair<FunctionInstance, double> Analyser::findBody(const FunctionInstance& oldFunc, const std::vector<FunctionInstance>& funcSubset){
        FunctionInstance currentHighest;
        double currentHighestValue = 0;
        for(auto const &newFunc : funcSubset){
            if(newFunc.isDeclaration){
                continue;
            }
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


    bool Analyser::compareFunctionHeader(const FunctionInstance& func, const FunctionInstance& newFunc) {
        int output = 0;
        // compare the params without overloading
        output += compareParams(func, newFunc, false);

        output += compareFunctionHeaderExceptParams(func, newFunc);

        return output;
    }

    bool Analyser::compareOverloadedFunctionHeader(const FunctionInstance& func) {
        std::vector<FunctionInstance> overloadedFunctions;
        std::vector<FunctionInstance> oldOverloadedInstances;

        bool matchFound=false;
        // TODO convert to while
        int i = 0;
        while(i<oldProgram.size()) {
            matchFound = false;
            // check if there is an exact match
            int j = 0;
            while(j<newProgram.size()) {
                if (newProgram.at(j).isDeclaration) {
                    j++;
                    continue;
                }

                // function header is an exact match
                // TODO: how to handle the scenario that multiple files and namespaces have functions with the same name?
                if(oldProgram.at(i).name == func.name && oldProgram.at(i).name == newProgram.at(j).name && !compareParams(oldProgram.at(i), newProgram.at(j), true)) {
                    outputHandler->initialiseFunctionInstance(func);
                    // proceed normally
                    bool analysisResult = compareFunctionHeaderExceptParams(oldProgram.at(i), newProgram.at(j));
                    oldProgram.erase(oldProgram.begin() + i);
                    newProgram.erase(newProgram.begin() + j);
                    matchFound = true;
                    outputHandler->endOfCurrentFunction();
                    break;
                }else{
                    j++;
                }
            }
            if(!matchFound){
                i++;
            }
        }

        for (auto & item : newProgram){
            if(func.name == item.name){
                overloadedFunctions.push_back(item);
            }
        }

        for (auto & item : oldProgram){
            if(func.name == item.name){
                oldOverloadedInstances.push_back(item);
            }
        }
        for (const auto &item: oldOverloadedInstances){
            outputHandler->initialiseFunctionInstance(item);
            if(overloadedFunctions.empty()){
                outputHandler->outputDeletedFunction(func, true);
                outputHandler->endOfCurrentFunction();
                continue;
            }

            // if there isn't an exact match, find the nearest match
            auto closest = findBody(item, overloadedFunctions);
            if (closest.second != 0) {
                // there is another function that fits, proceed to compare it normally
                outputHandler->outputOverloadedDisclaimer(item, std::to_string(closest.second));
                compareParams(item, closest.first, false);
                compareFunctionHeaderExceptParams(item, closest.first);
                // TODO: block for multiple old functions to be mapped to a single new function? (i.e. delete the here found function as well)
            } else {
                outputHandler->outputDeletedFunction(func, true);
            }
            outputHandler->endOfCurrentFunction();
        }

        return true;
    }

    bool Analyser::compareFunctionHeaderExceptParams(const FunctionInstance& func, const FunctionInstance& newFunc){
        int output = 0;

        output += compareReturnType(func, newFunc);

        output += compareScope(func, newFunc);

        output += compareNamespaces(func, newFunc);

        output += compareFile(func, newFunc);

        // Declarations are not part of the function header and should therefore not be included in the check of function header similarity
        compareDeclarations(func, newFunc);

        return output;
    }

    bool Analyser::compareParams(const FunctionInstance& func, const FunctionInstance& newFunc, bool internalUse) {
        bool output = false;
        vector<matcher::Operation> operations = matcher::getOptimalParamConversion(func.params, newFunc.params);

        if(operations.size() != 0){
            if(internalUse){
                return true;
            }
            output = true;
            for (const auto &item: operations){
                if(item.type == matcher::Operation::Types::REPLACEMENT){
                    if(item.oldParam.first != item.newParam.first){
                        outputHandler->outputParamChange(item.positionInOldParam, func, item.newParam);
                    }else{
                        // if the types are the same, the change is in the defaults
                        if(item.newParam.second.second.empty()){
                            // the default was removed -> TODO: treat as an addition?
                            outputHandler->outputNewParam(item.positionInOldParam, func, item.newParam);
                        }else{
                            // both have a default, that itself changed OR there is a new default
                            outputHandler->outputParamDefaultChange(item.positionInOldParam, func, item.newParam);
                        }
                    }
                } else if(item.type == matcher::Operation::Types::INSERTION){
                    outputHandler->outputNewParam(item.positionInOldParam, func, item.newParam);
                } else if(item.type == matcher::Operation::Types::DELETION){
                    outputHandler->outputDeletedParam(item.positionInOldParam, func.params);
                }else{
                    throw new std::exception;
                }
            }
        }
        return output;
    }

    bool Analyser::compareReturnType(const FunctionInstance& func, const FunctionInstance& newFunc) {
        if(newFunc.returnType != func.returnType) {
            outputHandler->outputNewReturn(newFunc, func.returnType);
            return true;
        }
        return false;
    }

    bool Analyser::compareScope(const FunctionInstance& func, const FunctionInstance& newFunc){
        if(func.scope != newFunc.scope){
            outputHandler->outputNewScope(newFunc, func);
            return true;
        }
        return false;
    }

    bool Analyser::compareFile(const FunctionInstance& func, const FunctionInstance& newFunc){
        if(func.filename != newFunc.filename) {
            outputHandler->outputNewFilename(newFunc, func);
            return true;
        }
        return false;
    }

    bool Analyser::compareNamespaces(const FunctionInstance& func, const FunctionInstance& newFunc){
        bool output = false;
        if(func.location.size() != newFunc.location.size()){
            outputHandler->outputNewNamespaces(newFunc, func);
            return true;
        }
        for(int i=0;i<func.location.size();i++){
            if(func.location.at(i) != newFunc.location.at(i)){
                outputHandler->outputNewNamespaces(newFunc, func);
                output = true;
            }
        }
        return output;
    }

    std::vector<std::string> compareFilenameVectors(const std::vector<FunctionInstance>& decl1, const std::vector<FunctionInstance>& decl2){
        std::vector<std::string> output;
        for(const auto& oldDecl : decl1){
            bool found = false;
            for(const auto& newDecl : decl2){
                if(oldDecl.filename == newDecl.filename){
                    found = true;
                }
            }
            if(!found){
                output.push_back(oldDecl.filename);
            }
        }
        return output;
    }

    bool Analyser::compareDeclarations(const FunctionInstance& func, const FunctionInstance& newFunc){
        auto deletedDecl = compareFilenameVectors(func.declarations, newFunc.declarations);
        auto newDecl = compareFilenameVectors(newFunc.declarations, func.declarations);

        bool output = false;

        if(deletedDecl.size() > 0){
            outputHandler->outputDeletedDeclPositions(newFunc, deletedDecl, func);
            output = true;
        }

        if(newDecl.size() > 0){
            outputHandler->outputNewDeclPositions(newFunc, newDecl);
            output = true;
        }
        return output;
    }
}
