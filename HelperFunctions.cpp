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

    std::vector<std::string> acceptedFileEndings{".cpp",".c",".h",".hpp",".C",".cc",".CPP",".cp",".cxx",".cppm"};

    void listFiles(const std::string &path, std::vector<std::string>* listOfFiles, const std::vector<std::string>* excludedFiles){
        fs::recursive_directory_iterator it(path);

        for(decltype(it) end; it != end; ++it){
            auto iteratedFile = it->path().string();
            if(it->is_directory()) {
                if (std::find_if(excludedFiles->begin(), excludedFiles->end(), [iteratedFile, path](std::string str){
                    if(str.at(str.length()-1) == '/' || str.at(str.length()-1) == '\\'){
                        str = str.substr(0, str.length() - 1);
                    }
                    if(str.at(0) == '/' || str.at(0) == '\\'){
                        str = str.substr(1, str.length());
                    }
                    str = path + str;
                    if(iteratedFile.length() < str.length()){
                        return false;
                    }
                    return str == iteratedFile;
                }) != excludedFiles->end()){
                    it.disable_recursion_pending();
                }
                continue;
            }
            if(std::find_if(excludedFiles->begin(), excludedFiles->end(), [it](const std::string& str){
                if(it->path().string().length() < str.length()){
                    return false;
                }
                return str == it->path().string().substr(it->path().string().length() - str.length(), str.length());
            }) != excludedFiles->end() || !(std::find(acceptedFileEndings.begin(), acceptedFileEndings.end(), fs::path(it->path()).extension()) != acceptedFileEndings.end())){
                continue;
            }

            listOfFiles->push_back(fs::canonical(it->path()));
        }
    }

    std::string getAllParamsAsString(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& params){
        std::string output = "[";
        for (const auto &item: params){
            if(!item.second.second.empty()){
                output += item.first + " " + item.second.first + " = " + item.second.second + ", ";
            }else{
                output += item.first + " " + item.second.first + ", ";
            }
        }
        return output.substr(0, output.length()-2) + "]";
    }

    std::string getAllNamespacesAsString(const std::vector<std::string>& params){
        std::string output;
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

    std::vector<std::string> convertPairIntoFlatVector(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& vec){
        std::vector<std::string> output;
        for (const auto &item: vec){
            output.push_back(item.first);
        }
        return output;
    }

    std::string getAllParamsInRegularForm(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& params){
        std::string output = "(";
        for (const auto &item: params){
            if(!item.second.second.empty()){
                output += item.first + " " + item.second.first + " = " + item.second.second + ", ";
            }else{
                output += item.first + " " + item.second.first + ", ";
            }
        }
        return output.substr(0, output.length()-2) + ")";
    }

    std::string retrieveFunctionHeader(const functionanalysis::FunctionInstance& func){
        return func.returnType + " " + func.qualifiedName + getAllParamsInRegularForm(func.params);
    }

    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> convertFlatParamsIntoParamStructure(const std::vector<std::string>& params){
        std::vector<std::pair<std::string, std::pair<std::string, std::string>>> output;
        output.reserve(params.size());
        for (const auto &item: params){
            output.emplace_back(item, std::make_pair("", ""));
        }
        return output;
    }

    std::string getAllTemplateParamsAsString(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& templateParams){
        std::string output = "<";
        for (const auto &item: templateParams){
            if(!item.second.second.empty()){
                output += item.first + " " + item.second.first + " = " + item.second.second + ", ";
            }else{
                output += item.first + " " + item.second.first + ", ";
            }
        }
        return output.substr(0, output.length()-2) + ">";
    }

    bool paramsAreEqual(const functionanalysis::FunctionInstance& oldFunc, const functionanalysis::FunctionInstance& newFunc){
        if(oldFunc.params.size() != newFunc.params.size()){
            return false;
        }
        for(unsigned int i = 0; i < oldFunc.params.size(); i++){
            if(oldFunc.params.at(i).first != newFunc.params.at(i).first){
                return false;
            }
        }
        return true;
    }

    std::string getSingleTemplateParamAsString(const std::pair<std::string, std::pair<std::string, std::string>>& param){
        if(!param.second.second.empty()){
            return param.first + " " + param.second.first + " = " + param.second.second;
        }else{
            return param.first + " " + param.second.first;
        }
    }
}