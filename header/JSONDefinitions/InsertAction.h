#pragma once

#include <string>
#include "../../include/json.hpp"

using json = nlohmann::json;

struct InsertAction {
    // new params, function moved from another file and came here, moved into new namespace
    std::string target;
    std::string insertionLocation;
    std::string reference;
    std::string value;
    std::string defaultValue;

    InsertAction(std::string target, std::string insertionLocation, std::string reference, std::string value, std::string defaultValue){
        this->target = target;
        this->insertionLocation = insertionLocation;
        this->reference = reference;
        this->value = value;
        this->defaultValue = defaultValue;
    }
};

static void to_json(json &j, const InsertAction &o) {
    j = json{{"target",            o.target},
             {"insertionLocation", o.insertionLocation},
             {"reference",         o.reference},
             {"value",             o.value},
             {"defaultValue",      o.defaultValue}};
}
