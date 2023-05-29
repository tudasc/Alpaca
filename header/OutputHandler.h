#pragma once

#include <string>
#include <vector>
#include "Analyser.h"
#include "VariableAnalyser.h"

class OutputHandler {
public:
    OutputHandler() = default;

    // FUNCTIONS
    virtual void initialiseFunctionInstance(const analysis::FunctionInstance& func) = 0;

    virtual void outputNewParam(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) = 0;

    virtual void outputParamChange(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) = 0;

    virtual void outputParamDefaultChange(int oldPosition, const analysis::FunctionInstance& oldFunc, std::pair<std::string, std::pair<std::string, std::string>> newParam, const analysis::FunctionInstance& newFunc) = 0;

    virtual void outputDeletedParam(int oldPosition, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& oldParams, const analysis::FunctionInstance& newFunc) = 0;

    virtual void outputNewReturn(const analysis::FunctionInstance& newFunc, std::string oldReturn) = 0;

    virtual void outputNewScope(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) = 0;

    virtual void outputNewNamespaces(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) = 0;

    virtual void outputNewFilename(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) = 0;

    virtual void outputNewDeclPositions(const analysis::FunctionInstance& newFunc, std::vector<std::string> addedDecl) = 0;

    virtual void outputDeletedDeclPositions(const analysis::FunctionInstance& newFunc, std::vector<std::string> deletedDecl, const analysis::FunctionInstance& oldFunc) = 0;

    virtual void outputDeletedFunction(const analysis::FunctionInstance& deletedFunc, bool overloaded) = 0;

    virtual void outputRenamedFunction(const analysis::FunctionInstance& newFunc, std::string oldName, std::string percentage) = 0;

    virtual void outputOverloadedDisclaimer(const analysis::FunctionInstance& func, std::string percentage) = 0;

    virtual void outputStorageClassChange(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) = 0;

    virtual void outputFunctionSpecifierChange(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) = 0;

    virtual void outputFunctionConstChange(const analysis::FunctionInstance& newFunc, const analysis::FunctionInstance& oldFunc) = 0;

    virtual bool printOut() = 0;

    virtual bool endOfCurrentFunction() = 0;

    // VARIABLES
    virtual void initialiseVariableInstance(const variableanalysis::VariableInstance& var) = 0;

    virtual void outputVariableDeleted(const variableanalysis::VariableInstance& var) = 0;

    virtual void outputVariableDefinitionDeleted(const variableanalysis::VariableInstance& var) = 0;

    virtual void outputVariableDefinitionAdded(const variableanalysis::VariableInstance& var) = 0;

    virtual void outputVariableFileChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableLocationChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableTypeChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableDefaultValueChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableStorageClassChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableInlineChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableAccessSpecifierChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableConstChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableExplicitChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableVolatileChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableMutableChange(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual void outputVariableClassMember(const variableanalysis::VariableInstance& oldVar, const variableanalysis::VariableInstance& newVar) = 0;

    virtual bool endOfCurrentVariable() = 0;
};
