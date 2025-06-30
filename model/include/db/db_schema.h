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

    // Schema versioning
    static int getCurrentSchemaVersion() { return CURRENT_SCHEMA_VERSION; }

    // Validation
    bool tableExists(const QString& tableName);

private:
    QSqlDatabase& db;
    static const int CURRENT_SCHEMA_VERSION = 1;

    // Individual table creation methods
    bool createMetadataTable();
    bool createFileTable();
    bool createCourseTable();
    bool createScheduleTable();

    // Index creation methods
    bool createFileIndexes();
    bool createCourseIndexes();
    bool createMetadataIndexes();
    bool createScheduleIndexes();


    // Utility methods
    bool executeQuery(const QString& query);

};

#endif // DB_SCHEMA_H