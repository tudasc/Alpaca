#include <string>
#include <vector>
#include "header/CodeMatcher.h"
#include <llvm/Support/CommandLine.h>
#include "header/Analyser.h"


namespace matcher{
    std::string stripCodeOfComments(std::string code){
        std::string strippedCode;
        std::string delimiter = "\n";

        size_t pos = 0;
        std::string singleLine;
        while ((pos = code.find(delimiter)) != std::string::npos) {
            singleLine = code.substr(0, pos);
            if(int comment = singleLine.find("//")){
                singleLine = singleLine.substr(0, comment);
            }
            strippedCode += singleLine;
            code.erase(0, pos + delimiter.length());
        }
        strippedCode += code;

        code = strippedCode;
        strippedCode = "";
        delimiter = "/*";
        pos = 0;

        while ((pos = code.find(delimiter)) != std::string::npos) {
            strippedCode += code.substr(0, pos);
            pos = code.find("*/");
            code.erase(0, pos + delimiter.length());
        }
        strippedCode += code;

        return strippedCode;
    }

    std::string stripCodeOfEmptySpaces(std::string code){
        code.erase(std::remove_if(code.begin(), code.end(), ::isspace), code.end());
        return code;
    }
    unsigned int levenshteinDistanceBetweenCodeBlocks(const std::string& s1, const std::string& s2){
        const std::size_t len1 = s1.size(), len2 = s2.size();
        std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

        d[0][0] = 0;
        for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
        for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

        for(unsigned int i = 1; i <= len1; ++i)
            for(unsigned int j = 1; j <= len2; ++j)
                d[i][j] = std::min(std::min(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
        return d[len1][len2];
    }

    double compareFunctionBodies(analyse::FunctionInstance oldFunc, analyse::FunctionInstance newFunc) {
        auto strippedNewCode = matcher::stripCodeOfEmptySpaces(matcher::stripCodeOfComments(newFunc.body));
        auto strippedOldCode = matcher::stripCodeOfEmptySpaces(matcher::stripCodeOfComments(oldFunc.body));
        double distance = matcher::levenshteinDistanceBetweenCodeBlocks(strippedOldCode,strippedNewCode);
        double sum = strippedOldCode.length() + strippedNewCode.length();
        return ((sum - distance) / sum) * 100;
    }

}

