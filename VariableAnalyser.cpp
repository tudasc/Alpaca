#include "header/VariableAnalyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/HelperFunctions.h"
#include "header/OutputHandler.h"

namespace variableanalysis {

    class VariableAnalyser{
        std::vector<VariableInstance> oldVariables;
        std::vector<VariableInstance> newVariables;
        OutputHandler* outputHandler;
    int findVariable(const std::vector<VariableInstance>& set, const std::string& qualifiedName){
        for(int i=0;i<set.size();i++){
            if(set.at(i).qualifiedName == qualifiedName){
                return i;
            }
        }
        return -1;
    }
    public:
        VariableAnalyser(const std::vector<VariableInstance>& oldVariables, const std::vector<VariableInstance>& newVariables, OutputHandler* outputHandler){
            this->oldVariables = oldVariables;
            this->newVariables = newVariables;
            this->outputHandler = outputHandler;
        }
        void compareVariables(){
            // TODO: moving variables okay, for now use qualifiedName for finding correct variables -> potential for a lot of logic (maybe using the modified levenshtein distance?)
            for (const auto &oldVar: oldVariables){
                outputHandler->initialiseVariableInstance(oldVar);
                auto indexInNew = findVariable(newVariables, oldVar.qualifiedName);
                if(indexInNew != -1){
                    VariableInstance newVar = newVariables.at(indexInNew);
                    compareMainHeader(oldVar, newVar);
                    compareQualifiers(oldVar, newVar);
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

    };

}