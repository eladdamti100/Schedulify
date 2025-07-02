#include "schedule_filter_service.h"
#include <set>

ScheduleFilterService& ScheduleFilterService::getInstance() {
    static ScheduleFilterService instance;
    return instance;
}

vector<int> ScheduleFilterService::filterSchedulesByQuery(const string& userQuery,
                                                          const vector<int>& availableScheduleIds) {
    try {
        Logger::get().logInfo("=== SCHEDULE FILTER SERVICE ===");
        Logger::get().logInfo("User query: " + userQuery);
        Logger::get().logInfo("Available schedule IDs: " + std::to_string(availableScheduleIds.size()));

        // Initialize database if needed
        if (!initializeIfNeeded()) {
            Logger::get().logError("Failed to initialize database for filtering");
            return availableScheduleIds; // Return all if we can't filter
        }

        // Create bot query request
        BotQueryRequest request;
        request.userMessage = userQuery;
        request.scheduleMetadata = getScheduleMetadata();
        request.availableScheduleIds = availableScheduleIds;

        // Use Claude API to process the query
        ClaudeAPIClient claudeClient;
        BotQueryResponse claudeResponse = claudeClient.processScheduleQuery(request);

        if (claudeResponse.hasError) {
            Logger::get().logError("Claude API error: " + claudeResponse.errorMessage);
            return availableScheduleIds; // Return all if Claude fails
        }

        // If this is a filter query, execute the SQL
        if (claudeResponse.isFilterQuery && !claudeResponse.sqlQuery.empty()) {
            Logger::get().logInfo("Executing SQL filter: " + claudeResponse.sqlQuery);

            vector<int> filteredIds = executeSQLFilter(
                    claudeResponse.sqlQuery,
                    claudeResponse.queryParameters,
                    availableScheduleIds
            );

            Logger::get().logInfo("Filter complete: " + std::to_string(filteredIds.size()) +
                                  " schedules match criteria");
            return filteredIds;
        } else {
            Logger::get().logInfo("No filter query - returning all available schedules");
            return availableScheduleIds; // No filtering needed
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception in schedule filtering: " + std::string(e.what()));
        return availableScheduleIds; // Return all on error
    }
}

string ScheduleFilterService::getScheduleMetadata() {
    if (!initializeIfNeeded()) {
        return "Database not available for schedule metadata";
    }

    try {
        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            return "Database not connected";
        }

        return db.schedules()->getSchedulesMetadataForBot();

    } catch (const std::exception& e) {
        Logger::get().logError("Exception getting schedule metadata: " + std::string(e.what()));
        return "Error retrieving schedule metadata: " + std::string(e.what());
    }
}

bool ScheduleFilterService::isReady() {
    return initializeIfNeeded();
}

bool ScheduleFilterService::initializeIfNeeded() {
    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            Logger::get().logInfo("Initializing database for schedule filtering");
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logError("Failed to initialize database");
                return false;
            }
        }

        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("Database not connected");
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception initializing database: " + std::string(e.what()));
        return false;
    }
}

vector<int> ScheduleFilterService::executeSQLFilter(const string& sqlQuery,
                                                    const vector<string>& parameters,
                                                    const vector<int>& availableScheduleIds) {
    try {
        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("Database not connected for SQL execution");
            return availableScheduleIds;
        }

        // Execute the SQL query to get matching schedule IDs
        vector<int> allMatchingIds = db.schedules()->executeCustomQuery(sqlQuery, parameters);

        Logger::get().logInfo("SQL query returned " + std::to_string(allMatchingIds.size()) + " total matches");

        // Filter to only include available schedule IDs
        vector<int> filteredIds;
        std::set<int> availableSet(availableScheduleIds.begin(), availableScheduleIds.end());

        for (int scheduleId : allMatchingIds) {
            if (availableSet.find(scheduleId) != availableSet.end()) {
                filteredIds.push_back(scheduleId);
            }
        }

        Logger::get().logInfo("Filtered to " + std::to_string(filteredIds.size()) +
                              " schedules from available set");

        return filteredIds;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception executing SQL filter: " + std::string(e.what()));
        return availableScheduleIds; // Return all on error
    }
}