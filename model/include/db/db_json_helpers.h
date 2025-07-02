#ifndef DB_JSON_HELPERS_H
#define DB_JSON_HELPERS_H

#include "model_interfaces.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <string>
#include <vector>

using namespace std;

class DatabaseJsonHelpers {
public:
    // Group JSON conversion
    static string groupsToJson(const vector<Group>& groups);
    static vector<Group> groupsFromJson(const string& json);

    // Schedule JSON conversion
    static string scheduleToJson(const InformativeSchedule& schedule);
    static InformativeSchedule scheduleFromJson(const string& json, int id, int schedule_index,
                                                int amount_days, int amount_gaps, int gaps_time,
                                                int avg_start, int avg_end);

    // Course JSON conversion helpers
    static string courseToJson(const Course& course);
    static Course courseFromJson(const string& json, int id, const string& raw_id,
                                 const string& name, const string& teacher, int semester = 1);

private:
    DatabaseJsonHelpers() = default; // Static class, no instantiation
};

#endif // DB_JSON_HELPERS_H