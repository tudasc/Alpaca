#include "header/FunctionAnalyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/CodeMatcher.h"
#include "header/HelperFunctions.h"
#include "header/OutputHandler.h"
#include <vector>
#include "omp.h"

using namespace llvm;
using namespace functionanalysis;
using namespace std;

namespace functionanalysis{

    class FunctionAnalyser {
        std::vector<FunctionInstance> oldProgram;
        std::vector<FunctionInstance> newProgram;
        OutputHandler* outputHandler;
        std::vector<std::vector<FunctionInstance>> newOverloadedFunctions;
        int counter = 0;

        const double percentageCutOff = 90;

        static bool isFunctionOverloaded(const FunctionInstance& oldFunc, const FunctionInstance& newFunc){
            // overloaded Functions have to have the same qualified name (i.e. be in the same namespace) and share at least one declaration position (definitions don´t have to be in the same file) : if one of the declarations is empty, the definition is the declaration
            return oldFunc.qualifiedName == newFunc.qualifiedName && checkIfADeclarationMatches(oldFunc, newFunc);
        }

        int findFunction(const std::vector<FunctionInstance>& set, const FunctionInstance& oldFunc){
            int tempSafe = -1;
            for(int i=0;i<set.size();i++){
                if(set.at(i).qualifiedName == oldFunc.qualifiedName && set.at(i).filename == oldFunc.filename){
                    // prefer definitions to declarations if one is found
                    if(oldFunc.isDeclaration && set.at(i).isDeclaration){
                        //return i;
                    }
                    //if(!set.at(i).isDeclaration){
                        return i;
                    //}else{
                        tempSafe = i;
                    //}
                }else{
                    tempSafe = checkIfADeclarationMatches(set.at(i), oldFunc) && oldFunc.name == set.at(i).name ? i : tempSafe;
                }
            }

            for(auto& overloadedSet : newOverloadedFunctions){
                if(overloadedSet.at(0).name != oldFunc.name) continue;
                for(auto& func : overloadedSet){
                    if(func.qualifiedName == oldFunc.qualifiedName && func.filename == oldFunc.filename){
                        // signals that the searched function is now overloaded
                        // TODO: add disclaimer
                        return -2;
                    }
                }
            }

            return tempSafe;
        }

        pair<int,int> countFunctions(const std::vector<FunctionInstance>& set, const FunctionInstance& func){
            int counter=0;
            int found=-1;
            for(int i=0;i<set.size();i++){
                if(isFunctionOverloaded(set.at(i), func)){
                    counter++;
                    found = i;
                }
            }
            if(counter == 1){
                return make_pair(counter,found);
            }
            return make_pair(counter,-1);
        }

    public:

