#pragma once

#include <string>
#include <vector>
#include "Analyser.h"

class OutputHandler {
public:
    OutputHandler() {}

    virtual void initialiseFunctionInstance(const analyse::FunctionInstance& func) = 0;

    virtual void outputNewParam(int position, const analyse::FunctionInstance& newFunc, int numberOfNewParams) = 0;

    virtual void outputParamChange(int position, std::pair<std::string, std::string> oldParam, const analyse::FunctionInstance& newFunc) = 0;

    virtual void outputDeletedParam(int position, const std::vector<std::pair<std::string, std::string>>& oldParams, const analyse::FunctionInstance& newFunc, int numberOfDeletedParams) = 0;

    virtual void outputNewReturn(const analyse::FunctionInstance& newFunc, std::string oldReturn) = 0;

    virtual void outputNewScope(const analyse::FunctionInstance& newFunc, std::string oldScope) = 0;

    virtual void outputNewNamespaces(const analyse::FunctionInstance& newFunc, const analyse::FunctionInstance& oldFunc) = 0;

    virtual void outputNewFilename(const analyse::FunctionInstance& newFunc, const analyse::FunctionInstance& oldFunc) = 0;

    virtual void outputNewDeclPositions(const analyse::FunctionInstance& newFunc, std::vector<std::string> addedDecl) = 0;

    virtual void outputDeletedDeclPositions(const analyse::FunctionInstance& newFunc, std::vector<std::string> deletedDecl, const analyse::FunctionInstance& oldFunc) = 0;

    virtual void outputDeletedFunction(const analyse::FunctionInstance& deletedFunc, bool overloaded) = 0;

    virtual void outputRenamedFunction(const analyse::FunctionInstance& newFunc, std::string oldName, std::string percentage) = 0;

    virtual void outputOverloadedDisclaimer(const analyse::FunctionInstance& func, std::string percentage) = 0;

    virtual bool printOut() = 0;

    virtual bool endOfCurrentFunction() = 0;
};
