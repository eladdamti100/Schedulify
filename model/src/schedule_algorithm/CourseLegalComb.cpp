#include "CourseLegalComb.h"

// Generates all valid combinations of groups for a given course
vector<CourseSelection> CourseLegalComb::generate(const Course& course) {
    vector<CourseSelection> combinations;

    try {
        // Collect all non-empty group types
        vector<pair<string, vector<const Group*>>> availableGroupTypes;

        if (!course.blocks.empty()) {
            vector<const Group*> blockGroups;
            for (const auto& group : course.blocks) {
                blockGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"blocks", blockGroups});
        }

        if (!course.Lectures.empty()) {
            vector<const Group*> lectureGroups;
            for (const auto& group : course.Lectures) {
                lectureGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"lectures", lectureGroups});
        }

        if (!course.Tirgulim.empty()) {
            vector<const Group*> tutorialGroups;
            for (const auto& group : course.Tirgulim) {
                tutorialGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"tutorials", tutorialGroups});
        }

        if (!course.labs.empty()) {
            vector<const Group*> labGroups;
            for (const auto& group : course.labs) {
                labGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"labs", labGroups});
        }

        if (!course.DepartmentalSessions.empty()) {
            vector<const Group*> deptGroups;
            for (const auto& group : course.DepartmentalSessions) {
                deptGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"departmental", deptGroups});
        }

        if (!course.Reinforcements.empty()) {
            vector<const Group*> reinforcementGroups;
            for (const auto& group : course.Reinforcements) {
                reinforcementGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"reinforcements", reinforcementGroups});
        }

        if (!course.Guidance.empty()) {
            vector<const Group*> guidanceGroups;
            for (const auto& group : course.Guidance) {
                guidanceGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"guidance", guidanceGroups});
        }

        if (!course.OptionalColloquium.empty()) {
            vector<const Group*> colloquiumGroups;
            for (const auto& group : course.OptionalColloquium) {
                colloquiumGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"colloquium", colloquiumGroups});
        }

        if (!course.Registration.empty()) {
            vector<const Group*> registrationGroups;
            for (const auto& group : course.Registration) {
                registrationGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"registration", registrationGroups});
        }

        if (!course.Thesis.empty()) {
            vector<const Group*> thesisGroups;
            for (const auto& group : course.Thesis) {
                thesisGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"thesis", thesisGroups});
        }

        if (!course.Project.empty()) {
            vector<const Group*> projectGroups;
            for (const auto& group : course.Project) {
                projectGroups.push_back(&group);
            }
            availableGroupTypes.push_back({"project", projectGroups});
        }

        // If no groups are available, return empty combinations
        if (availableGroupTypes.empty()) {
            Logger::get().logWarning("No groups available for course ID " + to_string(course.id));
            return combinations;
        }

        // Generate all combinations using recursive approach
        vector<const Group*> currentCombination;
        generateCombinationsRecursive(availableGroupTypes, 0, currentCombination, combinations, course.id);

        if (combinations.empty()) {
            Logger::get().logWarning("No valid combinations generated for course ID " + to_string(course.id));
        } else {
            Logger::get().logInfo("Generated " + to_string(combinations.size()) + " valid combinations for course ID " + to_string(course.id));
        }
    } catch (const exception& e) {
        Logger::get().logError("Exception in CourseLegalComb::generate for course ID " + to_string(course.id) + ": " + e.what());
    }

    return combinations;
}

// Recursive helper method to generate all combinations
void CourseLegalComb::generateCombinationsRecursive(
        const vector<pair<string, vector<const Group*>>>& availableGroupTypes,
        int currentTypeIndex,
        vector<const Group*>& currentCombination,
        vector<CourseSelection>& combinations,
        int courseId) {

    if (currentTypeIndex == availableGroupTypes.size()) {
        // We have selected one group from each type, check for conflicts
        if (!hasAnyCombinationConflict(currentCombination)) {
            // Create CourseSelection from current combination
            CourseSelection selection = createCourseSelection(currentCombination, availableGroupTypes, courseId);
            combinations.push_back(selection);
        }
        return;
    }

    // Try each group in the current type
    const auto& currentType = availableGroupTypes[currentTypeIndex];
    for (const Group* group : currentType.second) {
        currentCombination.push_back(group);
        generateCombinationsRecursive(availableGroupTypes, currentTypeIndex + 1, currentCombination, combinations, courseId);
        currentCombination.pop_back();
    }
}

// Helper method to check if any groups in the combination have conflicts
bool CourseLegalComb::hasAnyCombinationConflict(const vector<const Group*>& groups) {
    for (size_t i = 0; i < groups.size(); i++) {
        for (size_t j = i + 1; j < groups.size(); j++) {
            if (hasGroupConflict(groups[i], groups[j])) {
                return true;
            }
        }
    }
    return false;
}

// Helper method to create CourseSelection from the selected groups
CourseSelection CourseLegalComb::createCourseSelection(
        const vector<const Group*>& selectedGroups,
        const vector<pair<string, vector<const Group*>>>& availableGroupTypes,
        int courseId) {

    CourseSelection selection;
    selection.courseId = courseId;
    selection.lectureGroup = nullptr;
    selection.tutorialGroup = nullptr;
    selection.labGroup = nullptr;
    selection.blockGroup = nullptr;
    selection.departmentalGroup = nullptr;
    selection.reinforcementGroup = nullptr;
    selection.guidanceGroup = nullptr;
    selection.colloquiumGroup = nullptr;
    selection.registrationGroup = nullptr;
    selection.thesisGroup = nullptr;
    selection.projectGroup = nullptr;

    // Map selected groups to their types
    for (size_t i = 0; i < selectedGroups.size(); i++) {
        const string& typeName = availableGroupTypes[i].first;
        const Group* group = selectedGroups[i];

        if (typeName == "lectures") {
            selection.lectureGroup = group;
        } else if (typeName == "tutorials") {
            selection.tutorialGroup = group;
        } else if (typeName == "labs") {
            selection.labGroup = group;
        } else if (typeName == "blocks") {
            selection.blockGroup = group;
        } else if (typeName == "departmental") {
            selection.departmentalGroup = group;
        } else if (typeName == "reinforcements") {
            selection.reinforcementGroup = group;
        } else if (typeName == "guidance") {
            selection.guidanceGroup = group;
        } else if (typeName == "colloquium") {
            selection.colloquiumGroup = group;
        } else if (typeName == "registration") {
            selection.registrationGroup = group;
        } else if (typeName == "thesis") {
            selection.thesisGroup = group;
        } else if (typeName == "project") {
            selection.projectGroup = group;
        }
    }

    return selection;
}

// Helper method to check if two groups have any conflicting sessions
bool CourseLegalComb::hasGroupConflict(const Group* group1, const Group* group2) {
    if (!group1 || !group2) {
        return false;
    }

    try {
        for (const auto& session1 : group1->sessions) {
            for (const auto& session2 : group2->sessions) {
                if (TimeUtils::isOverlap(&session1, &session2)) {
                    return true;
                }
            }
        }
    } catch (const exception& e) {
        Logger::get().logError("Exception in hasGroupConflict: " + string(e.what()));
        return true;
    }

    return false; // No conflicts found
}