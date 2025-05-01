#pragma once

#include <map>

namespace helper {

    void listFiles(const std::string &path, std::vector<std::string> *listOfFiles);
    std::string getAllParamsAsString(const std::vector<std::pair<std::string, std::string>> & params);
    std::string getAllNamespacesAsString(const std::vector<std::string>& params);
    std::string stripCodeOfEmptySpaces(std::string code);
    std::string stripCodeOfComments(std::string code);
}