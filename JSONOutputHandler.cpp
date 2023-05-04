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

    // TODO: Clean this up to make sure no classes without changes are printed
    void initialiseFunctionInstance(const analyse::FunctionInstance &func) override {
        if(files.count(func.filename) == 0) {
            JSONFile newFile = JSONFile(func.filename);
            files.insert(make_pair(newFile.file, newFile));
        }
        currentFunc = new JSONFunction(func.name, func.params);
        currentFile = func.filename;
    }

    // TODO: Add checks for default values
    void outputNewParam(int position, const analyse::FunctionInstance& newFunc, int numberOfNewParams) override{
        std::string insertionLocation;
        std::string reference;
        if(position == 0){
            insertionLocation = "before";
            reference = "";
        }else{
            insertionLocation = "after";
            reference = newFunc.params.at(position-1).second;
        }
        InsertAction insertAction = InsertAction("parameter", insertionLocation, reference, newFunc.params.at(position).first + " " + newFunc.params.at(position).second, "");
        currentFunc->insertActions.push_back(insertAction);
    }

    void outputParamChange(int position, std::pair<std::string, std::string> oldParam, const analyse::FunctionInstance &newFunc) override {
        ReplaceAction replaceAction = ReplaceAction("parameter type", oldParam.first, newFunc.params.at(position).first);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputDeletedParam(int position, const std::vector<std::pair<std::string, std::string>> &oldParams, const analyse::FunctionInstance &newFunc, int numberOfDeletedParams) override {
        RemoveAction removeAction = RemoveAction("parameter", oldParams.at(position).second);
        currentFunc->removeActions.push_back(removeAction);
    }

    void outputNewReturn(const analyse::FunctionInstance &newFunc, std::string oldReturn) override {
        ReplaceAction replaceAction = ReplaceAction("return type", oldReturn, newFunc.returnType);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputNewScope(const analyse::FunctionInstance &newFunc, std::string oldScope) override {
        // TODO: should i treat this as a remove action?
    }

    void outputNewNamespaces(const analyse::FunctionInstance &newFunc, const analyse::FunctionInstance& oldFunc) override {
        // TODO: treat a namespace switch as a replace action or a remove -> insert action?
        ReplaceAction replaceAction = ReplaceAction("namespace", helper::getAllNamespacesAsString(oldFunc.location), helper::getAllNamespacesAsString(newFunc.location));
        currentFunc->replaceActions.push_back(replaceAction);
    }

    void outputNewFilename(const analyse::FunctionInstance &newFunc, std::string oldName) override{
        // TODO: ask how to handle
    }

    void outputNewDeclPositions(const analyse::FunctionInstance &newFunc, std::vector<std::string> addedDecl) override {
        // TODO: ask how to handle
    }

    void outputDeletedDeclPositions(const analyse::FunctionInstance &newFunc, std::vector<std::string> deletedDecl, const analyse::FunctionInstance& oldFunc) override {
        // TODO: ask how to handle
    }

    void outputDeletedFunction(const analyse::FunctionInstance &deletedFunc, bool overloaded) override {
        // TODO: ask how to handle
    }

    void outputOverloadedDisclaimer(const analyse::FunctionInstance &func, std::string percentage) override {
        // not relevant for this format, stays empty
    }

    void outputRenamedFunction(const analyse::FunctionInstance &newFunc, std::string oldName, std::string percentage) override {
        ReplaceAction replaceAction = ReplaceAction("function name", oldName, newFunc.name);
        currentFunc->replaceActions.push_back(replaceAction);
    }

    vector<JSONFile> getFilesAsVector(){
        vector<JSONFile> output;
        for (const auto &item: files){
            output.push_back(item.second);
        }
        return output;
    }

    bool printOut() override {

        /* remove all empty functions
        for (const auto &item: files){
            vector<JSONFunction> newFunc = vector<JSONFunction>();
            for (const auto &func: item.second.functions){
                if(func.replaceActions.size() != 0 || func.removeActions.size() != 0 || func.insertActions.size() != 0){
                    newFunc.push_back(func);
                }
            }
            files.find(item.first)->second.functions = newFunc;
        }
         */
        // remove all empty files
        for(const auto &item : files){
            if(item.second.functions.size() == 0){
                files.erase(item.first);
            }
        }

        std::ofstream output("output.json");
        json j = getFilesAsVector();
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
