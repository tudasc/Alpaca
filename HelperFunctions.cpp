#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
#include "header/HelperFunctions.h"
#include <filesystem>
#include <llvm/Support/raw_ostream.h>
#include <csignal>

namespace fs = std::filesystem;

namespace helper {

    std::vector<std::string> acceptableFileEndings{".cpp",".c",".h",".hpp",".C",".cc",".CPP",".cp",".cxx",".cppm"};

    void listFiles(const std::string &path, std::vector<std::string>* listOfFiles){
        for(const auto& entry : fs::recursive_directory_iterator(path)){
            if(entry.is_directory() || !(std::find(acceptableFileEndings.begin(), acceptableFileEndings.end(), fs::path(entry.path()).extension()) != acceptableFileEndings.end())){
                continue;
            }
            listOfFiles->push_back(fs::canonical(entry.path()));
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

    std::vector<std::string> convertPairIntoFlatVector(std::vector<std::pair<std::string, std::string>> vec){
        std::vector<std::string> output;
        for (const auto &item: vec){
            output.push_back(item.first);
        }
        return output;
    }

    std::string getAllParamsInRegularForm(const std::vector<std::pair<std::string, std::string>>& params){
        std::string output = "(";
        for (const auto &item: params){
            output += item.first + " " + item.second + ", ";
        }
        return output.substr(0, output.length()-2) + ")";
    }

    std::string retrieveFunctionHeader(const analyse::FunctionInstance& func){
        return func.returnType + " " + func.qualifiedName + getAllParamsInRegularForm(func.params);
    }
}