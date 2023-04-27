#pragma once

#include <vector>
#include "../include/json.hpp"
#include "Analyser.h"

using json = nlohmann::json;
using namespace analyse;

struct InsertAction {
 // new params, function moved from another file and came here, moved into new namespace
};

void to_json(json& j, const InsertAction& o) {
 //
}

struct RemoveActions {
 // function removed, parameter removed, moved to another file, moved into another namespace, maybe changed scope because a now private function might make it impossible to use
};

void to_json(json& j, const RemoveActions& o) {

}

struct ReplaceActions {
    // param type changed, return type changed, name changed
};

void to_json(json& j, const ReplaceActions& o) {

}

struct JSONFunction {
    // TODO: Visibility? Namespaces?
    std::string name; // insert that this is the old version?
    std::vector<std::string> arguments; // insert that this is the old version?
    // TODO: Maybe insert another layer called action to resemble the example, but seems redundant
    std::vector<InsertAction> insertActions;
    std::vector<RemoveActions> removeActions;
    std::vector<ReplaceActions> replaceActions;
    // TODO: Where to put changes in the declarations, another separate vector or as part of the actions?
};

void to_json(json& j, const JSONFunction& o){
    j = json{{"name", o.name}, {"arguments", o.arguments}, {"insertAction", o.insertActions}, {"removeActions", o.removeActions}, {"replaceActions", o.replaceActions}};
}

struct JSONFile {
    std::string file;
    std::vector <JSONFunction> functions;
};


void to_json(json& j, const JSONFile& o) {
    j = json{{"file", o.file}, {"functions", o.functions}};
}
