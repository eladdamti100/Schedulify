#ifndef MODEL_INTERFACES_H
#define MODEL_INTERFACES_H

#include <string>
#include <utility>
#include <vector>

using namespace std;


// Course structs

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
    int semester; // 1: semester A, 2: semester B, 3: summer, 4: yearly
    string raw_id;
    string name;
    string teacher;
    vector<Group> Lectures;
    vector<Group> DepartmentalSessions;
    vector<Group> Reinforcements;
    vector<Group> Guidance;
    vector<Group> OptionalColloquium;
    vector<Group> Registration;
    vector<Group> Thesis;
    vector<Group> Project;
    vector<Group> Tirgulim;
    vector<Group> labs;
    vector<Group> blocks;

    string getUniqueId() const {
        return raw_id + "_sem" + std::to_string(semester);
    }

    bool conflictsWith(const Course& other) const {
        return getUniqueId() == other.getUniqueId();
    }

    string getDisplayName() const {
        string semesterName;
        switch(semester) {
            case 1: semesterName = "Semester A"; break;
            case 2: semesterName = "Semester B"; break;
            case 3: semesterName = "Summer"; break;
            case 4: semesterName = "Yearly"; break;
            default: semesterName = "Unknown"; break;
        }
        return name + " (" + semesterName + ")";
    }
};


// Schedule structs

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
    string unique_id;
    string semester = "A";

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

    vector<ScheduleDay> week;
};

struct FileLoadData {
    vector<int> fileIds;
    string operation_type;
    string filePath;
};


// Lightweight schedule metrics for in-memory bot filtering (no DB dependency)
struct ScheduleFilterMetrics {
    string unique_id;
    string semester;
    int amount_days = 0;
    int amount_gaps = 0;
    int gaps_time = 0;
    int avg_start = 0;
    int avg_end = 0;
    int earliest_start = 0;
    int latest_end = 0;
    int longest_gap = 0;
    int total_class_time = 0;
    int consecutive_days = 0;
    bool weekend_classes = false;
    bool has_morning_classes = false;
    bool has_early_morning = false;
    bool has_evening_classes = false;
    bool has_late_evening = false;
    int max_daily_hours = 0;
    int min_daily_hours = 0;
    int avg_daily_hours = 0;
    bool has_lunch_break = false;
    int max_daily_gaps = 0;
    int avg_gap_length = 0;
    int schedule_span = 0;
    double compactness_ratio = 0.0;
    bool weekday_only = false;
    bool has_monday = false;
    bool has_tuesday = false;
    bool has_wednesday = false;
    bool has_thursday = false;
    bool has_friday = false;
    bool has_saturday = false;
    bool has_sunday = false;
};

// Bot structs

struct BotQueryRequest {
    string userMessage;
    string scheduleMetadata;
    vector<int> availableScheduleIds;
    vector<string> availableUniqueIds;
    string semester;
    vector<ScheduleFilterMetrics> viewScheduleMetrics;

    BotQueryRequest() = default;
    BotQueryRequest(string message, string metadata, string semester,const vector<int>& ids)
            : userMessage(std::move(message)), scheduleMetadata(std::move(metadata)), semester(std::move(semester)), availableScheduleIds(ids) {}

    BotQueryRequest(string message, string metadata, string semester,
                    const vector<string>& uniqueIds, const vector<int>& indices)
            : userMessage(std::move(message)), scheduleMetadata(std::move(metadata)),
              semester(std::move(semester)), availableUniqueIds(uniqueIds), availableScheduleIds(indices) {}
};

struct BotQueryResponse {
    string userMessage;
    string sqlQuery;
    vector<string> queryParameters;
    bool isFilterQuery;
    bool hasError;
    string errorMessage;
    vector<int> filteredScheduleIds;
    vector<string> filteredUniqueIds;

    BotQueryResponse() : isFilterQuery(false), hasError(false) {}
    BotQueryResponse(string message, string query, const vector<string>& params, bool isFilter)
            : userMessage(std::move(message)), sqlQuery(std::move(query)), queryParameters(params), isFilterQuery(isFilter), hasError(false) {}

    BotQueryResponse(string message, string query, const vector<string>& params,
                     bool isFilter, const vector<string>& uniqueIds)
            : userMessage(std::move(message)), sqlQuery(std::move(query)),
              queryParameters(params), isFilterQuery(isFilter), hasError(false),
              filteredUniqueIds(uniqueIds) {}
};

struct UniqueIdConversionRequest {
    vector<string> uniqueIds;
    string semester;

    UniqueIdConversionRequest() = default;
    UniqueIdConversionRequest(const vector<string>& ids, const string& sem)
            : uniqueIds(ids), semester(sem) {}
};

struct IndexConversionRequest {
    vector<int> indices;
    string semester;

    IndexConversionRequest() = default;
    IndexConversionRequest(const vector<int>& ids, const string& sem)
            : indices(ids), semester(sem) {}
};


// Main model menu

enum class ModelOperation {
    GENERATE_COURSES,
    VALIDATE_COURSES,
    GENERATE_SCHEDULES,
    SAVE_SCHEDULE,
    PRINT_SCHEDULE,
    BOT_QUERY_SCHEDULES,
    GET_LAST_FILTERED_IDS,
    GET_LAST_FILTERED_UNIQUE_IDS,
    LOAD_FROM_HISTORY,
    GET_FILE_HISTORY,
    DELETE_FILE_FROM_HISTORY,
    CLEAN_SCHEDULES,
    CONVERT_UNIQUE_IDS_TO_INDICES,
    CONVERT_INDICES_TO_UNIQUE_IDS
};

class IModel {
public:
    virtual ~IModel() = default;
    virtual void* executeOperation(ModelOperation operation, const void* data, const string& path) = 0;
};

#endif //MODEL_INTERFACES_H