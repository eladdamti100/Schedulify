#ifndef DB_JSON_HELPERS_H
#define DB_JSON_HELPERS_H

#include "model_interfaces.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <string>
#include <vector>

class DatabaseJsonHelpers {
public:
    // Group JSON conversion
    static string groupsToJson(const vector<Group>& groups);
    static vector<Group> groupsFromJson(const string& json);

    // Enhanced Schedule JSON conversion
    static string scheduleToJson(const InformativeSchedule& schedule);
    static InformativeSchedule scheduleFromJson(const string& json, int id, int schedule_index,
                                                int amount_days, int amount_gaps, int gaps_time,
                                                int avg_start, int avg_end);

    // Course JSON conversion helpers (for complete serialization if needed)
    static string courseToJson(const Course& course);
    static Course courseFromJson(const string& json, const string& uniqid, int course_id,
                                 const string& raw_id, const string& name, const string& teacher,
                                 int semester, const string& course_key);

    // Session type conversion utilities
    static string sessionTypeToString(SessionType type);
    static SessionType sessionTypeFromString(const string& typeStr);

    // Enhanced validation helpers
    static bool isValidJson(const string& json);
    static bool validateGroupsJson(const string& json);
    static bool validateScheduleJson(const string& json);

private:
    DatabaseJsonHelpers() = default; // Static class, no instantiation

    // Helper methods for JSON object creation
    static QJsonObject sessionToJsonObject(const Session& session);
    static Session sessionFromJsonObject(const QJsonObject& obj);
    static QJsonObject groupToJsonObject(const Group& group);
    static Group groupFromJsonObject(const QJsonObject& obj);

    // Schedule-specific JSON helpers
    static QJsonObject scheduleItemToJsonObject(const ScheduleItem& item);
    static ScheduleItem scheduleItemFromJsonObject(const QJsonObject& obj);
    static QJsonObject scheduleDayToJsonObject(const ScheduleDay& day);
    static ScheduleDay scheduleDayFromJsonObject(const QJsonObject& obj);
};

#endif // DB_JSON_HELPERS_H