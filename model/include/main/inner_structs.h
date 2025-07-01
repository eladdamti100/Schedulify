#ifndef INNER_STRUCTS_H
#define INNER_STRUCTS_H

#include "model_interfaces.h"

struct CourseSelection {
    int courseId;
    int courseSemester = 1;
    string courseUniqid;
    string courseKey;

    const Group* lectureGroup;
    const Group* tutorialGroup;
    const Group* labGroup;
    const Group* blockGroup;
    const Group* departmentalGroup;
    const Group* reinforcementGroup;
    const Group* guidanceGroup;
    const Group* colloquiumGroup;
    const Group* registrationGroup;
    const Group* thesisGroup;
    const Group* projectGroup;

    // Default constructor
    CourseSelection() = default;

    // Constructor that sets all course-related fields
    CourseSelection(int id, int semester, const std::string& uniqid, const std::string& key)
            : courseId(id), courseSemester(semester), courseUniqid(uniqid), courseKey(key) {}

    // Constructor from Course object
    explicit CourseSelection(const Course& course)
            : courseId(course.id), courseSemester(course.semester),
              courseUniqid(course.uniqid), courseKey(course.course_key) {}

    // Helper methods
    std::string getDisplayName() const {
        return "Course " + std::to_string(courseId) + " (Semester " + std::to_string(courseSemester) + ")";
    }

    // Validate that all course fields are consistent
    bool isValid() const {
        // Check that courseKey matches expected format: {courseId}_s{semester}
        std::string expectedKey = std::to_string(courseId) + "_s" + std::to_string(courseSemester);
        return courseKey == expectedKey;
    }
};

struct CourseInfo {
    string raw_id;
    string name;
};

#endif //INNER_STRUCTS_H