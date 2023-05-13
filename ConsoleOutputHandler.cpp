#include "header/OutputHandler.h"
#include <string>
#include <vector>
#include "header/Analyser.h"
#include <llvm/Support/CommandLine.h>
#include "header/HelperFunctions.h"

using namespace std;

class ConsoleOutputHandler : public OutputHandler {
public:
    std::string output = "";
    std::string completeOutput = "";
    std::string startingMessage = "";

    ConsoleOutputHandler(){}

    void initialiseFunctionInstance(const analyse::FunctionInstance &func) override {
        this->startingMessage = "-------------------------------------" + func.name + "-------------------------------------\n";
        output = startingMessage;
    }

    void outputNewParam(int oldPosition, const analyse::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam) override{
        output += "There is a new parameter (" + newParam.first + " " + newParam.second.first + ") after the param " + oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first
                + ". Full old params for reference: " +helper::getAllParamsAsString(oldFunc.params) + "\n";
    }

    void outputParamChange(int oldPosition, const analyse::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam) override {
        output += "A parameter changed from " + oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first + " to "
        + newParam.first + " " + newParam.second.first+ " at position " + to_string(oldPosition) + "\n";
    }

    void outputParamDefaultChange(int oldPosition, const analyse::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam) override {

        string oldValue = (oldFunc.params.at(oldPosition).second.second.empty()) ? oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first : oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first + " = " + oldFunc.params.at(oldPosition).second.second;
        string newValue = (newParam.second.second.empty()) ? newParam.first + " " + newParam.second.first : newParam.first + " " + newParam.second.first + " = " + newParam.second.second;

        output += "A default value of the param " + oldValue + " changed to " + newValue + "\n";
    }

    void outputDeletedParam(int oldPosition, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& oldParams) override {
        output += "The parameter " + oldParams.at(oldPosition).first + " " + oldParams.at(oldPosition).second.first + " was deleted\n";
     }

     void outputNewReturn(const analyse::FunctionInstance &newFunc, std::string oldReturn) override {
        output += "The return type changed from " + oldReturn + " to " + newFunc.returnType + "\n";
    }

    void outputNewScope(const analyse::FunctionInstance& newFunc, const analyse::FunctionInstance& oldFunc) override {
        output += "The scope changed from " + oldFunc.scope + " to " + newFunc.scope + "\n";
    }

    void outputNewNamespaces(const analyse::FunctionInstance &newFunc, const analyse::FunctionInstance& oldFunc) override {
        output += "The function moved namespaces from " + helper::getAllNamespacesAsString(oldFunc.location) + " to "
                + helper::getAllNamespacesAsString(newFunc.location) + "\n";
    }

    void outputNewFilename(const analyse::FunctionInstance &newFunc, const analyse::FunctionInstance& oldFunc) override{
        output += "The function definition moved files from " + oldFunc.filename + " to " + newFunc.filename + "\n";
    }

    void outputNewDeclPositions(const analyse::FunctionInstance &newFunc, std::vector<std::string> addedDecl) override {
        output += "The function has new declarations in the files " + helper::getAllNamespacesAsString(addedDecl) + "\n";
    }

    void outputDeletedDeclPositions(const analyse::FunctionInstance &newFunc, std::vector<std::string> deletedDecl, const analyse::FunctionInstance& oldFunc) override {
        output += "Declarations in the files " + helper::getAllNamespacesAsString(deletedDecl) + " were deleted\n";
    }

    void outputDeletedFunction(const analyse::FunctionInstance &deletedFunc, bool overloaded) override {
        if(overloaded){
            output += "The overloaded function with the params " + helper::getAllParamsAsString(deletedFunc.params) + " was deleted\n";
        } else {
            output += "The function was deleted from the file " + deletedFunc.filename + "\n";
        }
    }

    void outputOverloadedDisclaimer(const analyse::FunctionInstance &func, std::string percentage) override {
        output += "DISCLAIMER: There is code similarity of " + percentage
                + "% between the old overloaded function and the in the following analyzed instance of the overloaded function\n";
    }

    void outputRenamedFunction(const analyse::FunctionInstance &newFunc, std::string oldName, std::string percentage) override {
        output += "The function was renamed to \"" + newFunc.name +
                  "\" with a code similarity of " + percentage + "%\n";
    }

    bool printOut() override {
        llvm::outs()<<completeOutput;
        return true;
    }

    bool endOfCurrentFunction() override{
        if(output != startingMessage){
            output += "\n";
            completeOutput += output;
            return true;
        }
        return false;
    }

    virtual ~ConsoleOutputHandler(){}

};
