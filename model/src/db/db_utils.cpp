#include "db_utils.h"
#include "logger.h"
#include <QSqlError>
#include <QFileInfo>
#include <QTime>
#include <QElapsedTimer>

DatabaseUtils::PerformanceStats DatabaseUtils::performanceStats;

bool DatabaseUtils::enableWALMode(QSqlDatabase& db) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for WAL mode setup");
        return false;
    }

    QSqlQuery query(db);

    // Enable WAL mode for better concurrent access
    if (!query.exec("PRAGMA journal_mode=WAL")) {
        Logger::get().logWarning("Failed to enable WAL mode: " + query.lastError().text().toStdString());
        return false;
    }

    Logger::get().logInfo("WAL mode enabled successfully");
    return true;
}

bool DatabaseUtils::optimizeForBulkInserts(QSqlDatabase& db) {
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    bool success = true;

    // Optimize settings for bulk inserts
    std::vector<QString> optimizations = {
            "PRAGMA synchronous=OFF",           // Faster writes, less crash safety
            "PRAGMA cache_size=10000",          // Larger cache
            "PRAGMA temp_store=MEMORY",         // Use memory for temp storage
            "PRAGMA journal_mode=MEMORY"        // Keep journal in memory
    };

    for (const QString& pragma : optimizations) {
        if (!query.exec(pragma)) {
            Logger::get().logWarning("Failed to apply optimization: " + pragma.toStdString() +
                                     " - " + query.lastError().text().toStdString());
            success = false;
        }
    }

    if (success) {
        Logger::get().logInfo("Database optimized for bulk operations");
    }

    return success;
}

bool DatabaseUtils::restoreNormalSettings(QSqlDatabase& db) {
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    bool success = true;

    // Restore normal settings
    std::vector<QString> normalSettings = {
            "PRAGMA synchronous=FULL",          // Restore crash safety
            "PRAGMA cache_size=2000",           // Normal cache size
            "PRAGMA temp_store=DEFAULT",        // Default temp storage
            "PRAGMA journal_mode=WAL"           // WAL mode for normal operation
    };

    for (const QString& pragma : normalSettings) {
        if (!query.exec(pragma)) {
            Logger::get().logWarning("Failed to restore setting: " + pragma.toStdString());
            success = false;
        }
    }

    if (success) {
        Logger::get().logInfo("Database settings restored to normal");
    }

    return success;
}

