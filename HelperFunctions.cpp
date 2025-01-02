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

    std::map<std::string, std::string> createRelativeMap(const std::vector<std::string>& files, const std::string& path, bool old);

    std::pair<std::vector<std::string>, std::vector<std::string>> filterUnchangedFiles(const std::vector<std::string>& oldFiles, const std::vector<std::string>& newFiles, std::string oldPath, std::string newPath){
        std::vector<std::string> oldOutput;
        std::vector<std::string> newOutput;
        // keep track of the found new files to later be able to add them to the output
        // this is done since new files that dont have an equivalent in the old files are not compared and therefore not added in the normal workflow
        std::vector<std::string> removedNewFiles;
        // create a map with the input paths of the old files as key
        auto oldRelativeMap = createRelativeMap(oldFiles, oldPath, true);
        // create a map with the relative paths of the new / old files as key (since for a file in both versions the relative path is the same)
        auto newRelativeMap = createRelativeMap(newFiles, newPath, false);

        for (int i=0; i<oldFiles.size(); i++) {

            if(oldFiles.at(i).empty()){
                continue;
            }

            // get the absolute old file path
            std::string absoluteOldFile = fs::absolute(oldFiles.at(i));

            // check if the file is present in the new files, if not the oldFile is added to the output
            if (newRelativeMap.find(oldRelativeMap.at(oldFiles.at(i))) == newRelativeMap.end()) {
                oldOutput.push_back(oldFiles.at(i));
                continue;
            }
            // get the absolute new filepath by using the relative path from the old file
            std::string absoluteNewFile = fs::absolute(newRelativeMap.at(oldRelativeMap.at(oldFiles.at(i))));

            std::array<char, 128> buffer{};
            std::string txt;
            // create diff command with the absolute file paths
            std::string command = "diff -q " + absoluteOldFile + " " + absoluteNewFile;
            // execute the command
            // TODO: this is a security risk, because the command is not checked for malicious code (not sure if relevant, but another way to do this would be nice)
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
            if (!pipe) {
                throw std::runtime_error("popen() failed!");
            }
            // read the output of the command
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                txt += buffer.data();
            }

            // if the output is not empty, the files have changes and need to be processed by ALPACA
            if (!txt.empty()) {
                oldOutput.push_back(oldFiles.at(i));
                newOutput.push_back(newRelativeMap.at(oldRelativeMap.at(oldFiles.at(i))));
            } else {
                // if the output is empty, the files are the same and this is noted in the removedFiles list
                removedNewFiles.push_back(newRelativeMap.at(oldRelativeMap.at(oldFiles.at(i))));
            }
        }

        // add all new files that were found to be equal to an old file to the output
        for (int i=0; i<newFiles.size(); i++) {
            // check if the file is either in the removed files or the new output, if not add it to the new output
            if (std::find(removedNewFiles.begin(), removedNewFiles.end(), newFiles.at(i)) == removedNewFiles.end() && std::find(newOutput.begin(), newOutput.end(), newFiles.at(i)) == newOutput.end()) {
                newOutput.push_back(newFiles.at(i));
            }
        }

        return std::make_pair(oldOutput, newOutput);
    }

    std::string makeRelative(const std::string& file, const std::string& dir) {
        return std::filesystem::relative(file, fs::canonical(std::filesystem::absolute(dir)).string());
    }

    std::map<std::string, std::string> createRelativeMap(const std::vector<std::string>& files, const std::string& path, const bool old){
        std::map<std::string, std::string> output;
        for (const auto &item: files){
            if(old) {
                output.insert(std::make_pair(item, makeRelative(item, path)));
            } else {
                output.insert(std::make_pair(makeRelative(item, path), item));
            }
        }
        return output;
    }
}