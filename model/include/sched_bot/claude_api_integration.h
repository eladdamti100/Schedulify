#ifndef CLAUDE_API_INTEGRATION_H
#define CLAUDE_API_INTEGRATION_H

#include <curl/curl.h>
#include <json/json.h>
#include <cstdlib>

#include "model_interfaces.h"
#include "logger.h"
#include "sql_validator.h"
#include "db_manager.h"

#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <thread>

class ClaudeAPIClient {
public:
    ClaudeAPIClient();
    ~ClaudeAPIClient();

    // Main API method
    BotQueryResponse processScheduleQuery(const BotQueryRequest& request);
    static BotQueryResponse generateFallbackResponse(const BotQueryRequest& request);
    static BotQueryResponse ActivateBot(const BotQueryRequest& request);

private:
    // API interaction methods
    Json::Value createRequestPayload(const BotQueryRequest& request);
    static std::string createSystemPrompt(const std::string& scheduleMetadata);
    static BotQueryResponse parseClaudeResponse(const std::string& responseData);
    static bool extractSQLQuery(const std::string& content, std::string& sqlQuery, std::vector<std::string>& parameters);

    const std::string CLAUDE_API_URL = "https://api.anthropic.com/v1/messages";
    const std::string CLAUDE_MODEL = "claude-3-5-sonnet-20241022";
};

#endif // CLAUDE_API_INTEGRATION_H