#pragma once

#include <string>
#include <vector>

namespace objectanalysis{

    enum class ObjectType{
        CLASS,
        STRUCT,
        ENUM,
        ENUM_CLASS,
        ENUM_STRUCT,
        ENUM_UNION,
        NAMESPACE,
        UNKNOWN
    };

    class ObjectInstance{
        public:
        ObjectType objectType;
        std::string name;
        std::string qualifiedName;
        std::vector<std::string> location;
        std::string filename;
        bool isAbstract;
        bool isFinal;
    };
}