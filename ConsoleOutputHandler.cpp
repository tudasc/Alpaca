#include "header/OutputHandler.h"
#include <string>
#include <vector>
#include "header/Analyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/HelperFunctions.h"

using namespace std;

class ConsoleOutputHandler : public OutputHandler {
public:
    std::string output;
    std::string completeOutput;
    std::string startingMessage;

    ConsoleOutputHandler()= default;

    void initialiseFunctionInstance(const analysis::FunctionInstance &func) override {
        this->startingMessage = "-------------------------------------" + func.qualifiedName + " (Function) -------------------------------------\n";
        output = startingMessage;
    }

    void initialiseVariableInstance(const variableanalysis::VariableInstance &var) override {
        this->startingMessage = "-------------------------------------" + var.qualifiedName + " (Variable) -------------------------------------\n";
        output = startingMessage;
    }

    void outputNewParam(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) override{
        if(oldPosition != 0) {
            output +=
                    "There is a new parameter (" + newParam.first + " " + newParam.second.first + ") after the param " +
                    oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first
                    + ". Full params: " + helper::getAllParamsAsString(oldFunc.params) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";
        }else {
            output += "There is a new parameter (" + newParam.first + " " + newParam.second.first + ")" + ". Full params: " + helper::getAllParamsAsString(oldFunc.params) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";
        }
    }

    void outputParamChange(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) override {
        output += "A parameter changed from " + oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first + " to "
        + newParam.first + " " + newParam.second.first+ " at position " + to_string(oldPosition) + ". Full params: " + helper::getAllParamsAsString(oldFunc.params) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";
    }

    void outputParamDefaultChange(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) override {

        string oldValue = (oldFunc.params.at(oldPosition).second.second.empty()) ? oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first : oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first + " = " + oldFunc.params.at(oldPosition).second.second;
        string newValue = (newParam.second.second.empty()) ? newParam.first + " " + newParam.second.first : newParam.first + " " + newParam.second.first + " = " + newParam.second.second;

        output += "A default value of the param " + oldValue + " changed to " + newValue + ". Full params: " + helper::getAllParamsAsString(oldFunc.params) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";
    }

    void outputDeletedParam(int oldPosition, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& oldParams, const analysis::FunctionInstance& newFunc) override {
        output += "The parameter " + oldParams.at(oldPosition).first + " " + oldParams.at(oldPosition).second.first + " was deleted " + ". Full params: " + helper::getAllParamsAsString(oldParams) + " -> " + helper::getAllParamsAsString(newFunc.params) + "\n";;
     }

     void outputNewReturn(const analysis::FunctionInstance &newFunc, std::string oldReturn) override {
        output += "The return type changed from " + oldReturn + " to " + newFunc.returnType + "\n";
    }

    void outputNewScope(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) override {
        output += "The scope changed from " + oldFunc.scope + " to " + newFunc.scope + "\n";
    }

    void outputNewNamespaces(const analysis::FunctionInstance &newFunc, const analysis::FunctionInstance& oldFunc) override {
        output += "The function moved namespaces from " + helper::getAllNamespacesAsString(oldFunc.location) + " to "
                + helper::getAllNamespacesAsString(newFunc.location) + "\n";
    }

    void outputNewFilename(const analysis::FunctionInstance &newFunc, const analysis::FunctionInstance& oldFunc) override{
        output += "The function definition moved files from " + oldFunc.filename + " to " + newFunc.filename + "\n";
    }

    void outputNewDeclPositions(const analysis::FunctionInstance &newFunc, std::vector<std::string> addedDecl) override {
        output += "The function has new declarations in the files " + helper::getAllNamespacesAsString(addedDecl) + "\n";
    }

    void outputDeletedDeclPositions(const analysis::FunctionInstance &newFunc, std::vector<std::string> deletedDecl, const analysis::FunctionInstance& oldFunc) override {
        output += "Declarations in the files " + helper::getAllNamespacesAsString(deletedDecl) + " were deleted\n";
    }

    void outputDeletedFunction(const analysis::FunctionInstance &deletedFunc, bool overloaded) override {
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

    void outputOverloadedDisclaimer(const analysis::FunctionInstance &func, std::string percentage) override {
        output += "DISCLAIMER: There is code similarity of " + percentage
                + "% between the old overloaded function and the in the following analyzed instance of the overloaded function\n";
    }

    void outputRenamedFunction(const analysis::FunctionInstance &newFunc, std::string oldName, std::string percentage) override {
        output += "The function was renamed to \"" + newFunc.name +
                  "\" with a code similarity of " + percentage + "%\n";
    }

    void outputStorageClassChange(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) override{
        output += "The functions storage class changed from " + oldFunc.storageClass + " to " + newFunc.storageClass + "\n";
    }

    void outputFunctionSpecifierChange(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) override{
        output += "The function specifier changed from " + oldFunc.memberFunctionSpecifier + " to " + newFunc.memberFunctionSpecifier + "\n";
    }

    bool endOfCurrentFunction() override{
        if(output != startingMessage){
            output += "\n";
            completeOutput += output;
            return true;
        }
        return false;
    }

    // Variables

    void outputVariableFileChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override{

    }

    void outputVariableTypeChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableDefaultValueChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableStorageClassChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableInlineChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableAccessSpecifierChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableConstChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableExplicitChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableVolatileChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableMutableChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    void outputVariableClassMember(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) override {

    }

    bool endOfCurrentVariable() override {
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
