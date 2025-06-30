#ifndef INNER_STRUCTS_H
#define INNER_STRUCTS_H

#include "model_interfaces.h"

struct CourseSelection {
    int courseId;
    const Group* lectureGroup;
    const Group* tutorialGroup;       // nullptr if none
    const Group* labGroup;            // nullptr if none
    const Group* blockGroup;          // nullptr if none
    const Group* departmentalGroup;   // nullptr if none
    const Group* reinforcementGroup;  // nullptr if none
    const Group* guidanceGroup;       // nullptr if none
    const Group* colloquiumGroup;     // nullptr if none
    const Group* registrationGroup;   // nullptr if none
    const Group* thesisGroup;         // nullptr if none
    const Group* projectGroup;        // nullptr if none
};

struct CourseInfo {
    string raw_id;
    string name;
};

#endif //INNER_STRUCTS_H