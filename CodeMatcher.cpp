#include <string>
#include <vector>
#include "header/CodeMatcher.h"
#include <llvm/Support/CommandLine.h>
#include "header/Analyser.h"
#include "header/HelperFunctions.h"


namespace matcher{

    using namespace std;

    vector<string> convertStringToVector(const string& str){
        vector<string> output;
        for (const auto &item: str){
            output.push_back(to_string(item));
        }
        return output;
    }

    vector<vector<int>> levenshteinDistance(const vector<string>& s1, const vector<string>& s2){
        const size_t len1 = s1.size(), len2 = s2.size();
        vector<vector<int>> d(len1 + 1, vector<int>(len2 + 1));

        d[0][0] = 0;
        for(int i = 1; i <= len1; ++i) d[i][0] = i;
        for(int i = 1; i <= len2; ++i) d[0][i] = i;

        for(int i = 1; i <= len1; ++i)
            for(int j = 1; j <= len2; ++j)
                d[i][j] = min(min(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
        return d;
    }



    double compareFunctionBodies(const analysis::FunctionInstance& oldFunc, const analysis::FunctionInstance& newFunc) {
        auto strippedNewCode = helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(newFunc.body));
        auto strippedOldCode = helper::stripCodeOfEmptySpaces(helper::stripCodeOfComments(oldFunc.body));
        // TODO: reevaluate
        if(strippedNewCode == "{}" || strippedOldCode == "{}"){
            return 0;
        }
        double distance = matcher::levenshteinDistance(convertStringToVector(strippedOldCode),
                                                                        convertStringToVector(strippedNewCode))[strippedOldCode.length()][strippedNewCode.length()];
        double sum = strippedOldCode.length() + strippedNewCode.length();
        return ((sum - distance) / sum) * 100;
    }

    /*
     * Converts the complex param structure to a flat vector by removing the param names and appending default values directly to the type string
     */
    vector<string> paramsToFlatString(const vector<pair<string, pair<string, string>>>& param){
        vector<string> output;
        for (const auto &item: param){
            string token;
            if(item.second.second.empty()) {
                token = item.first;
            }else{
                token = item.first + "=" + item.second.second;
            }
            output.push_back(token);
        }
        return output;
    }

    vector<Operation> getOptimalParamConversion(vector<pair<string, pair<string, string>>> oldParamStructure, vector<pair<string, pair<string, string>>> newParamStructure){
        vector<string> oldParams = paramsToFlatString(oldParamStructure);
        vector<string> newParams = paramsToFlatString(newParamStructure);

        vector<Operation> operations;
        // get the full levenshteinDistance matrix
        vector<vector<int>> levenshteinMatrix = levenshteinDistance(oldParams, newParams);

        // traverse the matrix from the bottom right up to the top left to retrieve the optimal set of operations
        int i = levenshteinMatrix.size()-1, j = levenshteinMatrix.at(i).size()-1;

        // instantly return empty list if the distance is 0
        if(levenshteinMatrix[i][j] == 0){
            return operations;
        }

        if(i == 0){
            for(int x = 0;x<j;x++){
                Operation op = Operation(Operation::Types::INSERTION, 0, newParamStructure.at(x));
                operations.insert(operations.begin(), op);
            }
        }
        if(j == 0){
            for(int x = 0;x<i;x++){
                Operation op = Operation(Operation::Types::DELETION, x, oldParamStructure.at(x));
                operations.insert(operations.begin(), op);
            }
        }

        while(j > 0 && i > 0){
            // evaluate which of the adjacent cells is the optimal next step
            int currentCell = levenshteinMatrix[i][j];
            int diagonalCell = levenshteinMatrix[i-1][j-1];
            int leftCell = levenshteinMatrix[i][j-1];
            int aboveCell = levenshteinMatrix[i-1][j];

            // check if the diagonal cell is the smallest and if yes, save a replacement operation
            if(diagonalCell <= leftCell && diagonalCell <= aboveCell && (diagonalCell == currentCell || diagonalCell == currentCell - 1)){
                j--, i--;
                if(diagonalCell == currentCell - 1){
                    Operation op = Operation(Operation::Types::REPLACEMENT, i, oldParamStructure.at(i), newParamStructure.at(j));
                    operations.insert(operations.begin(), op);
                }// otherwise no operation needed (identical token)
            }
            // check left cell and if yes, save an insertion
            else if(leftCell <= aboveCell && (leftCell == currentCell || leftCell == currentCell - 1)){
                j--;
                // subtracting 1 from the position to compensate for the lack of this particular param in the old function params
                Operation op = Operation(Operation::Types::INSERTION, i-1, newParamStructure.at(j));
                operations.insert(operations.begin(), op);
            }
            // otherwise save a deletion
            else{
                i--;
                Operation op = Operation(Operation::Types::DELETION, i, oldParamStructure.at(i));
                operations.insert(operations.begin(), op);
            }
        }
        return operations;
    }

}

