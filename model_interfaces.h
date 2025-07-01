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
    int semester;
    string raw_id;
    string name;
    string teacher;
    string uniqid;
    string course_key;

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

    // Default constructor
    Course() : id(0), semester(1) {}

    // Helper method to generate unique ID when file ID is known
    void generateUniqueId(int fileId) {
        uniqid = std::to_string(id) + "_f" + std::to_string(fileId) + "_s" + std::to_string(semester);
        course_key = std::to_string(id) + "_s" + std::to_string(semester);
    }

    // Helper method to generate course key (independent of file)
    void generateCourseKey() {
        course_key = std::to_string(id) + "_s" + std::to_string(semester);
    }

    // Helper method to get course key (const version)
    string getCourseKey() const {
        return std::to_string(id) + "_s" + std::to_string(semester);
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

    // Validation method to check if course has valid semester
    bool hasValidSemester() const {
        return semester >= 1 && semester <= 4;
    }
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
    int semester;

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

    // Default constructor
    InformativeSchedule() : index(0), semester(1) {}

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