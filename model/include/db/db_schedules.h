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
    bool insertSchedule(const InformativeSchedule& schedule, const string& scheduleName = "",
                        const vector<int>& sourceFileIds = {});
    bool insertSchedules(const vector<InformativeSchedule>& schedules, const string& setName = "",
                         const vector<int>& sourceFileIds = {});
    bool deleteAllSchedules();
    bool deleteSchedulesBySetId(int setId);

    // Schedule retrieval operations
    static vector<InformativeSchedule> getAllSchedules();
    vector<InformativeSchedule> getSchedulesBySetId(int setId);

    // Schedule filtering operations
    static vector<InformativeSchedule> getSchedulesByMetrics(int maxDays = -1, int maxGaps = -1, int maxGapTime = -1,
                                                      int minAvgStart = -1,int maxAvgStart = -1, int minAvgEnd = -1,
                                                      int maxAvgEnd = -1);

    // SQL filtering operations for bot functionality
    vector<int> executeCustomQuery(const string& sqlQuery, const vector<string>& parameters);
    vector<InformativeSchedule> getSchedulesByIds(const vector<int>& scheduleIds);
    string getSchedulesMetadataForBot();

    // Schedule set operations
    int createScheduleSet(const string& setName, const vector<int>& sourceFileIds = {});
    vector<ScheduleSetEntity> getAllScheduleSets();
    ScheduleSetEntity getScheduleSetById(int setId);
    bool deleteScheduleSet(int setId);

    // Utility operations
    bool scheduleSetExists(int setId);
    int getScheduleCount();
    int getScheduleCountBySetId(int setId);
    int getScheduleSetCount();

    // Statistics operations
    map<string, int> getScheduleStatistics();

    // Performance operations for bulk inserts
    bool insertSchedulesBulk(const vector<InformativeSchedule>& schedules, int setId);

private:

    QSqlDatabase& db;

    // Helper methods
    static InformativeSchedule createScheduleFromQuery(QSqlQuery& query);
    static ScheduleSetEntity createScheduleSetFromQuery(QSqlQuery& query);
    bool updateScheduleSetCount(int setId);
    static bool isValidScheduleQuery(const string& sqlQuery);
    vector<string> getWhitelistedTables();
    static vector<string> getWhitelistedColumns();
    void debugScheduleQuery(const string& debugQuery);
};

#endif // DB_SCHEDULES_H