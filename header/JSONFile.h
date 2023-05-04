#pragma once

#include <vector>
#include "../include/json.hpp"
#include "Analyser.h"
#include "JSONDefinitions/JSONFunction.h"


using json = nlohmann::json;
using namespace analyse;
namespace customJSON {

    struct JSONFile {
        std::string file;
        std::vector<JSONFunction> functions;

        JSONFile(std::string name){
            this->file = name;
            this->functions = std::vector<JSONFunction>();
        }

    };

    static void to_json(json &j, const JSONFile &o) {
        j = json{{"file",      o.file},
                 {"functions", o.functions}};
    }
}