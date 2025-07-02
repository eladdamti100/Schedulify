// validate_courses.cpp

#include "validate_courses.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>

OptimizedSlot::OptimizedSlot(const string& start, const string& end, const string& id)
        : start_time(start), end_time(end), course_id(id) {
    start_minutes = toMinutes(start);
    end_minutes = toMinutes(end);
}

bool OptimizedSlot::overlapsWith(const OptimizedSlot& other) const {
    return (start_minutes < other.end_minutes && other.start_minutes < end_minutes);
}

// Main validation function - now validates per semester
vector<string> validate_courses(vector<Course> courses) {
    vector<string> errors;
    errors.reserve(50);

    // Group courses by semester for validation
    unordered_map<int, vector<Course>> coursesBySemester;

    // Track unique course IDs to detect duplicates within same validation set
    unordered_map<string, Course> uniqueCourses;

    for (const auto& course : courses) {
        string uniqueId = course.getUniqueId();

        // Check for duplicates in the input set
        if (uniqueCourses.find(uniqueId) != uniqueCourses.end()) {
            errors.push_back("Duplicate course found: " + course.name +
                             " (ID: " + uniqueId + ")");
            continue;
        }

        uniqueCourses[uniqueId] = course;

        // Add course to appropriate semester(s)
        if (course.semester == 1) {
            coursesBySemester[1].push_back(course);
        } else if (course.semester == 2) {
            coursesBySemester[2].push_back(course);
        } else if (course.semester == 3) {
            coursesBySemester[3].push_back(course);
        } else if (course.semester == 4) {
            // Yearly courses go to both semesters A and B
            coursesBySemester[1].push_back(course);
            coursesBySemester[2].push_back(course);
        }
    }

    // Validate each semester separately
    for (const auto& semesterPair : coursesBySemester) {
        int semester = semesterPair.first;
        const vector<Course>& semesterCourses = semesterPair.second;

        string semesterName;
        switch (semester) {
            case 1: semesterName = "Semester A"; break;
            case 2: semesterName = "Semester B"; break;
            case 3: semesterName = "Summer"; break;
            default: semesterName = "Unknown"; break;
        }

        Logger::get().logInfo("Validating " + semesterName + " with " +
                              to_string(semesterCourses.size()) + " courses");

        vector<string> semesterErrors = validateSemesterCourses(semesterCourses, semesterName);
        errors.insert(errors.end(), semesterErrors.begin(), semesterErrors.end());
    }

    Logger::get().logInfo("Validation completed. Total conflicts found: " + to_string(errors.size()));
    return errors;
}

// Validate courses within a single semester
vector<string> validateSemesterCourses(const vector<Course>& courses, const string& semesterName) {
    BuildingSchedule schedule;
    vector<string> errors;

    size_t processed = 0;
    for (const auto& course : courses) {
        processSessionGroups(course.Lectures, course, schedule, errors, semesterName);
        processSessionGroups(course.labs, course, schedule, errors, semesterName);
        processSessionGroups(course.Tirgulim, course, schedule, errors, semesterName);

        processed++;
    }

    Logger::get().logInfo(semesterName + " validation completed. Processed " +
                          to_string(processed) + " courses with " +
                          to_string(errors.size()) + " conflicts found");

    return errors;
}

// Add semester context to session group processing
void processSessionGroups(const vector<Group>& groups, const Course& course,
                          BuildingSchedule& schedule, vector<string>& errors, const string& semesterName) {
    string courseUniqueId = course.getUniqueId();
    for (const auto& group : groups) {
        for (const auto& session : group.sessions) {
            processSession(session, courseUniqueId, schedule, errors, semesterName);
        }
    }
}

// Add semester context to session processing
void processSession(const Session& session, const string& courseUniqueId,
                    BuildingSchedule& schedule, vector<string>& errors, const string& semesterName) {
    if (session.building_number.empty() || session.room_number.empty()) {
        errors.push_back("[" + semesterName + "] Course " + courseUniqueId + " has invalid room information");
        return;
    }

    if (session.start_time.empty() || session.end_time.empty()) {
        errors.push_back("[" + semesterName + "] Course " + courseUniqueId + " has invalid time information");
        return;
    }

    RoomKey roomKey = createRoomKey(session.building_number, session.room_number);
    RoomSchedule& roomSchedule = schedule[roomKey];
    DaySlots& daySlots = roomSchedule[session.day_of_week];

    OptimizedSlot newSlot(session.start_time, session.end_time, courseUniqueId);

    if (newSlot.start_minutes == -1 || newSlot.end_minutes == -1) {
        errors.push_back("[" + semesterName + "] Course " + courseUniqueId + " has invalid time format");
        return;
    }

    if (newSlot.start_minutes >= newSlot.end_minutes) {
        errors.push_back("[" + semesterName + "] Course " + courseUniqueId + " has start time after end time");
        return;
    }

    // Check for conflicts with other courses in the same semester
    for (const auto& existingSlot : daySlots) {
        if (newSlot.overlapsWith(existingSlot)) {
            stringstream errorMsg;
            errorMsg << "[" << semesterName << "] Course " << courseUniqueId << " overlaps with "
                     << existingSlot.course_id << " in " << session.building_number
                     << "-" << session.room_number << " on day " << session.day_of_week
                     << " (" << session.start_time << "-" << session.end_time << ")";
            errors.push_back(errorMsg.str());
            return;
        }
    }

    // Add the slot to the schedule
    daySlots.push_back(newSlot);
}

// Existing helper functions remain the same
string createRoomKey(const string& building, const string& room) {
    return building + "-" + room;
}

int toMinutes(const string& timeStr) {
    if (timeStr.length() < 5) return -1;

    size_t colonPos = timeStr.find(':');
    if (colonPos == string::npos) return -1;

    try {
        int hours = stoi(timeStr.substr(0, colonPos));
        int minutes = stoi(timeStr.substr(colonPos + 1));
        return hours * 60 + minutes;
    } catch (const exception&) {
        return -1;
    }
}

bool isOverLapping(const OptimizedSlot& s1, const OptimizedSlot& s2) {
    return s1.overlapsWith(s2);
}