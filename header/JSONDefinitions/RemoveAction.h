#pragma once

#include <string>
#include "../../include/json.hpp"

using json = nlohmann::json;

struct RemoveAction {
    // function removed, parameter removed, moved to another file, moved into another namespace, maybe changed scope because a now private function might make it impossible to use
    std::string target;
    std::string value;

    RemoveAction(std::string target, std::string value){
        this->target = target;
        this->value = value;
    }
};

static void to_json(json &j, const RemoveAction &o) {
    j = json{{"target", o.target},
             {"value",  o.value}};
}