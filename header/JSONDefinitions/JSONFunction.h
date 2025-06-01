#pragma once

#include <vector>
#include "../../include/json.hpp"
#include "../FunctionAnalyser.h"
#include "InsertAction.h"
#include "RemoveAction.h"
#include "ReplaceAction.h"
#include "../HelperFunctions.h"


struct JSONFunction {
    std::string name; // insert that this is the old version?
    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> arguments; // insert that this is the old version?
    std::vector<InsertAction> insertActions;
    std::vector<RemoveAction> removeActions;
    std::vector<ReplaceAction> replaceActions;

    JSONFunction(std::string name, std::vector<std::pair<std::string, std::pair<std::string, std::string>>> arguments){
        this->name = name;
        this->arguments = arguments;
        this->insertActions = std::vector<InsertAction>();
        this->removeActions = std::vector<RemoveAction>();
        this->replaceActions = std::vector<ReplaceAction>();
    }
};

static void to_json(json &j, const JSONFunction &o) {
    j = json{{"name",           o.name},
             {"arguments",      helper::convertPairIntoFlatVector(o.arguments)},
             {"insertAction",   o.insertActions},
             {"removeActions",  o.removeActions},
             {"replaceActions", o.replaceActions}};
}

