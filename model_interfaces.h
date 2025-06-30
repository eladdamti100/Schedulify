#ifndef MODEL_INTERFACES_H
#define MODEL_INTERFACES_H

#include <string>
#include <utility>
#include <vector>

using namespace std;

enum class SessionType {
    LECTURE,
    TUTORIAL,
    LAB,
    BLOCK,
    DEPARTMENTAL_SESSION,
    REINFORCEMENT,
    GUIDANCE,
    OPTIONAL_COLLOQUIUM,
    REGISTRATION,
    THESIS,
    PROJECT,
    UNSUPPORTED

};

class Session {
public:
    int day_of_week;
    string start_time;
    string end_time;
    string building_number;
    string room_number;
};

class Group {
public:
    SessionType type;
    vector<Session> sessions;
};

class Course {
public:
    int id;
    int semester; // 1: semester A 2: semster B 3: Semester summer 4: All semesters 
    string raw_id;
    string name;
    string teacher;
    vector<Group> Lectures;
    vector<Group> DepartmentalSessions; // ש.מחלקה
    vector<Group> Reinforcements;       // תגבור
    vector<Group> Guidance;             // הדרכה
    vector<Group> OptionalColloquium;   // קולוקויום רשות
    vector<Group> Registration;         // רישום
    vector<Group> Thesis;
    vector<Group> Project;
    vector<Group> Tirgulim;
    vector<Group> labs;
    vector<Group> blocks;
};

struct ScheduleItem {
    string courseName;
    string raw_id;
    string type;
    string start;
    string end;
    string building;
    string room;
};

struct ScheduleDay {
    string day;
    vector<ScheduleItem> day_items;
};

struct InformativeSchedule {
    int index;

    int amount_days = 0;
    int amount_gaps = 0;
    int gaps_time = 0;
    int avg_start = 0;
    int avg_end = 0;

    int earliest_start = 0;        // Earliest class start time (minutes from midnight)
    int latest_end = 0;            // Latest class end time (minutes from midnight)
    int longest_gap = 0;           // Longest single gap between classes (minutes)
    int total_class_time = 0;      // Total minutes of actual classes

    int consecutive_days = 0;      // Longest streak of consecutive class days
    string days_json = "[]"; // Array of days with classes [1,2,3,4,5]
    bool weekend_classes = false; // Has classes on Sat/Sun

    bool has_morning_classes = false;   // Classes before 10:00 AM (600 minutes)
    bool has_early_morning = false;     // Classes before 8:30 AM (510 minutes)
    bool has_evening_classes = false;   // Classes after 6:00 PM (1080 minutes)
    bool has_late_evening = false;      // Classes after 8:00 PM (1200 minutes)

    int max_daily_hours = 0;       // Most hours of classes in a single day
    int min_daily_hours = 0;       // Fewest hours on days with classes
    int avg_daily_hours = 0;       // Average hours per study day

    bool has_lunch_break = false;  // Has gap between 12:00-14:00 (720-840 minutes)
    int max_daily_gaps = 0;        // Maximum gaps in a single day
    int avg_gap_length = 0;        // Average gap length when gaps exist

    int schedule_span = 0;         // Time from first to last class (latest_end - earliest_start)
    double compactness_ratio = 0.0; // total_class_time / schedule_span (efficiency measure)

    bool weekday_only = false;     // Only Monday-Friday classes
    bool has_monday = false;
    bool has_tuesday = false;
    bool has_wednesday = false;
    bool has_thursday = false;
    bool has_friday = false;
    bool has_saturday = false;
    bool has_sunday = false;

    vector<ScheduleDay> week;
};

struct FileLoadData {
    vector<int> fileIds;
    string operation_type;
    string filePath;
};

struct BotQueryRequest {
    string userMessage;
    string scheduleMetadata;
    vector<int> availableScheduleIds;

    BotQueryRequest() = default;
    BotQueryRequest(string message, string metadata, const vector<int>& ids)
            : userMessage(std::move(message)), scheduleMetadata(std::move(metadata)), availableScheduleIds(ids) {}
};

struct BotQueryResponse {
    string userMessage;
    string sqlQuery;
    vector<string> queryParameters;
    bool isFilterQuery;
    bool hasError;
    string errorMessage;

    BotQueryResponse() : isFilterQuery(false), hasError(false) {}
    BotQueryResponse(string message, string query, const vector<string>& params, bool isFilter)
            : userMessage(std::move(message)), sqlQuery(std::move(query)), queryParameters(params), isFilterQuery(isFilter), hasError(false) {}
};

enum class ModelOperation {
    GENERATE_COURSES,
    VALIDATE_COURSES,
    GENERATE_SCHEDULES,
    SAVE_SCHEDULE,
    PRINT_SCHEDULE,
    BOT_QUERY_SCHEDULES,
    GET_LAST_FILTERED_IDS,
    LOAD_FROM_HISTORY,
    GET_FILE_HISTORY,
    DELETE_FILE_FROM_HISTORY,
    CLEAN_SCHEDULES
};

class IModel {
public:
    virtual ~IModel() = default;
    virtual void* executeOperation(ModelOperation operation, const void* data, const string& path) = 0;
};

#endif //MODEL_INTERFACES_H