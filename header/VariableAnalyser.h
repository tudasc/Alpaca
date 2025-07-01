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
        bool isDefinition;
        std::vector<VariableInstance> definitions;
        [[nodiscard]] bool equals(const VariableInstance& variableInstance) const{

            for (const auto &item: this->definitions){
                bool found = false;
                for (const auto &item2: variableInstance.definitions){
                    if (item.equals(item2)){
                        found = true;
                        break;
                    }
                }
                if (!found){
                    return false;
                }
            }
            return this->isClassMember == variableInstance.isClassMember && this->name == variableInstance.name && this->qualifiedName == variableInstance.qualifiedName && this->type == variableInstance.type && this->defaultValue == variableInstance.defaultValue && this->filename == variableInstance.filename && this->storageClass == variableInstance.storageClass && this->isInline == variableInstance.isInline && this->accessSpecifier == variableInstance.accessSpecifier && this->isConst == variableInstance.isConst && this->qualifiers == variableInstance.qualifiers && this->isExplicit == variableInstance.isExplicit && this->isVolatile == variableInstance.isVolatile && this->isMutable == variableInstance.isMutable;
        }
    };
}