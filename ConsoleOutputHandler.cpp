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
    std::string startingMessage = "";

    ConsoleOutputHandler(){}

    void initialiseFunctionInstance(const analyse::FunctionInstance &func) override {
        this->startingMessage = "-------------------------------------" + func.name + "-------------------------------------\n";
        output += startingMessage;
    }

    void outputNewParam(int position, const analyse::FunctionInstance& newFunc, int numberOfNewParams) override{
        output += "There is a new parameter at the position " + to_string(position+1)
        + ". The complete new parameters are " + helper::getAllParamsAsString(newFunc.params) + "\n";
    }

    void outputParamChange(int position, std::pair<std::string, std::string> oldParam, const analyse::FunctionInstance &newFunc) override {
        output += "A parameter changed from " + oldParam.first + " " + oldParam.second + " to "
        + newFunc.params.at(position).first + " " + newFunc.params.at(position).second + ". The new complete params are "
        + helper::getAllParamsAsString(newFunc.params) + "\n";
    }

    void outputDeletedParam(int position, const std::vector<std::pair<std::string, std::string>> &oldParams, const analyse::FunctionInstance &newFunc, int numberOfDeletedParams) override {
        output += "The parameter " + oldParams.at(position).first + " " + oldParams.at(position).second + " was deleted. The new complete parameters are "
                + helper::getAllParamsAsString(newFunc.params) + "\n";
     }

     void outputNewReturn(const analyse::FunctionInstance &newFunc, std::string oldReturn) override {
        output += "The return type changed from " + oldReturn + " to " + newFunc.returnType + "\n";
    }

    void outputNewScope(const analyse::FunctionInstance &newFunc, std::string oldScope) override {
        output += "The scope changed from " + oldScope + " to " + newFunc.scope + "\n";
    }

    void outputNewNamespaces(const analyse::FunctionInstance &newFunc, const analyse::FunctionInstance& oldFunc) override {
        output += "The function moved namespaces from " + helper::getAllNamespacesAsString(oldFunc.location) + " to "
                + helper::getAllNamespacesAsString(newFunc.location) + "\n";
    }

    void outputNewFilename(const analyse::FunctionInstance &newFunc, std::string oldName) override{
        output += "The function definition moved files from " + oldName + " to " + newFunc.filename + "\n";
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
        if(output != startingMessage){
            output += "\n";
            llvm::outs()<<output;
            return true;
        }
        return false;
    }

    virtual ~ConsoleOutputHandler(){}

};
