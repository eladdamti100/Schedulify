#ifndef DB_ENTITIES_H
#define DB_ENTITIES_H

#include "db_json_helpers.h"

#include <QDateTime>
#include <string>
#include <utility>
#include <vector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QByteArray>

using namespace std;

// Database entity structures representing table schemas
struct FileEntity {
    int id = 0;
    string file_name;
    string file_type;
    QDateTime upload_time;
    QDateTime updated_at;

    // Default constructor
    FileEntity() = default;

    // Constructor for easy creation
    FileEntity(string name, string type) : file_name(std::move(name)), file_type(std::move(type)),
              upload_time(QDateTime::currentDateTime()), updated_at(QDateTime::currentDateTime()) {}
};

struct CourseEntity {
    int id = 0;                   // Course ID (matches Course.id)
    string raw_id;                // Original course identifier from file
    string name;                  // Course name
    string teacher;               // Teacher/instructor name
    int semester = 1;             // Semester number (1=A, 2=B, 3=Summer, 4=Yearly)
    string lectures_json;         // JSON serialized lectures groups
    string tutorials_json;        // JSON serialized tutorials groups
    string labs_json;             // JSON serialized labs groups
    string blocks_json;           // JSON serialized blocks groups
    int file_id = 0;             // Foreign key to file table - REQUIRED
    QDateTime created_at;         // When course was added to database
    QDateTime updated_at;         // Last modification time

    // Default constructor
    CourseEntity() = default;

    // Constructor for database insertion
    CourseEntity(int courseId, string rawId, string courseName, string teacherName, int semesterNum, int fileId) :
            id(courseId), raw_id(std::move(rawId)), name(std::move(courseName)), teacher(std::move(teacherName)),
            semester(semesterNum), file_id(fileId), created_at(QDateTime::currentDateTime()),
            updated_at(QDateTime::currentDateTime()) {}
};

struct MetadataEntity {
    int id = 0;
    string key;
    string value;
    string description;
    QDateTime updated_at;

    // Default constructor
    MetadataEntity() = default;

    // Constructor for database insertion
    MetadataEntity(string metaKey, string metaValue,string desc = "") : key(std::move(metaKey)),
    value(std::move(metaValue)), description(std::move(desc)), updated_at(QDateTime::currentDateTime()) {}
};

struct DatabaseStatistics {
    int total_files = 0;
    int total_courses = 0;
    int total_metadata_entries = 0;
    string database_version;
    QDateTime last_accessed;
    QString database_size;

    // Default constructor
    DatabaseStatistics() = default;
};

struct CourseConflictInfo {
    int course_id = 0;
    string raw_id;
    string course_name;
    int semester = 1;
    int file_id = 0;
    string file_name;
    QDateTime upload_time;

    // Default constructor
    CourseConflictInfo() = default;

    // Constructor for conflict tracking
    CourseConflictInfo(int courseId, string rawId, string name, int semesterNum, int fileId, string fileName, QDateTime uploadTime) :
            course_id(courseId), raw_id(std::move(rawId)), course_name(std::move(name)), semester(semesterNum), file_id(fileId),
            file_name(std::move(fileName)), upload_time(std::move(uploadTime)) {}
};

struct ScheduleEntity {
    int id = 0;
    int schedule_set_id = 0;
    int schedule_index = 0;
    string schedule_name;
    string schedule_data_json;
    int amount_days = 0;
    int amount_gaps = 0;
    int gaps_time = 0;
    int avg_start = 0;
    int avg_end = 0;
    QDateTime created_at;
    QDateTime updated_at;

    // Default constructor
    ScheduleEntity() = default;

    // Constructor for database insertion
    ScheduleEntity(int setId, const InformativeSchedule& schedule, string  name = "")
            : schedule_set_id(setId), schedule_index(schedule.index), schedule_name(std::move(name)),
              amount_days(schedule.amount_days), amount_gaps(schedule.amount_gaps),
              gaps_time(schedule.gaps_time), avg_start(schedule.avg_start),
              avg_end(schedule.avg_end), created_at(QDateTime::currentDateTime()),
              updated_at(QDateTime::currentDateTime()) {
        schedule_data_json = DatabaseJsonHelpers::scheduleToJson(schedule);
    }
};

#endif // DB_ENTITIES_H