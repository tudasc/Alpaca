#include "header/OutputHandler.h"
#include <string>
#include <vector>
#include <fstream>
#include "header/JSONFile.h"
#include "header/JSONDefinitions/JSONFunction.h"
#include "header/JSONDefinitions/InsertAction.h"
#include "header/JSONDefinitions/RemoveAction.h"
#include "header/JSONDefinitions/ReplaceAction.h"
#include "header/HelperFunctions.h"
#include "include/json.hpp"

using namespace std;
using namespace customJSON;

class JSONOutputHandler : public OutputHandler {
public:
    std::map<string, JSONFile> files;
    JSONFunction* currentFunc{};
    std::string currentFile;

    JSONOutputHandler(){
        this->files = std::map<string, JSONFile>();
    }

    void initialiseFunctionInstance(const analysis::FunctionInstance &func) override {
        if(files.count(func.filename) == 0) {
            JSONFile newFile = JSONFile(func.filename);
            files.insert(make_pair(newFile.file, newFile));
        }
        currentFunc = new JSONFunction(func.name, func.params);
        currentFile = func.filename;
    }

    void initialiseVariableInstance(const variableanalysis::VariableInstance& var) override {
        if(files.count(var.filename) == 0) {
            JSONFile newFile = JSONFile(var.filename);
            files.insert(make_pair(newFile.file, newFile));
        }
        // TODO: currentVar
    }


    void outputNewParam(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) override{
        std::string insertionLocation;
        std::string reference;

        if(oldFunc.params.empty()){
            reference = "";
        } else if(oldPosition == 0){
            insertionLocation = "before";
            reference = oldFunc.params.at(0).second.first;
        }else{
            insertionLocation = "after";
            reference = oldFunc.params.at(oldPosition).second.first;
        }
        InsertAction insertAction = InsertAction("parameter", insertionLocation, reference, newParam.first + " " + newParam.second.first, newParam.second.second);
        currentFunc->insertActions.push_back(insertAction);
    }

    void outputParamChange(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) override {
        ReplaceAction replaceAction = ReplaceAction("parameter type", oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first, newParam.first + " " + newParam.second.first);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputParamDefaultChange(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) override {
        // if the default is now empty and had a value earlier, it constitutes a new param
        if(newParam.second.second.empty() && !oldFunc.params.at(oldPosition).second.second.empty()){
            outputNewParam(oldPosition, oldFunc, newParam, newFunc);
            return;
        }

        string oldValue = (oldFunc.params.at(oldPosition).second.second.empty()) ? oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first : oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first + " = " + oldFunc.params.at(oldPosition).second.second;
        string newValue = newParam.second.second.empty() ? newParam.first + " " + newParam.second.first : newParam.first + " " + newParam.second.first + " = " + newParam.second.second;

        ReplaceAction replaceAction = ReplaceAction("default param value", oldValue, newValue);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputDeletedParam(int oldPosition, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& oldParams, const FunctionInstance& newFunc) override {
        RemoveAction removeAction = RemoveAction("parameter", oldParams.at(oldPosition).second.first);
        currentFunc->removeActions.push_back(removeAction);
    }

    void outputNewReturn(const analysis::FunctionInstance &newFunc, std::string oldReturn) override {
        ReplaceAction replaceAction = ReplaceAction("return type", oldReturn, newFunc.returnType);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputNewScope(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) override {
        if(newFunc.scope == "private"){
            outputDeletedFunction(oldFunc, false);
        } // otherwise do nothing, as no action makes sense here?
    }

    void outputNewNamespaces(const analysis::FunctionInstance &newFunc, const analysis::FunctionInstance& oldFunc) override {
        ReplaceAction replaceAction = ReplaceAction("namespace", helper::getAllNamespacesAsString(oldFunc.location), helper::getAllNamespacesAsString(newFunc.location));
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputNewFilename(const analysis::FunctionInstance &newFunc, const analysis::FunctionInstance &oldFunc) override{
        /* remove function in file 1
        RemoveAction removeAction = RemoveAction("function definition", helper::retrieveFunctionHeader(oldFunc));
        currentFunc->removeActions.push_back(removeAction);
        // insert function in file 2? */
        // TODO: removeAction and then Insertion or is this a replacementAction?

    }

    void outputNewDeclPositions(const analysis::FunctionInstance &newFunc, std::vector<std::string> addedDecl) override {
        // TODO: in my opinion not compatible / usable -> not really an insertion or is it?
    }

    void outputDeletedDeclPositions(const analysis::FunctionInstance &newFunc, std::vector<std::string> deletedDecl, const analysis::FunctionInstance& oldFunc) override {
        RemoveAction removeAction = RemoveAction("function declaration", helper::retrieveFunctionHeader(oldFunc));
        for (const auto &item: deletedDecl){
            if(files.count(item) == 0){
                JSONFile newFile = JSONFile(item);
                files.insert(make_pair(newFile.file, newFile));
            }
            if(item == currentFile){
                currentFunc->removeActions.push_back(removeAction);
            }else {
                JSONFunction temp = JSONFunction(oldFunc.name, oldFunc.params);
                temp.removeActions.push_back(removeAction);
                files.find(item)->second.functions.push_back(temp);
            }
        }
    }

    void outputDeletedFunction(const analysis::FunctionInstance &deletedFunc, bool overloaded) override {
        if(deletedFunc.isDeclaration){
            RemoveAction removeAction = RemoveAction("function declaration", helper::retrieveFunctionHeader(deletedFunc));
            currentFunc->removeActions.push_back(removeAction);
        }else {
            RemoveAction removeAction = RemoveAction("function definition",
                                                     helper::retrieveFunctionHeader(deletedFunc));
            currentFunc->removeActions.push_back(removeAction);
        }
    }

    void outputOverloadedDisclaimer(const analysis::FunctionInstance &func, std::string percentage) override {
        // not relevant for this format, stays empty
    }

    void outputRenamedFunction(const analysis::FunctionInstance &newFunc, std::string oldName, std::string percentage) override {
        ReplaceAction replaceAction = ReplaceAction("function name", oldName, newFunc.name);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputStorageClassChange(const analysis::FunctionInstance &newFunc, const analysis::FunctionInstance &oldFunc) override {
        // TODO: is this a replacement?
    }

    void outputFunctionSpecifierChange(const analysis::FunctionInstance &newFunc, const analysis::FunctionInstance &oldFunc) override{
        // TODO: is this a replacement?
    }

    bool endOfCurrentFunction() override{
        if(!currentFunc->replaceActions.empty() || !currentFunc->removeActions.empty() || !currentFunc->insertActions.empty()){
            files.find(currentFile)->second.functions.push_back(*currentFunc);
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
        return true;
    }

    bool printOut() override {
        vector<JSONFile> vec;
        // remove all empty files
        for(const auto &item : files){
            if(!item.second.functions.empty()){
                vec.push_back(item.second);
            }
        }
        std::ofstream output("output.json");
        json j = vec;
        output << j << std::endl;
        return true;
    }

    virtual ~JSONOutputHandler()= default;

};
