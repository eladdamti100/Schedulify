#ifndef DB_SCHEDULES_H
#define DB_SCHEDULES_H

#include "db_entities.h"
#include "model_interfaces.h"
#include "db_json_helpers.h"
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

    // Schedule CRUD operations
    bool insertSchedule(const InformativeSchedule& schedule);
    bool insertSchedules(const vector<InformativeSchedule>& schedules);
    bool deleteAllSchedules();

    // Schedule retrieval operations
    vector<InformativeSchedule> getAllSchedules();

    // SQL filtering operations for bot functionality
    vector<int> executeCustomQuery(const string& sqlQuery, const vector<string>& parameters);
    vector<string> executeCustomQueryForUniqueIds(const string& sqlQuery, const vector<string>& parameters);
    string getUniqueIdByScheduleIndex(int scheduleIndex, const string& semester);
    int getScheduleIndexByUniqueId(const string& uniqueId);
    vector<int> getScheduleIndicesByUniqueIds(const vector<string>& uniqueIds);


    vector<InformativeSchedule> getSchedulesByIds(const vector<int>& scheduleIds);
    string getSchedulesMetadataForBot();

    // Utility operations
    int getScheduleCount();

    // Performance operations for bulk inserts
    bool insertSchedulesBulk(const vector<InformativeSchedule>& schedules);

private:
    QSqlDatabase& db;

    mutable mutex dbMutex;

    // Helper methods
    static InformativeSchedule createScheduleFromQuery(QSqlQuery& query);
    static bool isValidScheduleQuery(const string& sqlQuery);
    vector<string> getWhitelistedTables();
    static vector<string> getWhitelistedColumns();
    void debugScheduleQuery(const string& debugQuery);

};

#endif // DB_SCHEDULES_H