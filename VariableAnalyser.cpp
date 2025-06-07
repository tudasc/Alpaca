#include "header/VariableAnalyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/HelperFunctions.h"
#include "header/OutputHandler.h"
#include "header/CodeMatcher.h"

namespace variableanalysis {

    class VariableAnalyser{
        std::vector<VariableInstance> oldVariables;
        std::vector<VariableInstance> newVariables;
        OutputHandler* outputHandler;
    int findVariable(const std::vector<VariableInstance>& set, const VariableInstance& variableInstance){
        // prioritylist (name has to be the same): qualifiedName > filename > closest match on location
        std::map<int, VariableInstance> possibleMatches;
        for(int i=0;i<set.size();i++){
            if(set.at(i).qualifiedName == variableInstance.qualifiedName){
                return i;
            }
            if(set.at(i).name == variableInstance.name){
                possibleMatches.insert(std::make_pair(i, set.at(i)));
            }
        }
        for(const auto& possibleMatch: possibleMatches){
            // return the first match with the same filename
            if(possibleMatch.second.filename == variableInstance.filename){
                return possibleMatch.first;
            }
        }

        // TODO: introduce a threshold at which the location is considered to be too far away and then maybe omit the file check
        // if there isn't even a match with the same filename, return the variable with the closest matching location
        int smallestDiff = std::numeric_limits<int>::max();
        int candidate = -1;
        for (const auto &item: possibleMatches){
            auto matrix = matcher::levenshteinDistance(item.second.location, variableInstance.location);
            if(smallestDiff > matrix[item.second.location.size()][variableInstance.location.size()]){
                smallestDiff = matrix[item.second.location.size()][variableInstance.location.size()];
                candidate = item.first;
            }
        }

        return candidate;
    }
    public:
        VariableAnalyser(const std::vector<VariableInstance>& oldVariables, const std::vector<VariableInstance>& newVariables, OutputHandler* outputHandler){
            this->oldVariables = oldVariables;
            this->newVariables = newVariables;
            this->outputHandler = outputHandler;
        }
        void compareVariables(){
            for (const auto &oldVar: oldVariables){
                if(oldVar.accessSpecifier == "private" || oldVar.accessSpecifier == "protected"){
                    continue;
                }
                outputHandler->initialiseVariableInstance(oldVar);
                auto indexInNew = findVariable(newVariables, oldVar);
                if(indexInNew != -1){
                    VariableInstance newVar = newVariables.at(indexInNew);
                    compareMainHeader(oldVar, newVar);
                    compareLocation(oldVar, newVar);
                    compareQualifiers(oldVar, newVar);
                    compareDefinitions(oldVar, newVar);
                }else{
                    outputHandler->outputVariableDeleted(oldVar);
                }
                outputHandler->endOfCurrentVariable();
            }
        }

    private:
        void compareMainHeader(const VariableInstance& oldVar, const VariableInstance& newVar){
            if(oldVar.isClassMember != newVar.isClassMember){
                outputHandler->outputVariableClassMember(oldVar, newVar);
            }
            if(oldVar.type != newVar.type){
                outputHandler->outputVariableTypeChange(oldVar, newVar);
            }
            if(oldVar.defaultValue != newVar.defaultValue){
                outputHandler->outputVariableDefaultValueChange(oldVar, newVar);
            }
            if(oldVar.filename != newVar.filename){
                outputHandler->outputVariableFileChange(oldVar, newVar);
            }
            if(oldVar.storageClass != newVar.storageClass){
                outputHandler->outputVariableStorageClassChange(oldVar, newVar);
            }
        }

        void compareLocation(const VariableInstance& oldVar, const VariableInstance& newVar){
            if(oldVar.location.size() != newVar.location.size()){
                outputHandler->outputVariableLocationChange(oldVar, newVar);
            }else{
                 for(int i=0;i<oldVar.location.size();i++){
                    if(oldVar.location.at(i) != newVar.location.at(i)){
                        outputHandler->outputVariableLocationChange(oldVar, newVar);
                        break;
                    }
                }
            }
        }

        void compareQualifiers(const VariableInstance& oldVar, const VariableInstance& newVar){
        // processed in detail, in case there is more complicated logic introduced in the future
            if(oldVar.isInline != newVar.isInline){
                outputHandler->outputVariableInlineChange(oldVar, newVar);
            }
            if(oldVar.accessSpecifier != newVar.accessSpecifier){
                outputHandler->outputVariableAccessSpecifierChange(oldVar, newVar);
            }
            if(oldVar.isConst != newVar.isConst){
                outputHandler->outputVariableConstChange(oldVar, newVar);
            }
            if(oldVar.isExplicit != newVar.isExplicit){
                outputHandler->outputVariableExplicitChange(oldVar, newVar);
            }
            if(oldVar.isVolatile != newVar.isVolatile){
                outputHandler->outputVariableVolatileChange(oldVar, newVar);
            }
            if(oldVar.isMutable != newVar.isMutable){
                outputHandler->outputVariableMutableChange(oldVar, newVar);
            }
        }

        void compareDefinitions(const VariableInstance& oldVar, const VariableInstance& newVar){
            for (const auto &oldDef: oldVar.definitions){
                bool found = false;
                for (const auto &newDef: newVar.definitions){
                    if(oldDef.filename == newDef.filename){
                        found = true;
                        if(oldDef.defaultValue != newDef.defaultValue){
                            outputHandler->outputVariableDefaultValueChange(oldDef, newDef);
                        }
                    }
                }
                if(!found){
                    outputHandler->outputVariableDefinitionDeleted(oldDef);
                }
            }
            for (const auto &newDef: newVar.definitions){
                bool found = false;
                for (const auto &oldDef: oldVar.definitions){
                    if(oldDef.filename == newDef.filename){
                        found = true;
                    }
                }
                if(!found){
                    outputHandler->outputVariableDefinitionAdded(newDef);
                }
            }
        }

    };

}