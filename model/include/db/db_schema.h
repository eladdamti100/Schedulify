#ifndef DB_SCHEMA_H
#define DB_SCHEMA_H

#include "logger.h"

#include <QSqlDatabase>
#include <QString>
#include <QSqlQuery>
#include <QSqlError>

class DatabaseSchema {
public:
    explicit DatabaseSchema(QSqlDatabase& database);

    // Schema management
    bool createTables();
    bool createIndexes();
    bool dropAllTables();

    // Schema versioning
    static int getCurrentSchemaVersion() { return CURRENT_SCHEMA_VERSION; }
    static bool upgradeSchema(int fromVersion, int toVersion);

    // Validation
    bool validateSchema();
    bool tableExists(const QString& tableName);

private:
    QSqlDatabase& db;
    static const int CURRENT_SCHEMA_VERSION = 1;

    // Individual table creation methods
    bool createMetadataTable();
    bool createFileTable();
    bool createCourseTable();

    // Index creation methods
    bool createFileIndexes();
    bool createCourseIndexes();
    bool createMetadataIndexes();

    // Schema upgrade methods
    bool upgradeFromV1ToV2();
    bool upgradeFromV2ToV3();

    // Validation methods
    bool validateFileTableColumns();
    bool validateCourseTableColumns();

    // Utility methods
    bool executeQuery(const QString& query);

    // Schema creation methods
    bool createScheduleSetTable();
    bool createScheduleTable();

    // Index creation methods
    bool createScheduleSetIndexes();
    bool createScheduleIndexes();

    // Validation methods
    bool validateScheduleSetTableColumns();
    bool validateScheduleTableColumns();
};

#endif // DB_SCHEMA_H