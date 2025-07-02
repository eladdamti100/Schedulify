#include "cleanup_manager.h"

void CleanupManager::performCleanup() {
    if (!QCoreApplication::instance()) {
        return;
    }

    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (dbIntegration.isInitialized()) {
            auto& db = DatabaseManager::getInstance();
            if (db.isConnected()) {

                if (clearScheduleData(db)) {
                    Logger::get().logInfo("Schedule data cleared successfully");
                } else {
                    Logger::get().logWarning("Failed to clear some schedule data");
                }

                // Update metadata about cleanup
                try {
                    db.updateMetadata("last_cleanup",
                                      QDateTime::currentDateTime().toString(Qt::ISODate).toStdString());
                } catch (const std::exception& e) {
                    Logger::get().logWarning("Failed to update cleanup metadata: " + std::string(e.what()));
                }
            } else {
                Logger::get().logInfo("Database was not connected - no cleanup needed");
            }

            // Use force cleanup to ensure proper shutdown
            try {
                db.forceCleanup();
            } catch (const std::exception& e) {
                Logger::get().logWarning("Exception during force cleanup: " + std::string(e.what()));
            }
        } else {
            Logger::get().logInfo("Database was not initialized - no cleanup needed");
        }
    } catch (const std::exception& e) {
        Logger::get().logError("Exception during database cleanup: " + std::string(e.what()));
    }
}

bool CleanupManager::clearScheduleData(DatabaseManager& db) {
    bool success = true;

    try {
        if (db.schedules()->deleteAllSchedules()) {
            Logger::get().logInfo("All schedules cleared successfully");
        } else {
            Logger::get().logWarning("Failed to clear some schedules");
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception during schedule cleanup: " + std::string(e.what()));
        success = false;
    }

    return success;
}