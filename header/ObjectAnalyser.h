#pragma once

#include <string>
#include <vector>
#include <clang/Frontend/FrontendActions.h>

namespace objectanalysis{
    class ObjectInstance{
        public:
        clang::TagTypeKind objectType;
        std::string name;
        std::string qualifiedName;
        std::vector<std::string> location;
        std::string filename;
        std::string filePosition;
        bool isAbstract;
        bool isFinal;
    };
}