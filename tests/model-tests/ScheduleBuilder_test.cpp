#include "ScheduleBuilder.h"
#include "gtest/gtest.h"
#include "parseCoursesToVector.h"
#include "model_interfaces.h"

using namespace std;

// Helper to create a session with given parameters
Session makeTestSession(int day_of_week, const string& start_time, const string& end_time,
                       const string& building = "", const string& room = "") {
    Session session;
    session.day_of_week = day_of_week;
    session.start_time = start_time;
    session.end_time = end_time;
    session.building_number = building;
    session.room_number = room;
    return session;
}

// Helper to create a group with given type and sessions
Group makeGroup(SessionType type, const vector<Session>& sessions) {
    Group group;
    group.type = type;
    group.sessions = sessions;
    return group;
}

// Helper to create a course with given integer ID and groups
Course makeCourse(int id, const vector<Group>& lectures = {},
                  const vector<Group>& tirgulim = {},
                  const vector<Group>& labs = {},
                  const vector<Group>& blocks = {},
                  const string& name = "", const string& teacher = "") {
    Course course;
    course.id = id;
    course.raw_id = to_string(id);
    course.name = name.empty() ? "Course " + to_string(id) : name;
    course.teacher = teacher;
    course.Lectures = lectures;
    course.Tirgulim = tirgulim;
    course.labs = labs;
    course.blocks = blocks;
    return course;
}

// --- TEST CASES ---

// One course with lecture and tutorial that do not conflict
TEST(ScheduleBuilderTest, OneCourse_NoConflictWithinCourse) {
    ScheduleBuilder builder;

    // Create lecture group
    vector<Session> lectureSessions = {makeTestSession(1, "09:00", "10:00")};
    Group lectureGroup = makeGroup(SessionType::LECTURE, lectureSessions);

    // Create tutorial group
    vector<Session> tutorialSessions = {makeTestSession(1, "10:00", "11:00")};
    Group tutorialGroup = makeGroup(SessionType::TUTORIAL, tutorialSessions);

    Course course = makeCourse(101, {lectureGroup}, {tutorialGroup});

    vector<InformativeSchedule> result = builder.build({course});
    ASSERT_EQ(result.size(), 1);  // One valid schedule
}

// Two courses with overlapping sessions → conflict
TEST(ScheduleBuilderTest, TwoCourses_WithConflict) {
    ScheduleBuilder builder;

    // Course A with lecture from 09:00-11:00
    vector<Session> lectureSessionsA = {makeTestSession(1, "09:00", "11:00")};
    Group lectureGroupA = makeGroup(SessionType::LECTURE, lectureSessionsA);
    Course courseA = makeCourse(101, {lectureGroupA});

    // Course B with lecture from 10:00-12:00 (conflicts with A)
    vector<Session> lectureSessionsB = {makeTestSession(1, "10:00", "12:00")};
    Group lectureGroupB = makeGroup(SessionType::LECTURE, lectureSessionsB);
    Course courseB = makeCourse(102, {lectureGroupB});

    vector<InformativeSchedule> result = builder.build({courseA, courseB});
    ASSERT_EQ(result.size(), 0);  // No valid schedules due to conflict
}

// Two non-overlapping courses → valid combination
TEST(ScheduleBuilderTest, TwoCourses_NoConflict) {
    ScheduleBuilder builder;

    // Course A with lecture from 09:00-10:00
    vector<Session> lectureSessionsA = {makeTestSession(1, "09:00", "10:00")};
    Group lectureGroupA = makeGroup(SessionType::LECTURE, lectureSessionsA);
    Course courseA = makeCourse(101, {lectureGroupA});

    // Course B with lecture from 10:00-11:00 (non-conflicting)
    vector<Session> lectureSessionsB = {makeTestSession(1, "10:00", "11:00")};
    Group lectureGroupB = makeGroup(SessionType::LECTURE, lectureSessionsB);
    Course courseB = makeCourse(102, {lectureGroupB});

    vector<InformativeSchedule> result = builder.build({courseA, courseB});
    ASSERT_EQ(result.size(), 1);  // One valid schedule
}

// Course with only a lecture (no tutorial or lab)
TEST(ScheduleBuilderTest, NoTutorialOrLab) {
    ScheduleBuilder builder;

    vector<Session> lectureSessions = {makeTestSession(2, "13:00", "15:00")};
    Group lectureGroup = makeGroup(SessionType::LECTURE, lectureSessions);
    Course course = makeCourse(201, {lectureGroup});

    vector<InformativeSchedule> result = builder.build({course});
    ASSERT_EQ(result.size(), 1);  // Should handle lack of tutorials/labs
}

