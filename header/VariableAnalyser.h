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
        std::vector<std::string> location;
        std::string filename;
        std::string filePosition;
        std::string storageClass;
        bool isInline;
        std::string accessSpecifier;
        bool isConst;
        std::string qualifiers;
        bool isExplicit;
        bool isVolatile;
        bool isMutable;
        [[nodiscard]] bool equals(const VariableInstance& variableInstance) const{
            return this->isClassMember == variableInstance.isClassMember && this->name == variableInstance.name && this->qualifiedName == variableInstance.qualifiedName && this->type == variableInstance.type && this->filename == variableInstance.filename && this->storageClass == variableInstance.storageClass && this->isInline == variableInstance.isInline && this->accessSpecifier == variableInstance.accessSpecifier && this->isConst == variableInstance.isConst && this->qualifiers == variableInstance.qualifiers && this->isExplicit == variableInstance.isExplicit && this->isVolatile == variableInstance.isVolatile && this->isMutable == variableInstance.isMutable;
        }
    };
}