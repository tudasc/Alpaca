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

    std::vector<std::string> acceptedFileEndings{".cpp",".c",".h",".hpp",".C",".cc",".CPP",".cp",".cxx"};

    void listFiles(const std::string &path, std::vector<std::string>* listOfFiles, const std::vector<std::string>* excludedFiles){
        if(!fs::is_directory(path)){
            listOfFiles->push_back(fs::absolute(path));
            return;
        }
        fs::recursive_directory_iterator it(path);

        for(decltype(it) end; it != end; ++it){
            auto iteratedFile = it->path().string();
            if(it->is_directory()) {
                // get the last part of the path without any slashes
                std::string path = it->path().string();
                path = path.substr(path.find_last_of('/') + 1, path.length());
                // check if this folder is present in the excluded files
                if(std::find(excludedFiles->begin(), excludedFiles->end(), path) != excludedFiles->end()){
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
            listOfFiles->push_back(fs::absolute(it->path()));
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

    bool paramsAreEqual(const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& param1, const std::vector<std::pair<std::string, std::pair<std::string, std::string>>>& param2){
        if(param1.size() != param2.size()){
            return false;
        }
        for(unsigned int i = 0; i < param1.size(); i++){
            if(param1.at(i).first != param2.at(i).first){
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

    std::vector<std::string> excludeFiles(const std::string &path, std::vector<std::string> listOfFiles, const std::vector<std::string>* excludedFiles){
        std::vector<std::string> output = std::vector<std::string>();
        for (int i=0;i<listOfFiles.size();i++){
            bool found = false;

            for (const auto &exc: *excludedFiles){
                auto compexc = path + "/" + exc;
                try {
                    compexc = fs::canonical(compexc);
                } catch (fs::filesystem_error &e) {
                    continue;
                }
                if(listOfFiles.at(i).length() < compexc.length()){
                    continue;
                }
                auto sca = listOfFiles.at(i).substr(0, compexc.length());
                if(listOfFiles.at(i).substr(0, compexc.length()) == compexc){
                    found = true;
                    //listOfFiles.erase(std::remove(listOfFiles.begin(), listOfFiles.end(), listOfFiles.at(i)), listOfFiles.end());
                    break;
                }
            }
            if(!found){
                output.push_back(listOfFiles.at(i));
            }
        }
        return output;
    }

}