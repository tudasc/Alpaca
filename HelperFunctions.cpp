#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
#include "header/HelperFunctions.h"

namespace helper {

    void listFiles(const std::string &path, std::vector<std::string> *listOfFiles) {
        if (auto dir = opendir(path.c_str())) {
            while (auto x = readdir(dir)) {
                if (x->d_name[0] == '.') continue;
                if (x->d_type == DT_DIR) listFiles(path + x->d_name + "/", listOfFiles);
                if (x->d_type == DT_REG) {
                    // TODO: Make list of acceptable file endings
                    if (std::string(x->d_name).substr(std::string(x->d_name).length() - 4) == ".cpp" || std::string(x->d_name).substr(std::string(x->d_name).length() - 2) == ".h") {
                        listOfFiles->push_back(path + x->d_name);
                    }
                }
            }
            closedir(dir);
        }
    }

    std::string getAllParamsAsString(const std::vector<std::pair<std::string, std::string>>& params){
        std::string output = "[";
        for (const auto &item: params){
            output += item.first + " " + item.second + ", ";
        }
        return output.substr(0, output.length()-2) + "]";
    }

    std::string getAllNamespacesAsString(const std::vector<std::string>& params){
        std::string output = "";
        for (const auto &item: params){
            output += item + "::";
        }
        return output.substr(0, output.length()-2);
    }

    std::string stripCodeOfComments(std::string code){
        std::string strippedCode;
        std::string delimiter = "\n";

        unsigned long pos = 0;
        std::string singleLine;
        while ((pos = code.find(delimiter)) != std::string::npos) {
            singleLine = code.substr(0, pos);
            if(unsigned int comment = singleLine.find("//")){
                singleLine = singleLine.substr(0, comment);
            }
            strippedCode += singleLine;
            code.erase(0, pos + delimiter.length());
        }
        strippedCode += code;

        code = strippedCode;
        strippedCode = "";
        delimiter = "/*";
        pos = 0;

        while ((pos = code.find(delimiter)) != std::string::npos) {
            strippedCode += code.substr(0, pos);
            pos = code.find("*/");
            code.erase(0, pos + delimiter.length());
        }
        strippedCode += code;

        return strippedCode;
    }

    std::string stripCodeOfEmptySpaces(std::string code){
        code.erase(std::remove_if(code.begin(), code.end(), ::isspace), code.end());
        return code;
    }
}