        static bool checkIfADeclarationMatches(const FunctionInstance& oldDecl, const FunctionInstance& newDecl){
            if(oldDecl.filename == newDecl.filename){
                return true;
            }
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

        static std::vector<std::vector<FunctionInstance>> separateOverloadedFunctions(std::vector<FunctionInstance>& set){
            std::vector<std::vector<FunctionInstance>> result;
            int i=0;
            while (i < set.size()){
                std::vector<FunctionInstance> overloadedFunctionInstances;
                int j=0;
                while (j < set.size()){
                    if(i == j) {
                        j++;
                        continue;
                    }

                    if(isFunctionOverloaded(set.at(i), set.at(j))){
                        overloadedFunctionInstances.push_back(set.at(j));
                        set.erase(set.begin()+j);
                    }else{
                        j++;
                    }
                }
                if(!overloadedFunctionInstances.empty()){
                    overloadedFunctionInstances.push_back(set.at(i));
                    set.erase(set.begin()+i);
                    result.push_back(overloadedFunctionInstances);
                }else {
                    i++;
                }
            }
            outs() << "Found " << result.size() << " overloaded functions\n";
            outs() << set.size() << " non overloaded funcs left\n";
            return result;
        }

        FunctionAnalyser(const std::vector<FunctionInstance>& oldProgram,
                         const std::vector<FunctionInstance>& newProgram,
                         OutputHandler* outputHandler) {
            this->oldProgram = oldProgram;
            this->newProgram = newProgram;
            this->outputHandler = outputHandler;
        }

        void compareVersionsWithDoc(bool docEnabled, bool includePrivate) {

            auto overloadedFunctions = separateOverloadedFunctions(oldProgram);
            newOverloadedFunctions = separateOverloadedFunctions(newProgram);

            outs()<<"there are " << overloadedFunctions.size() << " overloaded functions\n";
            outs()<<"there are " << newOverloadedFunctions.size() << " new overloaded functions\n";

            //omp_set_num_threads(10);
            //for (auto const &func: oldProgram) {
            //#pragma omp parallel for default(none) shared(docEnabled, includePrivate, counter, outputHandler) private(oldProgram, newProgram)
            for(int i=0;i<oldProgram.size();i++) {

                counter++;
                if(counter % 2000 == 0){
                    outs() << "Analysed " << counter << " functions\n";
                }
                FunctionInstance func = oldProgram.at(i);

                if (func.scope == "private" && !includePrivate) {
                    //i++;
                    continue;
                }

                auto index = findFunction(newProgram, func);
                if (index == -1) {
                    outputHandler->initialiseFunctionInstance(func);
                    auto bodyStatus = findBody(func, docEnabled);
                    // only output a renaming, if the similar function is not private, because knowing about a private function is not useful to the user
                    if (bodyStatus.first == -1) {
                        outputHandler->outputDeletedFunction(func, false);
                        for (const auto &item: func.declarations){
                            if(!item.isDeclaration || func.isDeclaration) continue;
                            // the entire function is gone, this includes the decls
                            outputHandler->outputDeletedFunction(item, false);
                        }
                    } else {
                        // use the function found during the statistical functionanalysis
                        FunctionInstance newFunc = newProgram.at(bodyStatus.first);
                         if (newFunc.name != func.name) {
                            // further checks on the Header to ensure, that this is indeed a renamed function
                                if (docEnabled) {
                                    outputHandler->outputRenamedFunction(newFunc, func.name,
                                                                         std::to_string(bodyStatus.second));
                                } else {
                                    // perfect match, so using 100 is accurate
                                    outputHandler->outputRenamedFunction(newFunc, func.name, "100");
                                }
                        } else {
                            compareFunctionHeader(func, newFunc, false);
                        }
                        // remove the function from the newProgram, so that it is not compared again
                        newProgram.erase(newProgram.begin() + bodyStatus.first);
                    }
                    outputHandler->endOfCurrentFunction();
                    //++i;
                } else if(index == -2){
                    // function is newly overloaded and has to be added to the overloaded functions
                    overloadedFunctions.push_back({func});
                } else{
                    outputHandler->initialiseFunctionInstance(func);
                    FunctionInstance newFunc = newProgram.at(index);
                    compareFunctionHeader(func, newFunc, false);
                    outputHandler->endOfCurrentFunction();
                    //++i;
                }
            }

            // overloaded handling
            for(auto const& overloadedFunctionSet : overloadedFunctions){
                FunctionInstance func = overloadedFunctionSet.at(0);
                compareOverloadedFunctionHeader(func, overloadedFunctionSet);
            }
        }
    private:
        // TODO: maybe open up the criterias more to recognize renamed Files + Functions
        std::pair<int, double> findBody(const FunctionInstance &oldFunc, bool docEnabled) {
            int currentHighest = -1;
            double currentHighestValue = -1;
            for (int i = 0; i < newProgram.size(); i++) {
                FunctionInstance newFunc = newProgram.at(i);
                if (newFunc.isDeclaration) {
                    continue;
                }
                if (docEnabled) {
                    // prioritize functions that have the exact same name in the same file
                    if (oldFunc.name == newFunc.name) {
                        double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc);
                        if(percentageDifference >= percentageCutOff)
                            // check if found function has a match in the old program and if not, return its index (since findFunction checks for name AND file)
                            if(findFunction(oldProgram, newFunc) == -1) return std::make_pair(i, percentageDifference);
                    }
                    // TODO: make the check less strict, so it can change renames AND other things, would be better to increase strictness of the matcher (i.e. minimum of characters, etc.)
                    if (!compareFunctionHeader(oldFunc, newFunc, true)) {
                        double percentageDifference = matcher::compareFunctionBodies(oldFunc, newFunc);
                        if (percentageDifference >= percentageCutOff) {
                            if (percentageDifference > currentHighestValue) {
                                currentHighestValue = percentageDifference;
                                currentHighest = i;
                            }
                        }
                    }
                } else {
                    // strip code of comments / empty spaces and then compares them
                    if (helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(newFunc.body)) ==
                        helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(oldFunc.body))) {
                        currentHighestValue = 100;
                        currentHighest = i;
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

        bool compareOverloadedFunctionHeader(const FunctionInstance &func, const std::vector<FunctionInstance>& oldOverloadedInstances) {
            std::vector<FunctionInstance> overloadedFunctions;
            for(int x=0;x<oldOverloadedInstances.size();x++) {
                for (int i = 0; i < newOverloadedFunctions.size(); i++) {
                    if(oldOverloadedInstances.at(x).qualifiedName != newOverloadedFunctions.at(i).at(0).qualifiedName) {
                        continue;
                    }
                    for (int j = 0; j < newOverloadedFunctions.at(i).size(); j++) {
                        if (oldOverloadedInstances.at(x).qualifiedName == newOverloadedFunctions.at(i).at(j).qualifiedName &&
                                oldOverloadedInstances.at(x).filename == newOverloadedFunctions.at(i).at(j).filename) {
                            overloadedFunctions = newOverloadedFunctions.at(i);
                            break;
                        }
                    }
                }
            }


            for (const auto &item: oldOverloadedInstances) {
                outputHandler->initialiseFunctionInstance(item);

                if (overloadedFunctions.empty()) {
                    outputHandler->outputDeletedFunction(func, true);
                    for (const auto &decl: func.declarations){
                        if(!decl.isDeclaration || item.isDeclaration) continue;
                        // the entire function is gone, this includes the decls
                        outputHandler->outputDeletedFunction(decl, true);
                    }
                    outputHandler->endOfCurrentFunction();
                    continue;
                }

                bool found = false;
                for (const auto &newFunc: overloadedFunctions){
                    if(item.equals(newFunc)){
                        compareParams(item, newFunc, false);
                        compareFunctionHeaderExceptParams(item, newFunc, false);
                        found = true;
                    }
                }
                if(found) {
                   continue;
                }

                // if there isn't an exact match, find the nearest match
                auto closest = findBody(item, overloadedFunctions);
                if (closest.second != -1) {
                    // there is another function that fits, proceed to compare it normally
                    compareParams(item, closest.first, false);
                    compareFunctionHeaderExceptParams(item, closest.first, false);
                    // TODO: block for multiple old functions to be mapped to a single new function? (i.e. delete the here found function as well)
                } else {
                    outputHandler->outputDeletedFunction(item, true);
                    for (const auto &decls: item.declarations){
                        if(!item.isDeclaration || func.isDeclaration) continue;
                        // the entire function is gone, this includes the decls
                        outputHandler->outputDeletedFunction(decls, true);
                    }
                }
                outputHandler->endOfCurrentFunction();
            }

            return true;
        }

        bool compareFunctionTemplates(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse){
            bool output = false;
            // comparing the template parameters
            vector<matcher::Operation> operations = matcher::getOptimalParamConversion(func.templateParams, newFunc.templateParams);

            if(!operations.empty()){
                output = true;
            }
            if(!operations.empty() && !internalUse){
                for (const auto &item: operations) {
                    if (item.type == matcher::Operation::Types::REPLACEMENT) {
                        if (item.oldParam.first != item.newParam.first) {
                            outputHandler->outputTemplateParameterChanged(item.positionInOldParam, func, item.newParam, newFunc);
                        }
                        if (item.newParam.second.second != item.oldParam.second.second) {
                            outputHandler->outputTemplateParameterDefaultChanged(item.positionInOldParam, func, item.newParam,
                                                                    newFunc);
                        }
                    } else if (item.type == matcher::Operation::Types::INSERTION) {
                        outputHandler->outputTemplateParameterAdded(item.positionInOldParam, func, item.newParam, newFunc);
                    } else if (item.type == matcher::Operation::Types::DELETION) {
                        outputHandler->outputTemplateParameterDeleted(item.positionInOldParam, func, newFunc);
                    } else {
                        throw std::invalid_argument(
                                "An invalid argument was passed by the matcher::getOptimalParamConversion function");
                    }
                }
            }
            // comparing the template specifications on the basis of their params (i.e. if two specs have the same params, they are considered equal) and if a spec has no match it is considered deleted
            for (const auto &item: func.templateSpecializations){
                bool found = false;
                for (const auto &newItem: newFunc.templateSpecializations){
                    if(!compareParams(item, newItem, true)){
                        found = true;
                        break;
                    }
                }
                if(!found){
                    outputHandler->outputDeletedSpecialization(item);
                    output = true;
                }
            }

            for (const auto &item: newFunc.templateSpecializations){
                bool found = false;
                for (const auto &oldItem: func.templateSpecializations){
                    if(!compareParams(oldItem, item, true)){
                        found = true;
                        break;
                    }
                }
                if(!found){
                    outputHandler->outputNewSpecialization(item);
                    output = true;
                }
            }


            return output;
        }

        bool compareFunctionHeaderExceptParams(const FunctionInstance &func, const FunctionInstance &newFunc, bool internalUse) {
            int output = 0;

            if(func.isTemplateDecl && newFunc.isTemplateDecl){
                output += compareFunctionTemplates(func, newFunc, internalUse);
            }else if(func.isTemplateDecl && !newFunc.isTemplateDecl){
                if(!internalUse) {
                    outputHandler->outputTemplateIsNowFunction(func, newFunc);
                }
            }else if(!func.isTemplateDecl && newFunc.isTemplateDecl){
                if(!internalUse) {
                    outputHandler->outputFunctionIsNowTemplate(func, newFunc);
                }
            }

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
                        }

                        if (item.newParam.second.second != item.oldParam.second.second) {
                            outputHandler->outputParamDefaultChange(item.positionInOldParam, func, item.newParam,
                                                                    newFunc);
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
