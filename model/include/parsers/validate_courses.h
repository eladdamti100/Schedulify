#ifndef VALIDATE_COURSES_H
#define VALIDATE_COURSES_H

#include "model_interfaces.h"
#include "logger.h"

#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

struct OptimizedSlot {
    string start_time;
    string end_time;
    string course_id;
    int start_minutes;
    int end_minutes;

    OptimizedSlot(const string& start, const string& end, const string& id);
    bool overlapsWith(const OptimizedSlot& other) const;
};

using RoomKey = string;
using DaySlots = vector<OptimizedSlot>;
using RoomSchedule = unordered_map<int, DaySlots>;
using BuildingSchedule = unordered_map<RoomKey, RoomSchedule>;

// Main validation function
vector<string> validate_courses(vector<Course> courses);

// Validate courses within a single semester
vector<string> validateSemesterCourses(const vector<Course>& courses, const string& semesterName);

// Add semester context to function signatures
void processSessionGroups(const vector<Group>& groups, const Course& course,
                          BuildingSchedule& schedule, vector<string>& errors, const string& semesterName);

void processSession(const Session& session, const string& courseUniqueId,
                    BuildingSchedule& schedule, vector<string>& errors, const string& semesterName);

// Existing helper functions
string createRoomKey(const string& building, const string& room);
int toMinutes(const string& timeStr);
bool isOverLapping(const OptimizedSlot& s1, const OptimizedSlot& s2);

#endif //VALIDATE_COURSES_H