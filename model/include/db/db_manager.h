#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "db_entities.h"
#include "db_schema.h"
#include "db_files.h"
#include "db_courses.h"
#include "db_schedules.h"
#include "logger.h"
#include "db_json_helpers.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QVariant>
#include <QString>
#include <vector>
#include <string>
#include <memory>
#include <QDir>

using namespace std;

class DatabaseManager {
public:
    static DatabaseManager& getInstance();

    void forceCleanup();

    // Database lifecycle
    bool initializeDatabase(const QString& dbPath = QString());
    bool isConnected() const;
    void closeDatabase();

    DatabaseFileManager* files() { return fileManager.get(); }
    DatabaseCourseManager* courses() { return courseManager.get(); }
    DatabaseSchema* schema() { return schemaManager.get(); }

    bool insertMetadata(const string& key, const string& value, const string& description = "");
    bool updateMetadata(const string& key, const string& value);
    string getMetadata(const string& key, const string& defaultValue = "");
    vector<MetadataEntity> getAllMetadata();

    bool clearAllData();
    int getTableRowCount(const string& tableName);

    bool repairDatabase();
    void debugDatabaseSchema();

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    static int getCurrentSchemaVersion() { return CURRENT_SCHEMA_VERSION; }

    DatabaseScheduleManager* schedules() { return scheduleManager.get(); }

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    bool hasActiveConnections();
    void forceCloseActiveQueries();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    DatabaseManager(DatabaseManager&&) = delete;
    DatabaseManager& operator=(DatabaseManager&&) = delete;

    bool executeQuery(const QString& query, const QVariantList& params = QVariantList());

    static bool isQtApplicationReady();

    QSqlDatabase db;
    bool isInitialized = false;

    std::unique_ptr<DatabaseSchema> schemaManager;
    std::unique_ptr<DatabaseFileManager> fileManager;
    std::unique_ptr<DatabaseCourseManager> courseManager;

    static const int CURRENT_SCHEMA_VERSION = 1;

    friend class DatabaseRepair;
    std::unique_ptr<DatabaseScheduleManager> scheduleManager;
};

class DatabaseTransaction {
public:
    explicit DatabaseTransaction(DatabaseManager& dbManager);
    ~DatabaseTransaction();

    bool commit();
    void rollback();

private:
    DatabaseManager& db;
    bool committed = false;
    bool rolledBack = false;
};

#endif // DATABASE_MANAGER_H