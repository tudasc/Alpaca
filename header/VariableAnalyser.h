#pragma once

#include <string>
#include <vector>

namespace variableanalysis{

    class VariableInstance{
    public:
        bool isClassMember;
        std::string name;
        std::string qualifiedName;
        std::string type;
        std::string defaultValue;
        std::vector<std::string> location;
        std::string filename;
        std::string storageClass;
        bool isInline;
        std::string accessSpecifier;
        bool isConst;
        std::string qualifiers;
        bool isExplicit;
        bool isVolatile;
        bool isMutable;
    };
}