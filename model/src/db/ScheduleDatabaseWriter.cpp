#include "ScheduleDatabaseWriter.h"

ScheduleDatabaseWriter& ScheduleDatabaseWriter::getInstance() {
    static ScheduleDatabaseWriter instance;
    return instance;
}

ScheduleDatabaseWriter::~ScheduleDatabaseWriter() {
    if (sessionActive) {
        finalizeSession();
    }
}

bool ScheduleDatabaseWriter::initializeSession() {
    if (sessionActive) {
        Logger::get().logWarning("Session already active, finalizing previous session");
        finalizeSession();
    }

    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logError("Failed to initialize database for schedule writing");
                return false;
            }
        }

        sessionActive = true;
        sessionStats = SessionStats();
        sessionStats.sessionActive = true;
        currentBatch.clear();

        Logger::get().logInfo("Schedule writing session initialized");
        return true;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception initializing schedule writing session: " + std::string(e.what()));
        resetSession();
        return false;
    }
}

bool ScheduleDatabaseWriter::writeSchedule(const InformativeSchedule& schedule) {
    if (!sessionActive) {
        Logger::get().logError("No active session for schedule writing");
        return false;
    }

    try {
        // Add to current batch
        currentBatch.push_back(schedule);
        sessionStats.totalSchedulesWritten++;

        // Write batch if it's full
        if (currentBatch.size() >= static_cast<size_t>(batchSize)) {
            return flushBatch();
        }

        return true;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception writing schedule: " + std::string(e.what()));
        sessionStats.failedWrites++;
        return false;
    }
}

bool ScheduleDatabaseWriter::flushBatch() {
    if (currentBatch.empty()) {
        return true; // Nothing to flush
    }

    try {
        if (writeBatchToDatabase()) {
            sessionStats.successfulWrites += currentBatch.size();
            currentBatch.clear();

            // Log progress every 1000 successful writes
            if (sessionStats.successfulWrites % 1000 == 0) {
                Logger::get().logInfo("Progress: " + std::to_string(sessionStats.successfulWrites) + " schedules written");
            }

            return true;
        } else {
            sessionStats.failedWrites += currentBatch.size();
            currentBatch.clear(); // Clear batch even on failure to prevent repeated failures
            return false;
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception flushing batch: " + std::string(e.what()));
        sessionStats.failedWrites += currentBatch.size();
        currentBatch.clear();
        return false;
    }
}

bool ScheduleDatabaseWriter::finalizeSession() {
    if (!sessionActive) {
        return true; // Nothing to finalize
    }

    try {
        // Flush any remaining schedules
        bool success = flushBatch();

        // Update schedule count in set
        auto& db = DatabaseManager::getInstance();

        // Log session results
        Logger::get().logInfo("=== SCHEDULE WRITING SESSION COMPLETED ===");
        Logger::get().logInfo("Total Processed: " + std::to_string(sessionStats.totalSchedulesWritten));
        Logger::get().logInfo("Successfully Written: " + std::to_string(sessionStats.successfulWrites));
        Logger::get().logInfo("Failed Writes: " + std::to_string(sessionStats.failedWrites));

        resetSession();
        return success;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception finalizing session: " + std::string(e.what()));
        resetSession();
        return false;
    }
}

bool ScheduleDatabaseWriter::writeBatchToDatabase() {
    if (currentBatch.empty()) {
        return true;
    }

    try {
        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("Database not connected during batch write");
            return false;
        }

        // Use simplified bulk insert
        bool success = db.schedules()->insertSchedules(currentBatch);

        return success;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception in batch write: " + std::string(e.what()));
        return false;
    }
}

void ScheduleDatabaseWriter::resetSession() {
    sessionActive = false;
    sessionStats = SessionStats();
    currentBatch.clear();
}