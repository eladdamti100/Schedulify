#ifndef COURSE_LEGAL_COMB_H
#define COURSE_LEGAL_COMB_H

#include "model_interfaces.h"
#include "inner_structs.h"
#include "Logger.h"
#include "TimeUtils.h"
#include <vector>
#include <string>

using namespace std;

class CourseLegalComb {
public:
    // Generates all valid combinations of groups for a given course
    vector<CourseSelection> generate(const Course& course);

private:
    // Helper method to check if two groups have any conflicting sessions
    bool hasGroupConflict(const Group* group1, const Group* group2);

    // Recursive helper method to generate all combinations
    void generateCombinationsRecursive(
            const vector<pair<string, vector<const Group*>>>& availableGroupTypes,
            int currentTypeIndex,
            vector<const Group*>& currentCombination,
            vector<CourseSelection>& combinations,
            int courseId);

    // Helper method to check if any groups in the combination have conflicts
    bool hasAnyCombinationConflict(const vector<const Group*>& groups);

    // Helper method to create CourseSelection from the selected groups
    CourseSelection createCourseSelection(
            const vector<const Group*>& selectedGroups,
            const vector<pair<string, vector<const Group*>>>& availableGroupTypes,
            int courseId);
};
#endif // COURSE_LEGAL_COMB_H