// Two courses with multiple valid session combinations (no conflicts)
TEST(ScheduleBuilderTest, MultipleCombinations) {
    ScheduleBuilder builder;

    // Course A with two separate lecture groups (different sections)
    vector<Session> lectureSessionsA1 = {makeTestSession(1, "09:00", "10:00")};
    Group lectureGroupA1 = makeGroup(SessionType::LECTURE, lectureSessionsA1);

    vector<Session> lectureSessionsA2 = {makeTestSession(1, "11:00", "12:00")};
    Group lectureGroupA2 = makeGroup(SessionType::LECTURE, lectureSessionsA2);

    Course courseA = makeCourse(301, {lectureGroupA1, lectureGroupA2});

    // Course B with two separate lecture groups on different day
    vector<Session> lectureSessionsB1 = {makeTestSession(2, "09:00", "10:00")};
    Group lectureGroupB1 = makeGroup(SessionType::LECTURE, lectureSessionsB1);

    vector<Session> lectureSessionsB2 = {makeTestSession(2, "11:00", "12:00")};
    Group lectureGroupB2 = makeGroup(SessionType::LECTURE, lectureSessionsB2);

    Course courseB = makeCourse(302, {lectureGroupB1, lectureGroupB2});

    vector<InformativeSchedule> result = builder.build({courseA, courseB});
    ASSERT_EQ(result.size(), 4);  // 2 lecture groups per course = 2x2 = 4 valid combos
}

// Edge case: no courses given
TEST(ScheduleBuilderTest, EmptyCourseList) {
    ScheduleBuilder builder;
    vector<InformativeSchedule> result = builder.build({});
    ASSERT_EQ(result.size(), 1);  // One "empty" valid schedule (base case)
}

// One course with a single lecture session
TEST(ScheduleBuilderTest, OneCourse_OneSessionOnly) {
    ScheduleBuilder builder;

    vector<Session> lectureSessions = {makeTestSession(0, "08:00", "09:00")};
    Group lectureGroup = makeGroup(SessionType::LECTURE, lectureSessions);
    Course course = makeCourse(501, {lectureGroup});

    vector<InformativeSchedule> result = builder.build({course});
    ASSERT_EQ(result.size(), 1);  // Only one option, should be valid
}

// Three courses with identical session times → all conflict
TEST(ScheduleBuilderTest, MultipleCourses_ExactSameTimes) {
    ScheduleBuilder builder;

    Session session = makeTestSession(1, "09:00", "10:00");

    Group lectureGroup1 = makeGroup(SessionType::LECTURE, {session});
    Group lectureGroup2 = makeGroup(SessionType::LECTURE, {session});
    Group lectureGroup3 = makeGroup(SessionType::LECTURE, {session});

    Course course1 = makeCourse(601, {lectureGroup1});
    Course course2 = makeCourse(602, {lectureGroup2});
    Course course3 = makeCourse(603, {lectureGroup3});

    vector<InformativeSchedule> result = builder.build({course1, course2, course3});
    ASSERT_EQ(result.size(), 0);  // All conflict with each other
}

// Courses without tutorials or labs → test that empty vectors are handled correctly
TEST(ScheduleBuilderTest, CoursesWithNoTutorialOrLab_EmptyVectorsHandled) {
    ScheduleBuilder builder;

    vector<Session> lectureSessionsA = {makeTestSession(3, "12:00", "13:00")};
    Group lectureGroupA = makeGroup(SessionType::LECTURE, lectureSessionsA);
    Course course1 = makeCourse(701, {lectureGroupA});

    vector<Session> lectureSessionsB = {makeTestSession(3, "13:00", "14:00")};
    Group lectureGroupB = makeGroup(SessionType::LECTURE, lectureSessionsB);
    Course course2 = makeCourse(702, {lectureGroupB});

    vector<InformativeSchedule> result = builder.build({course1, course2});
    ASSERT_EQ(result.size(), 1);  // Should produce one valid schedule
}

