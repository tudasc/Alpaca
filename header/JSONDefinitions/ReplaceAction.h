#pragma once

#include <string>
#include "../../include/json.hpp"

using json = nlohmann::json;

struct ReplaceAction {
    // param type changed, return type changed, name changed
    std::string target;
    std::string oldValue;
    std::string newValue;

    ReplaceAction(std::string target, std::string oldValue, std::string newValue){
        this->target = target;
        this->oldValue = oldValue;
        this->newValue = newValue;
    }
};

static void to_json(json &j, const ReplaceAction &o) {
    j = json{{"target",   o.target},
             {"oldValue", o.oldValue},
             {"newValue", o.newValue}};
}