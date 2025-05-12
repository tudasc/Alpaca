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
#include <llvm/Support/CommandLine.h>

using namespace std;
using namespace customJSON;

class JSONOutputHandler : public OutputHandler {
public:
    std::map<string, JSONFile> files;
    JSONFunction* currentFunc;
    std::string currentFile;

    JSONOutputHandler(){
        this->files = std::map<string, JSONFile>();
    }

    void initialiseFunctionInstance(const analyse::FunctionInstance &func) override {
        if(files.count(func.filename) == 0) {
            JSONFile newFile = JSONFile(func.filename);
            files.insert(make_pair(newFile.file, newFile));
        }
        currentFunc = new JSONFunction(func.name, func.params);
        currentFile = func.filename;
    }

    void outputNewParam(int oldPosition, const analyse::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam) override{
        std::string insertionLocation;
        std::string reference;
        if(oldPosition == 0){
            insertionLocation = "before";
            reference = oldFunc.params.at(0).second.first;
        }else{
            insertionLocation = "after";
            reference = oldFunc.params.at(oldPosition).second.first;
        }
        InsertAction insertAction = InsertAction("parameter", insertionLocation, reference, newParam.first + " " + newParam.second.first, newParam.second.second);
        currentFunc->insertActions.push_back(insertAction);
    }

    void outputParamChange(int oldPosition, const analyse::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam) override {
        ReplaceAction replaceAction = ReplaceAction("parameter type", oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first, newParam.first + " " + newParam.second.first);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputParamDefaultChange(int oldPosition, const analyse::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam) override {
        // if the default is now empty and had a value earlier, it constitutes a new param
        if(newParam.second.second.empty() && !oldFunc.params.at(oldPosition).second.second.empty()){
            outputNewParam(oldPosition, oldFunc, newParam);
            return;
        }

        string oldValue = (oldFunc.params.at(oldPosition).second.second.empty()) ? oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first : oldFunc.params.at(oldPosition).first + " " + oldFunc.params.at(oldPosition).second.first + " = " + oldFunc.params.at(oldPosition).second.second;
        string newValue = newParam.second.second.empty() ? newParam.first + " " + newParam.second.first : newParam.first + " " + newParam.second.first + " = " + newParam.second.second;

        ReplaceAction replaceAction = ReplaceAction("default param value", oldValue, newValue);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputDeletedParam(int oldPosition, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& oldParams) override {
        RemoveAction removeAction = RemoveAction("parameter", oldParams.at(oldPosition).second.first);
        currentFunc->removeActions.push_back(removeAction);
    }

    void outputNewReturn(const analyse::FunctionInstance &newFunc, std::string oldReturn) override {
        ReplaceAction replaceAction = ReplaceAction("return type", oldReturn, newFunc.returnType);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputNewScope(const analyse::FunctionInstance& newFunc, const analyse::FunctionInstance& oldFunc) override {
        if(newFunc.scope == "private"){
            outputDeletedFunction(oldFunc, false);
        } // otherwise do nothing, as no action makes sense here?
    }

    void outputNewNamespaces(const analyse::FunctionInstance &newFunc, const analyse::FunctionInstance& oldFunc) override {
        ReplaceAction replaceAction = ReplaceAction("namespace", helper::getAllNamespacesAsString(oldFunc.location), helper::getAllNamespacesAsString(newFunc.location));
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputNewFilename(const analyse::FunctionInstance &newFunc, const analyse::FunctionInstance &oldFunc) override{
        /* remove function in file 1
        RemoveAction removeAction = RemoveAction("function definition", helper::retrieveFunctionHeader(oldFunc));
        currentFunc->removeActions.push_back(removeAction);
        // insert function in file 2? */
        // TODO: removeAction and then Insertion or is this a replacementAction?

    }

    void outputNewDeclPositions(const analyse::FunctionInstance &newFunc, std::vector<std::string> addedDecl) override {
        // TODO: in my opinion not compatible / usable -> not really an insertion or is it?
    }

    void outputDeletedDeclPositions(const analyse::FunctionInstance &newFunc, std::vector<std::string> deletedDecl, const analyse::FunctionInstance& oldFunc) override {
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

    void outputDeletedFunction(const analyse::FunctionInstance &deletedFunc, bool overloaded) override {
        RemoveAction removeAction = RemoveAction("function definition", helper::retrieveFunctionHeader(deletedFunc));
        currentFunc->removeActions.push_back(removeAction);
    }

    void outputOverloadedDisclaimer(const analyse::FunctionInstance &func, std::string percentage) override {
        // not relevant for this format, stays empty
    }

    void outputRenamedFunction(const analyse::FunctionInstance &newFunc, std::string oldName, std::string percentage) override {
        ReplaceAction replaceAction = ReplaceAction("function name", oldName, newFunc.name);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    bool printOut() override {
        vector<JSONFile> vec;
        // remove all empty files
        for(const auto &item : files){
            if(item.second.functions.size() != 0){
                vec.push_back(item.second);
            }
        }
        std::ofstream output("output.json");
        json j = vec;
        output << j << std::endl;
        return true;
    }

    bool endOfCurrentFunction() override{
        if(currentFunc->replaceActions.size() != 0 || currentFunc->removeActions.size() != 0 || currentFunc->insertActions.size() != 0){
            files.find(currentFile)->second.functions.push_back(*currentFunc);
            return true;
        }
        return false;
    }

    virtual ~JSONOutputHandler(){}

};
