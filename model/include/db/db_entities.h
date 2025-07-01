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
    // Ids
    string uniqid;                // PRIMARY KEY: Unique identifier (courseId_f{fileId}_s{semester})
    int course_id = 0;            // Course ID (Course.id)
    string raw_id;                // Course string ID (Course.raw_id)
    string course_key;            // Course key for conflict resolution (courseId_s{semester})
    int file_id = 0;             // Foreign key to file table

    // Information fields
    string name;
    int semester = 1;
    string teacher;

    // Groups as stringed jsons
    string lectures_json;
    string tutorials_json;
    string labs_json;
    string blocks_json;
    string departmental_sessions_json;
    string reinforcements_json;
    string guidance_json;
    string optional_colloquium_json;
    string registration_json;
    string thesis_json;
    string project_json;

    // Metadata
    QDateTime created_at;
    QDateTime updated_at;

    // Default constructor
    CourseEntity() = default;

    // Constructor for database insertion (basic)
    CourseEntity(int courseId, string rawId, string courseName, string teacherName, int semester, int fileId)
            : course_id(courseId), raw_id(std::move(rawId)), name(std::move(courseName)),
              teacher(std::move(teacherName)), semester(semester), file_id(fileId),
              created_at(QDateTime::currentDateTime()), updated_at(QDateTime::currentDateTime()) {

        // Generate uniqid and course_key
        uniqid = std::to_string(courseId) + "_f" + std::to_string(fileId) + "_s" + std::to_string(semester);
        course_key = std::to_string(courseId) + "_s" + std::to_string(semester);
    }

    // Constructor from Course object
    CourseEntity(const Course& course, int fileId)
            : course_id(course.id), raw_id(course.raw_id), name(course.name),
              teacher(course.teacher), semester(course.semester), uniqid(course.uniqid),
              course_key(course.course_key), file_id(fileId),
              created_at(QDateTime::currentDateTime()), updated_at(QDateTime::currentDateTime()) {}

    // Helper method to get semester name
    string getSemesterName() const {
        switch (semester) {
            case 1: return "Semester A";
            case 2: return "Semester B";
            case 3: return "Summer";
            case 4: return "Year-long";
            default: return "Unknown Semester";
        }
    }

    // Helper method to validate semester
    bool hasValidSemester() const {
        return semester >= 1 && semester <= 4;
    }
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
    MetadataEntity(string metaKey, string metaValue, string desc = "") : key(std::move(metaKey)),
                                                                         value(std::move(metaValue)),
                                                                         description(std::move(desc)),
                                                                         updated_at(QDateTime::currentDateTime()) {}
};

struct ScheduleEntity {
    int id = 0;
    int schedule_index = 0;
    int semester = 1;
    string schedule_name;
    string schedule_data_json;

    // Basic metrics
    int amount_days = 0;
    int amount_gaps = 0;
    int gaps_time = 0;
    int avg_start = 0;
    int avg_end = 0;

    // Enhanced time metrics
    int earliest_start = 0;
    int latest_end = 0;
    int longest_gap = 0;
    int total_class_time = 0;

    // Day pattern metrics
    int consecutive_days = 0;
    string days_json = "[]";
    bool weekend_classes = false;

    // Time preference flags
    bool has_morning_classes = false;
    bool has_early_morning = false;
    bool has_evening_classes = false;
    bool has_late_evening = false;

    // Daily intensity metrics
    int max_daily_hours = 0;
    int min_daily_hours = 0;
    int avg_daily_hours = 0;

    // Gap and break patterns
    bool has_lunch_break = false;
    int max_daily_gaps = 0;
    int avg_gap_length = 0;

    // Efficiency metrics
    int schedule_span = 0;
    double compactness_ratio = 0.0;

    // Additional boolean flags
    bool weekday_only = false;
    bool has_monday = false;
    bool has_tuesday = false;
    bool has_wednesday = false;
    bool has_thursday = false;
    bool has_friday = false;
    bool has_saturday = false;
    bool has_sunday = false;

    QDateTime created_at;
    QDateTime updated_at;

    // Default constructor
    ScheduleEntity() = default;

    // Constructor for database insertion from InformativeSchedule
    ScheduleEntity(const InformativeSchedule& schedule, string name = "")
            : schedule_index(schedule.index), semester(schedule.semester),
              schedule_name(std::move(name)),
            // Basic metrics
              amount_days(schedule.amount_days), amount_gaps(schedule.amount_gaps),
              gaps_time(schedule.gaps_time), avg_start(schedule.avg_start), avg_end(schedule.avg_end),
            // Enhanced time metrics
              earliest_start(schedule.earliest_start), latest_end(schedule.latest_end),
              longest_gap(schedule.longest_gap), total_class_time(schedule.total_class_time),
            // Day pattern metrics
              consecutive_days(schedule.consecutive_days), days_json(schedule.days_json),
              weekend_classes(schedule.weekend_classes),
            // Time preference flags
              has_morning_classes(schedule.has_morning_classes), has_early_morning(schedule.has_early_morning),
              has_evening_classes(schedule.has_evening_classes), has_late_evening(schedule.has_late_evening),
            // Daily intensity metrics
              max_daily_hours(schedule.max_daily_hours), min_daily_hours(schedule.min_daily_hours),
              avg_daily_hours(schedule.avg_daily_hours),
            // Gap and break patterns
              has_lunch_break(schedule.has_lunch_break), max_daily_gaps(schedule.max_daily_gaps),
              avg_gap_length(schedule.avg_gap_length),
            // Efficiency metrics
              schedule_span(schedule.schedule_span), compactness_ratio(schedule.compactness_ratio),
            // Additional boolean flags
              weekday_only(schedule.weekday_only), has_monday(schedule.has_monday),
              has_tuesday(schedule.has_tuesday), has_wednesday(schedule.has_wednesday),
              has_thursday(schedule.has_thursday), has_friday(schedule.has_friday),
              has_saturday(schedule.has_saturday), has_sunday(schedule.has_sunday),
            // Timestamps
              created_at(QDateTime::currentDateTime()), updated_at(QDateTime::currentDateTime()) {
        schedule_data_json = DatabaseJsonHelpers::scheduleToJson(schedule);
    }

    // Helper method to get semester name
    string getSemesterName() const {
        switch (semester) {
            case 1: return "Semester A";
            case 2: return "Semester B";
            case 3: return "Summer";
            case 4: return "Year-long";
            default: return "Unknown Semester";
        }
    }

    // Helper method to validate semester
    bool hasValidSemester() const {
        return semester >= 1 && semester <= 4;
    }
};

#endif // DB_ENTITIES_H