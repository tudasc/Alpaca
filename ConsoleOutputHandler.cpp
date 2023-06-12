#include "header/OutputHandler.h"
#include <string>
#include <vector>
#include "header/FunctionAnalyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/HelperFunctions.h"

using namespace std;

class ConsoleOutputHandler : public OutputHandler {
public:
    std::string output;
    std::string completeOutput;
    std::string startingMessage;

    ConsoleOutputHandler()= default;

    void initialiseFunctionInstance(const functionanalysis::FunctionInstance &func) override {
        this->startingMessage = "-------------------------------------" + func.qualifiedName + " (Function) -------------------------------------\n";
        this->startingMessage += "Full old function header " + func.fullHeader + "\n";
        if(func.isTemplateDecl){
            this->startingMessage += "------------- This function is a template -------------\n";
        }
        output = startingMessage;
    }

    void initialiseVariableInstance(const variableanalysis::VariableInstance &var) override {
        this->startingMessage = "-------------------------------------" + var.qualifiedName + " (Variable) -------------------------------------\n";
        output = startingMessage;
    }

    void initialiseObjectInstance(const objectanalysis::ObjectInstance& obj) override {
        this->startingMessage = "-------------------------------------" + obj.qualifiedName + " (Class / Namespace / Struct / Enum) -------------------------------------\n";
        output = startingMessage;
    }

    void outputNewParam(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) override{
        if(oldPosition != 0) {
            output +=
                    "There is a new parameter (" + newParam.first + " " + newParam.second.first + ") after the param " +
                    oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first
                    + ". Full params: " + helper::getAllParamsAsString(oldFunc.params) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";
        }else {
            output += "There is a new parameter (" + newParam.first + " " + newParam.second.first + ")" + ". Full params: " + helper::getAllParamsAsString(oldFunc.params) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";
        }
    }

    void outputParamChange(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) override {
        output += "A parameter changed from " + oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first + " to "
        + newParam.first + " " + newParam.second.first+ " at position " + to_string(oldPosition) + ". Full params: " + helper::getAllParamsAsString(oldFunc.params) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";
    }

    void outputParamDefaultChange(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) override {

        string oldValue = (oldFunc.params.at(oldPosition).second.second.empty()) ? oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first : oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first + " = " + oldFunc.params.at(oldPosition).second.second;
        string newValue = (newParam.second.second.empty()) ? newParam.first + " " + newParam.second.first : newParam.first + " " + newParam.second.first + " = " + newParam.second.second;

        output += "A default value of the param " + oldValue + " changed to " + newValue + ". Full params: " + helper::getAllParamsAsString(oldFunc.params) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";
    }

    void outputDeletedParam(int oldPosition, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& oldParams, const functionanalysis::FunctionInstance& newFunc) override {
        output += "The parameter " + oldParams.at(oldPosition).first + " " + oldParams.at(oldPosition).second.first + " was deleted " + ". Full params: " + helper::getAllParamsAsString(oldParams) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";;
     }

     void outputNewReturn(const functionanalysis::FunctionInstance &newFunc, std::string oldReturn) override {
        output += "The return type changed from " + oldReturn + " to " + newFunc.returnType + "\n";
    }

    void outputNewScope(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) override {
        output += "The scope changed from " + oldFunc.scope + " to " + newFunc.scope + "\n";
    }

    void outputNewNamespaces(const functionanalysis::FunctionInstance &newFunc, const functionanalysis::FunctionInstance& oldFunc) override {
        output += "The function moved namespaces from " + helper::getAllNamespacesAsString(oldFunc.location) + " to "
                + helper::getAllNamespacesAsString(newFunc.location) + "\n";
    }

    void outputNewFilename(const functionanalysis::FunctionInstance &newFunc, const functionanalysis::FunctionInstance& oldFunc) override{
        output += "The function definition moved files from " + oldFunc.filename + " to " + newFunc.filename + "\n";
    }

    void outputNewDeclPositions(const functionanalysis::FunctionInstance &newFunc, std::vector<std::string> addedDecl) override {
        output += "The function has new declarations in the files " + helper::getAllNamespacesAsString(addedDecl) + "\n";
    }

    void outputDeletedDeclPositions(const functionanalysis::FunctionInstance &newFunc, std::vector<std::string> deletedDecl, const functionanalysis::FunctionInstance& oldFunc) override {
        output += "Declarations in the files " + helper::getAllNamespacesAsString(deletedDecl) + " were deleted\n";
    }

