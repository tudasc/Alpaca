#pragma once

#include <map>
#include "Analyser.h"

namespace helper {

    void listFiles(const std::string &path, std::vector<std::string> *listOfFiles, const std::vector<std::string>* excludedFiles);
    std::string getAllParamsAsString(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& params);
    std::string getAllNamespacesAsString(const std::vector<std::string>& params);
    std::string stripCodeOfEmptySpaces(std::string code);
    std::string stripCodeOfComments(std::string code);
    std::vector<std::string> convertPairIntoFlatVector(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& vec);
    std::string retrieveFunctionHeader(const analysis::FunctionInstance& func);
}