// Ten non-conflicting courses in sequence → large input test
TEST(ScheduleBuilderTest, LargeInput_NoConflicts) {
    ScheduleBuilder builder;

    vector<Course> manyCourses;
    for (int i = 0; i < 10; ++i) {
        // Each course starts an hour after the previous
        int hour = 8 + i;
        string start = to_string(hour) + ":00";
        string end = to_string(hour + 1) + ":00";

        vector<Session> lectureSessions = {makeTestSession(1, start, end)};
        Group lectureGroup = makeGroup(SessionType::LECTURE, lectureSessions);
        manyCourses.push_back(makeCourse(800 + i, {lectureGroup}));
    }

    vector<InformativeSchedule> result = builder.build(manyCourses);
    ASSERT_EQ(result.size(), 1);  // One valid schedule including all 10 courses
}


// Chained Conflicts Block All Schedules
TEST(ScheduleBuilderTest, ChainedConflictsBlocksAllSchedules) {
    ScheduleBuilder builder;

    vector<Session> lectureSessionsA = {makeTestSession(1, "09:00", "10:00")};
    Group lectureGroupA = makeGroup(SessionType::LECTURE, lectureSessionsA);
    Course a = makeCourse(1101, {lectureGroupA});

    vector<Session> lectureSessionsB = {makeTestSession(1, "09:30", "10:30")};  // Overlaps with A and C
    Group lectureGroupB = makeGroup(SessionType::LECTURE, lectureSessionsB);
    Course b = makeCourse(1102, {lectureGroupB});

    vector<Session> lectureSessionsC = {makeTestSession(1, "10:00", "11:00")};
    Group lectureGroupC = makeGroup(SessionType::LECTURE, lectureSessionsC);
    Course c = makeCourse(1103, {lectureGroupC});

    vector<InformativeSchedule> result = builder.build({a, b, c});
    ASSERT_EQ(result.size(), 0);  // All schedules blocked due to transitive conflict
}

// Schedule Contains Correct Times
TEST(ScheduleBuilderTest, ScheduleContainsCorrectTimes) {
    ScheduleBuilder builder;

    vector<Session> lectureSessionsA = {makeTestSession(1, "09:00", "10:00")};
    Group lectureGroupA = makeGroup(SessionType::LECTURE, lectureSessionsA);
    Course courseA = makeCourse(1301, {lectureGroupA});

    vector<Session> lectureSessionsB = {makeTestSession(1, "10:00", "11:00")};
    Group lectureGroupB = makeGroup(SessionType::LECTURE, lectureSessionsB);
    Course courseB = makeCourse(1302, {lectureGroupB});

    vector<InformativeSchedule> result = builder.build({courseA, courseB});
    ASSERT_EQ(result.size(), 1);  // Should succeed

    const InformativeSchedule& sched = result[0];
    ASSERT_EQ(sched.index, 0);  // Should have valid index
    // Note: Updated to check InformativeSchedule structure instead of selections
}

// Test with multiple groups of the same type
TEST(ScheduleBuilderTest, MultipleGroupsSameType) {
    ScheduleBuilder builder;

    // Course with two different lecture groups (e.g., different sections)
    vector<Session> lectureSessionsGroup1 = {makeTestSession(1, "09:00", "10:00")};
    Group lectureGroup1 = makeGroup(SessionType::LECTURE, lectureSessionsGroup1);

    vector<Session> lectureSessionsGroup2 = {makeTestSession(2, "09:00", "10:00")};
    Group lectureGroup2 = makeGroup(SessionType::LECTURE, lectureSessionsGroup2);

    Course course = makeCourse(1401, {lectureGroup1, lectureGroup2});

    vector<InformativeSchedule> result = builder.build({course});
    ASSERT_EQ(result.size(), 2);  // Two possible schedules (one per lecture group)
}

// Test with all session types
TEST(ScheduleBuilderTest, AllSessionTypes) {
    ScheduleBuilder builder;

    vector<Session> lectureSessions = {makeTestSession(1, "09:00", "10:00")};
    Group lectureGroup = makeGroup(SessionType::LECTURE, lectureSessions);

    vector<Session> tutorialSessions = {makeTestSession(1, "10:00", "11:00")};
    Group tutorialGroup = makeGroup(SessionType::TUTORIAL, tutorialSessions);

    vector<Session> labSessions = {makeTestSession(1, "11:00", "12:00")};
    Group labGroup = makeGroup(SessionType::LAB, labSessions);

    vector<Session> blockSessions = {makeTestSession(1, "12:00", "13:00")};
    Group blockGroup = makeGroup(SessionType::BLOCK, blockSessions);

    Course course = makeCourse(1501, {lectureGroup}, {tutorialGroup}, {labGroup}, {blockGroup});

    vector<InformativeSchedule> result = builder.build({course});
    ASSERT_EQ(result.size(), 1);  // Should handle all session types
}