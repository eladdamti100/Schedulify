#ifndef CLEANUP_MANAGER_H
#define CLEANUP_MANAGER_H

#include "db_manager.h"
#include "model_db_integration.h"
#include "logger.h"

class CleanupManager {
public:
    // Perform application cleanup
    static void performCleanup();

private:
    // Clear schedule-related data from database
    static bool clearScheduleData(DatabaseManager& db);
};

#endif // CLEANUP_MANAGER_H