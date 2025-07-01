#ifndef DB_COURSES_H
#define DB_COURSES_H

#include "db_entities.h"
#include "model_interfaces.h"
#include "db_json_helpers.h"
#include "logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <algorithm>
#include <QSqlDatabase>
#include <vector>
#include <string>
#include <map>

using namespace std;

class DatabaseCourseManager {
public:
    explicit DatabaseCourseManager(QSqlDatabase& database);

    // Course CRUD operations
    bool insertCourse(const Course& course, int fileId);
    bool insertCourses(const vector<Course>& courses, int fileId);
    bool deleteAllCourses();
    bool deleteCoursesByFileId(int fileId);
    bool deleteCourseByUniqid(const string& uniqid);

    // Course retrieval operations - updated for new structure
    vector<Course> getAllCourses();
    Course getCourseByUniqid(const string& uniqid);
    vector<Course> getCoursesByFileId(int fileId);
    vector<Course> getCoursesByFileIds(const vector<int>& fileIds, vector<string>& warnings);
    vector<Course> getCoursesBySemester(int semester);
    vector<Course> getCoursesByCourseId(int courseId);  // All semesters of a specific course
    vector<Course> getCoursesByCourseKey(const string& courseKey);

    // Course utility operations
    int getCourseCount();
    int getCourseCountByFileId(int fileId);
    int getCourseCountBySemester(int semester);
    bool courseExists(const string& uniqid);
    bool courseKeyExists(const string& courseKey);

    // Conflict resolution operations
    vector<string> detectConflicts(const vector<int>& fileIds);
    map<string, vector<Course>> groupCoursesByKey(const vector<Course>& courses);
    Course selectLatestCourse(const vector<Course>& conflictingCourses);

    // Semester-specific operations
    vector<Course> getCoursesByFileIdAndSemester(int fileId, int semester);
    vector<string> getDistinctCourseKeys();
    vector<int> getDistinctSemesters();

private:
    QSqlDatabase& db;

    // Helper methods for creating Course objects from query results
    static Course createCourseFromQuery(QSqlQuery& query);

    // Helper methods for conflict resolution
    struct CourseConflictInfo {
        Course course;
        QDateTime uploadTime;
        string fileName;
        int fileId;

        CourseConflictInfo(Course c, QDateTime time, string name, int id)
                : course(std::move(c)), uploadTime(time), fileName(std::move(name)), fileId(id) {}
    };

    // Conflict resolution helpers
    map<string, vector<CourseConflictInfo>> buildConflictMap(const vector<int>& fileIds);
    vector<Course> resolveConflictsByLatest(const map<string, vector<CourseConflictInfo>>& conflictMap,
                                            vector<string>& warnings);

    // Query builders for different selection criteria
    QString buildCourseSelectQuery(const string& whereClause = "") const;
    QString buildCourseCountQuery(const string& whereClause = "") const;

    // JSON serialization helpers
    bool serializeGroupsToEntity(const Course& course, CourseEntity& entity);
    bool deserializeGroupsFromEntity(const CourseEntity& entity, Course& course);
};

#endif // DB_COURSES_H