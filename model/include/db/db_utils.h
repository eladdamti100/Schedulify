#ifndef DB_UTILS_H
#define DB_UTILS_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <vector>
#include <string>
#include <functional>

class DatabaseUtils {
public:
    // Performance optimization utilities
    static bool enableWALMode(QSqlDatabase& db);
    static bool optimizeForBulkInserts(QSqlDatabase& db);
    static bool restoreNormalSettings(QSqlDatabase& db);

    // Batch operation helpers
    static bool executeBatch(QSqlDatabase& db, const QString& query,
                             const std::vector<QVariantList>& batchData);

    // Database maintenance
    static bool vacuum(QSqlDatabase& db);
    static bool analyze(QSqlDatabase& db);
    static QString getDatabaseSize(QSqlDatabase& db);

    // Query helpers
    static int getTableRowCount(QSqlDatabase& db, const QString& tableName);
    static bool tableExists(QSqlDatabase& db, const QString& tableName);
    static std::vector<QString> getTableNames(QSqlDatabase& db);

    // Transaction helpers
    class BatchTransaction {
    public:
        explicit BatchTransaction(QSqlDatabase& database);
        ~BatchTransaction();

        bool commit();
        void rollback();
        bool isActive() const { return active && !committed && !rolledBack; }

    private:
        QSqlDatabase& db;
        bool active = false;
        bool committed = false;
        bool rolledBack = false;
    };

    // Performance monitoring
    struct PerformanceStats {
        int totalQueries = 0;
        int successfulQueries = 0;
        int failedQueries = 0;
        double averageQueryTime = 0.0;
        std::string lastError;
    };

    static PerformanceStats& getStats();
    static void resetStats();
    static void logPerformanceReport();

private:
    static PerformanceStats performanceStats;
    static void recordQuery(bool success, double timeMs, const std::string& error = "");
};

#endif // DB_UTILS_H