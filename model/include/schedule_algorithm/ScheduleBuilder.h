#ifndef SCHEDULE_BUILDER_H
#define SCHEDULE_BUILDER_H

#include "model_interfaces.h"
#include "CourseLegalComb.h"
#include "inner_structs.h"
#include "getSession.h"
#include "TimeUtils.h"
#include "logger.h"
#include "ScheduleDatabaseWriter.h"

#include <unordered_map>
#include <algorithm>
#include <vector>
#include <map>

class ScheduleBuilder {
public:
    // Public method to build all possible valid schedules from a list of courses
    vector<InformativeSchedule> build(const vector<Course>& courses, const string& semester);

private:
    static mutex courseInfoMapMutex;
    static unordered_map<int, CourseInfo> courseInfoMap;

    int totalSchedulesGenerated = 0;
    static string currentSemester;

    // Recursive backtracking function to build all valid schedules
    void backtrack(int index, const vector<vector<CourseSelection>>& allOptions, vector<CourseSelection>& current,
            vector<InformativeSchedule>& results);

    // Checks if there is a time conflict between two CourseSelections
    static bool hasConflict(const CourseSelection& a, const CourseSelection& b);

    // Converts a vector of CourseSelections to an InformativeSchedule
    static InformativeSchedule convertToInformativeSchedule(const vector<CourseSelection>& selections, int index);

    // Helper method to process all sessions in a group and add them to the day schedules
    static void processGroupSessions(const CourseSelection& selection, const Group* group, const string& sessionType,
                                     map<int, vector<ScheduleItem>>& daySchedules);

    // Helper method to build course info map
    static void buildCourseInfoMap(const vector<Course>& courses);

    // Get course name from courseInfoMap
    static string getCourseNameById(int courseId);

    // Get course raw id from courseInfoMap
    static string getCourseRawIdById(int courseId);

    // Calculate metadata fields of a given schedule
    static void calculateScheduleMetrics(InformativeSchedule& schedule);

    static string generateUniqueScheduleId(const string& semester, int index);
};

#endif // SCHEDULE_BUILDER_H