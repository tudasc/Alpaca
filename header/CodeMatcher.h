#pragma once

#include <string>
#include "Analyser.h"

namespace matcher{

    class Operation{
    public:
        enum Types { INSERTION, DELETION, REPLACEMENT };
        Types type;
        int positionInOldParam;
        std::pair<std::string, std::pair<std::string, std::string>> oldParam;
        std::pair<std::string, std::pair<std::string, std::string>> newParam;

        // constructor with both set is for a replacement
        Operation(Types type, int pos, std::pair<std::string, std::pair<std::string, std::string>> oldParam, std::pair<std::string, std::pair<std::string, std::string>> newParam){
            if(type != Types::REPLACEMENT){
                throw new std::exception();
            }
            this->type = Types::REPLACEMENT;
            this->positionInOldParam = pos;
            this->oldParam = oldParam;
            this->newParam = newParam;
        }

        Operation(Types type, int pos, std::pair<std::string, std::pair<std::string, std::string>> param){
            if(type == Types::DELETION){
                this->type = type;
                this->positionInOldParam = pos;
                this->oldParam = param;
            }else if(type == Types::INSERTION){
                this->type = type;
                this->positionInOldParam = pos;
                this->newParam = param;
            }else {
                // this shouldn't happen
                throw new std::exception();
            }
        }
    };

    double compareFunctionBodies(const analysis::FunctionInstance& oldFunc, const analysis::FunctionInstance& newFunc);
    std::vector<Operation> getOptimalParamConversion(std::vector<std::pair<std::string, std::pair<std::string, std::string>>> oldParamStructure, std::vector<std::pair<std::string, std::pair<std::string, std::string>>> newParamStructure);
}
