#pragma once

#include <string>
#include <vector>
#include "FunctionAnalyser.h"
#include "VariableAnalyser.h"

class OutputHandler {
public:
    OutputHandler() = default;

    // FUNCTIONS
    virtual void initialiseFunctionInstance(const functionanalysis::FunctionInstance& func) = 0;

    virtual void outputNewParam(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) {};

    virtual void outputParamChange(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) {};

    virtual void outputParamDefaultChange(int oldPosition, const functionanalysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const functionanalysis::FunctionInstance& newFunc) {};

    virtual void outputDeletedParam(int oldPosition, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& oldParams, const functionanalysis::FunctionInstance& newFunc) {};

    virtual void outputNewReturn(const functionanalysis::FunctionInstance& newFunc, std::string oldReturn) {};

    virtual void outputNewScope(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) {};

    virtual void outputNewNamespaces(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) {};

    virtual void outputNewFilename(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) {};

    virtual void outputNewDeclPositions(const functionanalysis::FunctionInstance& newFunc, std::vector<std::string> addedDecl) {};

    virtual void outputDeletedDeclPositions(const functionanalysis::FunctionInstance& newFunc, std::vector<std::string> deletedDecl, const functionanalysis::FunctionInstance& oldFunc) {};

    virtual void outputDeletedFunction(const functionanalysis::FunctionInstance& deletedFunc, bool overloaded) {};

    virtual void outputRenamedFunction(const functionanalysis::FunctionInstance& newFunc, std::string oldName, std::string percentage) {};

    virtual void outputOverloadedDisclaimer(const functionanalysis::FunctionInstance& func, std::string percentage) {};

    virtual void outputStorageClassChange(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) {};

    virtual void outputFunctionSpecifierChange(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) {};

    virtual void outputFunctionConstChange(const functionanalysis::FunctionInstance& newFunc, const functionanalysis::FunctionInstance& oldFunc) {};

    virtual bool printOut() = 0;

    virtual bool endOfCurrentFunction() = 0;

    // VARIABLES
    virtual void initialiseVariableInstance(const variableanalysis::VariableInstance& var) = 0;

    virtual void outputVariableDeleted(const variableanalysis::VariableInstance& var) {};

    virtual void outputVariableDefinitionDeleted(const variableanalysis::VariableInstance& var) {};

    virtual void outputVariableDefinitionAdded(const variableanalysis::VariableInstance& var) {};

    virtual void outputVariableFileChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableLocationChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableTypeChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableDefaultValueChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableStorageClassChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableInlineChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableAccessSpecifierChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableConstChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableExplicitChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableVolatileChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableMutableChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual void outputVariableClassMember(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) {};

    virtual bool endOfCurrentVariable() = 0;
};