    void outputDeletedFunction(const functionanalysis::FunctionInstance &deletedFunc, bool overloaded) override {
        if(deletedFunc.isDeclaration){
            output += "A declaration in the file " + deletedFunc.filename + " was deleted";
            return;
        }
        if(overloaded){
            output += "The overloaded function with the params " + helper::getAllParamsAsString(deletedFunc.params) + " was deleted\n";
        } else {
            output += "A function definition was deleted from the file " + deletedFunc.filename + "\n";
        }
    }

    void outputOverloadedDisclaimer(const functionanalysis::FunctionInstance &func, std::string percentage) override {
        output += "DISCLAIMER: There is code similarity of " + percentage
                + "% between the old overloaded function and the in the following analyzed instance of the overloaded function\n";
    }

    void outputRenamedFunction(const functionanalysis::FunctionInstance &newFunc, std::string oldName, std::string percentage) override {
        output += "The function was renamed to \"" + newFunc.name +
                  "\" with a code similarity of " + percentage + "%\n";
    }

    void outputStorageClassChange(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) override{
        output += "The functions storage class changed from " + oldFunc.storageClass + " to " + newFunc.storageClass + "\n";
    }

    void outputFunctionSpecifierChange(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) override{
        output += "The function specifier changed from " + oldFunc.memberFunctionSpecifier + " to " + newFunc.memberFunctionSpecifier + "\n";
    }

    void outputFunctionConstChange(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) override{
        if(oldFunc.isConst){
            output += "The function is not declared const anymore\n";
        }else{
            output += "The function is now declared const\n";
        }
    }

    bool endOfCurrentFunction() override{
        if(output != startingMessage){
            output += "\n";
            completeOutput += output;
            return true;
        }
        return false;
    }

    // Templates
    void outputTemplateIsNowFunction(const functionanalysis::FunctionInstance& oldFunc, const functionanalysis::FunctionInstance& newFunc) override {
        output += "The template is now a function";
    }

    void outputFunctionIsNowTemplate(const functionanalysis::FunctionInstance& oldFunc, const functionanalysis::FunctionInstance& newFunc) override {
        output += "The function is now a template";
    }

    void outputTemplateParameterAdded(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) override {
        output += "The template parameter " + helper::getSingleTemplateParamAsString(newParam) + " was added after the position " + std::to_string(oldPosition) + ". Full params: " + helper::getAllTemplateParamsAsString(oldFunc.templateParams) + " -> " + helper::getAllTemplateParamsAsString(newFunc.templateParams) + "\n";
    }

    void outputTemplateParameterDeleted(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, const functionanalysis::FunctionInstance& newFunc) override {
        output += "The template parameter " + helper::getSingleTemplateParamAsString(oldFunc.params.at(oldPosition)) + " was deleted at position " + std::to_string(oldPosition) + ". Full params: " + helper::getAllTemplateParamsAsString(oldFunc.templateParams) + " -> " + helper::getAllTemplateParamsAsString(newFunc.templateParams) + "\n";
    }

    void outputTemplateParameterChanged(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) override {
        output += "The template parameter " + helper::getSingleTemplateParamAsString(oldFunc.templateParams.at(oldPosition)) + " was changed to " + helper::getSingleTemplateParamAsString(newParam) + " at position " + std::to_string(oldPosition) + ". Full params: " + helper::getAllTemplateParamsAsString(oldFunc.templateParams) + " -> " + helper::getAllTemplateParamsAsString(newFunc.templateParams) + "\n";
    }

    void outputTemplateParameterDefaultChanged(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) override {
        output += "The template parameters " + helper::getSingleTemplateParamAsString(oldFunc.templateParams.at(oldPosition)) + " default value was changed to " + helper::getSingleTemplateParamAsString(newParam) + " at position " + std::to_string(oldPosition) + ". Full params: " + helper::getAllTemplateParamsAsString(oldFunc.templateParams) + " -> " + helper::getAllTemplateParamsAsString(newFunc.templateParams) + "\n";
    }

    void outputNewSpecialization(const functionanalysis::FunctionInstance& newSpec) override {
        output += "A new specialization was added with the params " + helper::getAllParamsAsString(newSpec.params) + "\n";
    }

    void outputDeletedSpecialization(const functionanalysis::FunctionInstance& oldSpec) override {
        output += "The specialization with the params " + helper::getAllParamsAsString(oldSpec.params) + " was deleted\n";
    }

    // Variables
    void outputVariableDeleted(const variableanalysis::VariableInstance& var) override{
        output += "The variable has been deleted\n";
    }

    void outputVariableDefinitionDeleted(const variableanalysis::VariableInstance &var) override{
        output += "The variable definition in file " + var.filename + " has been deleted\n";
    }

