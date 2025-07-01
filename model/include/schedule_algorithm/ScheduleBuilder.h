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
    // Updated build method - now accepts semester as parameter
    vector<InformativeSchedule> build(const vector<Course>& courses, int semester);

private:
    static unordered_map<string, CourseInfo> courseInfoMap;

    // Progressive writing state
    bool progressiveWriting = false;
    int totalSchedulesGenerated = 0;

    // Recursive backtracking function to build all valid schedules
    void backtrack(int index, const vector<vector<CourseSelection>>& allOptions, vector<CourseSelection>& current,
                   vector<InformativeSchedule>& results, int semester);

    // Checks if there is a time conflict between two CourseSelections
    static bool hasConflict(const CourseSelection& a, const CourseSelection& b);

    // Converts a vector of CourseSelections to an InformativeSchedule
    static InformativeSchedule convertToInformativeSchedule(const vector<CourseSelection>& selections, int index, int semester);

    // Helper method to process all sessions in a group and add them to the day schedules
    static void processGroupSessions(const CourseSelection& selection, const Group* group, const string& sessionType,
                                     map<int, vector<ScheduleItem>>& daySchedules);

    // Get a course name from course map according to its course_key
    static string getCourseNameByKey(const string& courseKey);

    // Get a course raw id from course map according to its course_key
    static string getCourseRawIdByKey(const string& courseKey);

    // Helper method to build course info map using course_key
    static void buildCourseInfoMap(const vector<Course>& courses);

    // Helper method that calculates metadata fields for a given schedule
    static void calculateScheduleMetrics(InformativeSchedule& schedule);
};

#endif // SCHEDULE_BUILDER_H