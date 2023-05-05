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

    Analyser::Analyser(const std::multimap<std::string, FunctionInstance>& oldProgram,
                       const std::multimap<std::string, FunctionInstance>& newProgram,
                       bool JSONOutput) {
        this->oldProgram = oldProgram;
        this->newProgram = newProgram;
        if(!JSONOutput){
            outputHandler = new ConsoleOutputHandler();
        }else{
            outputHandler = new JSONOutputHandler();
        }
    }

    void Analyser::compareVersionsWithDoc(bool docEnabled, bool includePrivate) {
        for (auto const &x: oldProgram) {
            FunctionInstance func = x.second;

            if(func.scope == "private" && !includePrivate){
                continue;
            }

            // skip this function if the old instance was private, because it couldn't have been used by anyone
            if(func.name == "main" || func.isDeclaration){
                continue;
            }

            outputHandler->initialiseFunctionInstance(func);


            if (newProgram.count(func.qualifiedName) <= 0) {
                auto bodyStatus = findBody(func, docEnabled);
                // only output a renaming, if the similar function is not private, because knowing about a private function is not useful to the user
                if (bodyStatus.first.empty()){
                    outputHandler->outputDeletedFunction(func, false);
                } else {
                    // use the function found during the statistical analysis
                    FunctionInstance newFunc = newProgram.find(bodyStatus.first)->second;
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
            } else if (newProgram.count(func.qualifiedName) == 1){
                FunctionInstance newFunc = newProgram.find(func.qualifiedName)->second;
                compareFunctionHeader(func, newFunc);
            } else {
                compareOverloadedFunctionHeader(func);
            }
            outputHandler->endOfCurrentFunction();
        }
        outputHandler->printOut();
    }



    std::pair<std::string, double> Analyser::findBody(const FunctionInstance& oldFunc, bool docEnabled) {
        std::string currentHighest = "";
        double currentHighestValue = 0;
        for(auto const &newFunc : newProgram){
            if(newFunc.second.isDeclaration){
                continue;
            }
            if(docEnabled) {
                double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc.second);

                // prioritize functions that have the exact same name and header
                // TODO: ask to make sure this is the correct way to handle this
                if (oldFunc.name == newFunc.second.name) {
                    return std::make_pair(newFunc.second.qualifiedName, percentageDifference);
                }

                if (percentageDifference >= percentageCutOff) {
                    if (percentageDifference > currentHighestValue) {
                        currentHighestValue = percentageDifference;
                        currentHighest = newFunc.second.qualifiedName;
                    }
                }
            }else{
                // strip code of comments / empty spaces and then compares them
                if(helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(newFunc.second.body)) == helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(oldFunc.body))){
                    currentHighestValue = 100;
                    currentHighest = newFunc.second.qualifiedName;
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
        // check if there is an exact match
        for (const auto &item: newProgram){
            if(item.second.isDeclaration){
                continue;
            }
            if(item.second.qualifiedName == func.qualifiedName){
                // function header is an exact match
                if(!compareParams(func, item.second, true)){
                    // proceed normally
                    return compareFunctionHeaderExceptParams(func, item.second);
                }
                overloadedFunctions.push_back(item.second);
            }
        }
        // if there isn't an exact match, find the nearest match
        auto closest = findBody(func, overloadedFunctions);
        if(closest.second != 0){
            int output = 0;
            // there is another function that fits, proceed to compare it normally
            outputHandler->outputOverloadedDisclaimer(func, std::to_string(closest.second));
            output += compareParams(func, closest.first, false);
            output += compareFunctionHeaderExceptParams(func, closest.first);
            return output;
        }else{
            outputHandler->outputDeletedFunction(func, true);
            return true;
        }
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
        unsigned long numberOldParams = func.params.size();
        unsigned long numberNewParams = newFunc.params.size();
        if (numberOldParams == numberNewParams) {
            for (int i=0; i<numberNewParams; i++) {
                if ((func.params.at(i).first) != (newFunc.params.at(i).first)) {
                    if(!internalUse) outputHandler->outputParamChange(i, func.params.at(i), newFunc);
                    output = true;
                }
            }
        }
        // a parameter was added
        else if(numberNewParams > numberOldParams) {
            // all arguments added breaks the normal algorithm, so it has to be handled separately
            if(numberOldParams == 0){
                for(int i=0;i<numberNewParams;i++){
                    if(!internalUse) outputHandler->outputNewParam(i, newFunc, 1);
                }
            }else{
                int offset = 0;
                for(int i=0;i<numberNewParams;i++){
                    // prevents an out of range error
                    if(i-offset >= numberOldParams){
                        offset++;
                    }
                    if ((func.params.at(i-offset).first) != (newFunc.params.at(i).first)) {
                        offset++;
                        if(!internalUse) outputHandler->outputNewParam(i, newFunc, 1);
                    }
                }
            }
            output = true;
        }
        else {
            // all arguments deleted breaks the normal algorithm, so it has to be handled separately
            if(numberNewParams == 0){
                for(int i=0;i<numberNewParams;i++){
                    if(!internalUse) outputHandler->outputDeletedParam(i, func.params, newFunc, 1);
                }
            } else {
                int offset = 0;
                // a parameter was deleted
                for(int i = 0; i < numberOldParams; i++) {
                    // prevents an out of range error
                    if(i-offset >= numberNewParams){
                        offset++;
                    }

                    if ((func.params.at(i).first) != (newFunc.params.at(i - offset).first)) {
                        offset++;
                        if (!internalUse) outputHandler->outputDeletedParam(i, func.params, newFunc, 1);
                    }
                }
            }
            output = true;
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
            outputHandler->outputNewScope(newFunc, func.scope);
            return true;
        }
        return false;
    }

    bool Analyser::compareFile(const FunctionInstance& func, const FunctionInstance& newFunc){
        if(func.filename != newFunc.filename) {
            outputHandler->outputNewFilename(newFunc, func.filename);
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