bool DatabaseUtils::executeBatch(QSqlDatabase& db, const QString& query,
                                 const std::vector<QVariantList>& batchData) {
    if (!db.isOpen() || batchData.empty()) {
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    BatchTransaction transaction(db);
    if (!transaction.isActive()) {
        recordQuery(false, timer.elapsed(), "Failed to start batch transaction");
        return false;
    }

    QSqlQuery sqlQuery(db);
    if (!sqlQuery.prepare(query)) {
        recordQuery(false, timer.elapsed(), "Failed to prepare batch query: " + sqlQuery.lastError().text().toStdString());
        return false;
    }

    int successCount = 0;
    for (const auto& values : batchData) {
        // Clear previous bindings
        sqlQuery.finish();

        // Bind new values
        for (const QVariant& value : values) {
            sqlQuery.addBindValue(value);
        }

        if (sqlQuery.exec()) {
            successCount++;
        } else {
            Logger::get().logWarning("Batch query failed: " + sqlQuery.lastError().text().toStdString());
        }
    }

    bool success = (successCount == static_cast<int>(batchData.size()));

    if (success && transaction.commit()) {
        recordQuery(true, timer.elapsed());
        Logger::get().logInfo("Batch execution successful: " + std::to_string(successCount) + " queries");
        return true;
    } else {
        recordQuery(false, timer.elapsed(), "Batch transaction failed");
        return false;
    }
}

bool DatabaseUtils::vacuum(QSqlDatabase& db) {
    if (!db.isOpen()) {
        return false;
    }

    Logger::get().logInfo("Starting database VACUUM operation...");
    QElapsedTimer timer;
    timer.start();

    QSqlQuery query("VACUUM", db);
    bool success = query.exec();

    if (success) {
        Logger::get().logInfo("VACUUM completed in " + std::to_string(timer.elapsed()) + "ms");
    } else {
        Logger::get().logError("VACUUM failed: " + query.lastError().text().toStdString());
    }

    recordQuery(success, timer.elapsed(), success ? "" : query.lastError().text().toStdString());
    return success;
}

bool DatabaseUtils::analyze(QSqlDatabase& db) {
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query("ANALYZE", db);
    bool success = query.exec();

    if (success) {
        Logger::get().logInfo("Database ANALYZE completed");
    } else {
        Logger::get().logError("ANALYZE failed: " + query.lastError().text().toStdString());
    }

    return success;
}

QString DatabaseUtils::getDatabaseSize(QSqlDatabase& db) {
    if (!db.isOpen()) {
        return "Unknown";
    }

    QFileInfo dbFile(db.databaseName());
    if (dbFile.exists()) {
        qint64 sizeBytes = dbFile.size();

        if (sizeBytes < 1024) {
            return QString::number(sizeBytes) + " bytes";
        } else if (sizeBytes < 1024 * 1024) {
            return QString::number(sizeBytes / 1024.0, 'f', 1) + " KB";
        } else {
            return QString::number(sizeBytes / (1024.0 * 1024.0), 'f', 1) + " MB";
        }
    }

    return "Unknown";
}

int DatabaseUtils::getTableRowCount(QSqlDatabase& db, const QString& tableName) {
    if (!db.isOpen()) {
        return -1;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM " + tableName);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return -1;
}

bool DatabaseUtils::tableExists(QSqlDatabase& db, const QString& tableName) {
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    query.addBindValue(tableName);

    return query.exec() && query.next();
}

std::vector<QString> DatabaseUtils::getTableNames(QSqlDatabase& db) {
    std::vector<QString> tables;

    if (!db.isOpen()) {
        return tables;
    }

    QSqlQuery query("SELECT name FROM sqlite_master WHERE type='table'", db);

    while (query.next()) {
        tables.push_back(query.value(0).toString());
    }

    return tables;
}

// BatchTransaction implementation
DatabaseUtils::BatchTransaction::BatchTransaction(QSqlDatabase& database) : db(database) {
    active = db.transaction();
    if (!active) {
        Logger::get().logError("Failed to start batch transaction");
    }
}

DatabaseUtils::BatchTransaction::~BatchTransaction() {
    if (active && !committed && !rolledBack) {
        rollback();
    }
}

bool DatabaseUtils::BatchTransaction::commit() {
    if (!active || committed || rolledBack) {
        return false;
    }

    committed = db.commit();
    if (!committed) {
        Logger::get().logError("Failed to commit batch transaction");
    }

    return committed;
}

void DatabaseUtils::BatchTransaction::rollback() {
    if (!active || committed || rolledBack) {
        return;
    }

    db.rollback();
    rolledBack = true;
}

// Performance monitoring
DatabaseUtils::PerformanceStats& DatabaseUtils::getStats() {
    return performanceStats;
}

void DatabaseUtils::resetStats() {
    performanceStats = PerformanceStats();
}

void DatabaseUtils::logPerformanceReport() {
    Logger::get().logInfo("=== DATABASE PERFORMANCE REPORT ===");
    Logger::get().logInfo("Total Queries: " + std::to_string(performanceStats.totalQueries));
    Logger::get().logInfo("Successful: " + std::to_string(performanceStats.successfulQueries));
    Logger::get().logInfo("Failed: " + std::to_string(performanceStats.failedQueries));
    Logger::get().logInfo("Average Query Time: " + std::to_string(performanceStats.averageQueryTime) + "ms");

    if (!performanceStats.lastError.empty()) {
        Logger::get().logInfo("Last Error: " + performanceStats.lastError);
    }
}

void DatabaseUtils::recordQuery(bool success, double timeMs, const std::string& error) {
    performanceStats.totalQueries++;

    if (success) {
        performanceStats.successfulQueries++;
    } else {
        performanceStats.failedQueries++;
        performanceStats.lastError = error;
    }

    // Update running average
    double totalTime = performanceStats.averageQueryTime * (performanceStats.totalQueries - 1) + timeMs;
    performanceStats.averageQueryTime = totalTime / performanceStats.totalQueries;
}