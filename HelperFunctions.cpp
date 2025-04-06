#include <dirent.h>
#include <string>
#include <vector>
#include "header/HelperFunctions.h"

namespace helper {
    void listFiles(const std::string &path, std::vector<std::string> *listOfFiles) {
        if (auto dir = opendir(path.c_str())) {
            while (auto x = readdir(dir)) {
                if (x->d_name[0] == '.') continue;
                if (x->d_type == DT_DIR) listFiles(path + x->d_name + "/", listOfFiles);
                if (x->d_type == DT_REG) {
                    // TO-DO: Make list of acceptable file endings
                    if (std::string(x->d_name).substr(std::string(x->d_name).length() - 4) == ".cpp") {
                        listOfFiles->push_back(path + x->d_name);
                    }
                }
            }
            closedir(dir);
        }
    }

    std::string getAllParamsAsString(const std::vector<std::string>& params){
        std::string output = "[";
        for (const auto &item: params){
            output += item + ", ";
        }
        return output.substr(0, output.length()-2) + "]";
    }
}