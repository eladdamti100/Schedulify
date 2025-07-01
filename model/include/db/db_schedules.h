#ifndef DB_SCHEDULES_H
#define DB_SCHEDULES_H

#include "model_interfaces.h"
#include "db_json_helpers.h"
#include "sql_validator.h"
#include "db_entities.h"
#include "db_utils.h"
#include "logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QSqlDatabase>
#include <vector>
#include <string>
#include <map>

using namespace std;

class DatabaseScheduleManager {
public:
    explicit DatabaseScheduleManager(QSqlDatabase& database);

    // Core CRUD operations
    bool insertSchedule(const InformativeSchedule& schedule, const string& name = "");
    bool insertSchedules(const vector<InformativeSchedule>& schedules);
    bool deleteAllSchedules();

    // Essential retrieval operations
    vector<InformativeSchedule> getAllSchedules();
    vector<InformativeSchedule> getSchedulesBySemester(int semester);
    vector<InformativeSchedule> getSchedulesByIds(const vector<int>& scheduleIds);

    // SQL filtering for bot integration
    vector<int> executeCustomQuery(const string& sqlQuery, const vector<string>& parameters);

    // Utility operations
    int getScheduleCount();

    // Performance operations for bulk inserts
    bool insertSchedulesBulk(const vector<InformativeSchedule>& schedules);

    // Bot integration - metadata generation
    string getSchedulesMetadataForBot();

private:
    QSqlDatabase& db;

    // Helper methods
    static InformativeSchedule createScheduleFromQuery(QSqlQuery& query);
    static bool isValidScheduleQuery(const string& sqlQuery);
    vector<string> getWhitelistedTables();
    static vector<string> getWhitelistedColumns();
};

#endif // DB_SCHEDULES_H