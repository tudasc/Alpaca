#pragma once

#include <map>
#include "FunctionAnalyser.h"

namespace helper {
    void listFiles(const std::string &path, std::vector<std::string> *listOfFiles, const std::vector<std::string>* excludedFiles);
    std::string getAllParamsAsString(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& params);
    std::string getAllNamespacesAsString(const std::vector<std::string>& params);
    std::string stripCodeOfEmptySpaces(std::string code);
    std::string stripCodeOfComments(std::string code);
    std::vector<std::string> convertPairIntoFlatVector(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& vec);
    std::string retrieveFunctionHeader(const functionanalysis::FunctionInstance& func);
    std::string getAllTemplateParamsAsString(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& templateParams);
    std::string getSingleTemplateParamAsString(const std::pair<std::string, std::pair<std::string, std::string>>& templateParam);
    bool paramsAreEqual(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& param1, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& param2);
    std::vector<std::string> excludeFiles(const std::string &path, std::vector<std::string> *listOfFiles, const std::vector<std::string>* excludedFiles);
}