    void outputVariableDefinitionAdded(const variableanalysis::VariableInstance &var) override{
        output += "The variable definition in file " + var.filename + " has been added\n";
    }

    void outputVariableFileChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override{
        output += "The variable file changed from " + oldVar.filename + " to " + newVar.filename + "\n";
    }

    void outputVariableLocationChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override{
        output += "The variable location has changed from " + helper::getAllNamespacesAsString(oldVar.location) + " to " + helper::getAllNamespacesAsString(newVar.location) + "\n";
    }

    void outputVariableTypeChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        output += "The variable type changed from " + oldVar.type + " to " + newVar.type + "\n";
    }

    void outputVariableDefaultValueChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        output += "The variable default value in the definition in file " + newVar.filename + "changed from " + oldVar.defaultValue + " to " + newVar.defaultValue + "\n";
    }

    void outputVariableStorageClassChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        output += "The variable storage class changed from " + oldVar.storageClass + " to " + newVar.storageClass + "\n";
    }

    void outputVariableInlineChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        if(oldVar.isInline){
            output += "The variable is not declared inline anymore\n";
        }else{
            output += "The variable is now declared inline\n";
        }
    }

    void outputVariableAccessSpecifierChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        output += "The variable access specifier changed from " + oldVar.accessSpecifier + " to " + newVar.accessSpecifier + "\n";
    }

    void outputVariableConstChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        if(oldVar.isConst){
            output += "The variable is not declared const anymore\n";
        }else{
            output += "The variable is now declared const\n";
        }
    }

    void outputVariableExplicitChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        if(oldVar.isExplicit){
            output += "The variable is not declared explicit anymore\n";
        }else{
            output += "The variable is now declared explicit\n";
        }
    }

    void outputVariableVolatileChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        if(oldVar.isVolatile){
            output += "The variable is not declared volatile anymore\n";
        }else{
            output += "The variable is now declared volatile\n";
        }
    }

    void outputVariableMutableChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        if(oldVar.isMutable){
            output += "The variable is not declared mutable anymore\n";
        }else{
            output += "The variable is now declared mutable\n";
        }
    }

    void outputVariableClassMember(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {
        if(oldVar.isClassMember){
            output += "The variable is not part of a class anymore, its original location was:" + helper::getAllNamespacesAsString(oldVar.location) + "\n";
        }else{
            output += "The variable is now part of a class, its new location is:" + helper::getAllNamespacesAsString(newVar.location) + "\n";
        }
    }

    bool endOfCurrentVariable() override {
        if(output != startingMessage){
            output += "\n";
            completeOutput += output;
            return true;
        }
        return false;
    }

    void outputObjectDeleted(const objectanalysis::ObjectInstance& obj) override {
        output += "The object has been deleted\n";
    }

    void outputObjectFilenameChange(const objectanalysis::ObjectInstance& oldObj, const objectanalysis::ObjectInstance& newObj) override {
        output += "The object filename changed from " + oldObj.filename + " to " + newObj.filename + "\n";
    }

    void outputObjectLocationChange(const objectanalysis::ObjectInstance& oldObj, const objectanalysis::ObjectInstance& newObj) override {
        output += "The object location has changed from " + helper::getAllNamespacesAsString(oldObj.location) + " to " + helper::getAllNamespacesAsString(newObj.location) + "\n";
    }

    void outputObjectFinalChange(const objectanalysis::ObjectInstance& oldObj, const objectanalysis::ObjectInstance& newObj) override {
        if(oldObj.isFinal){
            output += "The object is not declared final anymore\n";
        }else{
            output += "The object is now declared final\n";
        }
    }

    void outputObjectAbstractChange(const objectanalysis::ObjectInstance& oldObj, const objectanalysis::ObjectInstance& newObj) override {
        if(oldObj.isAbstract){
            output += "The object is not declared abstract anymore\n";
        }else{
            output += "The object is now declared abstract\n";
        }
    }

    void outputObjectTypeChange(const objectanalysis::ObjectInstance& oldObj, const objectanalysis::ObjectInstance& newObj) override {
        // TODO: C++ Enums are stupid, maybe change to string..
        output += "The object type changed\n";
    }

    bool endOfCurrentObject() override {
        if(output != startingMessage){
            output += "\n";
            completeOutput += output;
            return true;
        }
        return false;
    }


    bool printOut() override {
        llvm::outs()<<completeOutput;
        return true;
    }

    virtual ~ConsoleOutputHandler()= default;

};
