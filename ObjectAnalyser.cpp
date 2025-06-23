#include <string>
#include <utility>
#include <vector>
#include "header/ObjectAnalyser.h"
#include "header/OutputHandler.h"
#include "header/HelperFunctions.h"
#include "header/CodeMatcher.h"

using namespace objectanalysis;

namespace objectanalysis{
    class ObjectAnalyser{
        std::vector<ObjectInstance> oldObjects;
        std::vector<ObjectInstance> newObjects;
        OutputHandler* outputHandler;

        int findObject(const std::vector<ObjectInstance>& set, const ObjectInstance& objectInstance){
            // prioritylist (name has to be the same): qualifiedName > filename > closest match on location
            std::map<int, ObjectInstance> possibleMatches;
            for(int i=0;i<set.size();i++){
                if(set.at(i).qualifiedName == objectInstance.qualifiedName && set.at(i).filename == objectInstance.filename){
                    return i;
                }
                if(set.at(i).name == objectInstance.name){
                    possibleMatches.insert(std::make_pair(i, set.at(i)));
                }
            }
            for(const auto& possibleMatch: possibleMatches){
                // return the first match with the same filename
                if(possibleMatch.second.filename == objectInstance.filename){
                    return possibleMatch.first;
                }
            }

            // TODO: introduce a threshold at which the location is considered to be too far away and then maybe omit the file check
            // if there isn't even a match with the same filename, return the variable with the closest matching location
            int smallestDiff = std::numeric_limits<int>::max();
            int candidate = -1;
            for (const auto &item: possibleMatches){
                auto matrix = matcher::levenshteinDistance(item.second.location, objectInstance.location);
                if(smallestDiff > matrix[item.second.location.size()][objectInstance.location.size()]){
                    smallestDiff = matrix[item.second.location.size()][objectInstance.location.size()];
                    candidate = item.first;
                }
            }

            return candidate;
        }

    public:
        ObjectAnalyser(const std::vector<ObjectInstance>& oldObjects, const std::vector<ObjectInstance>& newObjects, OutputHandler* outputHandler){
            this->oldObjects = oldObjects;
            this->newObjects = newObjects;
            this->outputHandler = outputHandler;
        }

        void compareObjects(){
            for (const auto &item: oldObjects){
                outputHandler->initialiseObjectInstance(item);
                auto indexInNew = findObject(newObjects, item);
                if(indexInNew != -1){
                    ObjectInstance newItem = newObjects.at(indexInNew);
                    compareFilename(item, newItem);
                    compareLocation(item, newItem);
                    compareType(item, newItem);
                    if(item.isFinal != newItem.isFinal){
                        outputHandler->outputObjectFinalChange(item, newItem);
                    }
                    if(item.isAbstract != newItem.isAbstract){
                        outputHandler->outputObjectAbstractChange(item, newItem);
                    }
                    // TODO: compare inheritance
                }else{
                    outputHandler->outputObjectDeleted(item);
                }
                outputHandler->endOfCurrentObject();
            }
        }

    private:
        void compareFilename(const ObjectInstance& oldObj, const ObjectInstance& newObj){
            if(oldObj.filename != newObj.filename){
                outputHandler->outputObjectFilenameChange(oldObj, newObj);
            }
        }

        void compareLocation(const ObjectInstance& oldObj, const ObjectInstance& newObj){
            if(helper::getAllNamespacesAsString(oldObj.location) != helper::getAllNamespacesAsString(newObj.location)){
                outputHandler->outputObjectLocationChange(oldObj, newObj);
            }
        }

        void compareType(const ObjectInstance& oldObj, const ObjectInstance& newObj){
            if(oldObj.objectType != newObj.objectType){
                outputHandler->outputObjectTypeChange(oldObj, newObj);
            }
        }
    };
}