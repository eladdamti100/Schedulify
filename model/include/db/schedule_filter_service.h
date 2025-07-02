#ifndef SCHEDULE_FILTER_SERVICE_H
#define SCHEDULE_FILTER_SERVICE_H

#include "claude_api_integration.h"
#include "model_interfaces.h"
#include "model_db_integration.h"
#include "db_manager.h"
#include "logger.h"

#include <string>
#include <vector>

using namespace std;

class ScheduleFilterService {
public:
    static ScheduleFilterService& getInstance();

    // Main filtering method - takes user query and returns filtered schedule IDs
    vector<int> filterSchedulesByQuery(const string& userQuery,
                                       const vector<int>& availableScheduleIds);

    // Get schedule metadata for Claude API
    string getScheduleMetadata();

    // Check if service is ready (database initialized)
    bool isReady();

private:
    ScheduleFilterService() = default;
    ~ScheduleFilterService() = default;

    // Disable copy/move
    ScheduleFilterService(const ScheduleFilterService&) = delete;
    ScheduleFilterService& operator=(const ScheduleFilterService&) = delete;

    // Helper methods
    bool initializeIfNeeded();
    vector<int> executeSQLFilter(const string& sqlQuery,
                                 const vector<string>& parameters,
                                 const vector<int>& availableScheduleIds);
};

#endif // SCHEDULE_FILTER_SERVICE_H