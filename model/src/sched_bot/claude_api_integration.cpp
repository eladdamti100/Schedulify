#include <thread>
#include "claude_api_integration.h"

struct APIResponse {
    string data;
    long response_code;
    bool success;

    APIResponse() : response_code(0), success(false) {}
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, APIResponse* response) {
    size_t totalSize = size * nmemb;
    response->data.append((char*)contents, totalSize);
    return totalSize;
}

ClaudeAPIClient::ClaudeAPIClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    Logger::get().logInfo("Claude API client initialized with enhanced error handling");
}

ClaudeAPIClient::~ClaudeAPIClient() {
    curl_global_cleanup();
}

BotQueryResponse ClaudeAPIClient::processScheduleQuery(const BotQueryRequest& request) {
    BotQueryResponse response;

    const char* apiKey = getenv("ANTHROPIC_API_KEY");
    if (!apiKey || strlen(apiKey) == 0) {
        Logger::get().logError("ANTHROPIC_API_KEY environment variable not set");
        response.hasError = true;
        response.errorMessage = "API key not configured. Please set ANTHROPIC_API_KEY environment variable.";
        return response;
    }

    // Clean API key
    string cleanApiKey;
    for (char c : string(apiKey)) {
        if (c >= 33 && c <= 126) {
            cleanApiKey += c;
        }
    }

    Logger::get().logInfo("Starting Claude API request with retry logic");

    try {
        // Create the request payload
        Json::Value requestJson = createRequestPayload(request);
        Json::StreamWriterBuilder builder;
        string jsonString = Json::writeString(builder, requestJson);

        Logger::get().logInfo("Request payload size: " + to_string(jsonString.length()) + " bytes");

        // Retry logic for rate limiting and temporary failures
        const int maxRetries = 3;
        const vector<int> retryDelays = {2, 5, 10}; // seconds

        for (int attempt = 1; attempt <= maxRetries; ++attempt) {
            Logger::get().logInfo("API request attempt " + to_string(attempt) + "/" + to_string(maxRetries));

            CURL* curl = curl_easy_init();
            if (!curl) {
                response.hasError = true;
                response.errorMessage = "Failed to initialize CURL";
                return response;
            }

            APIResponse apiResponse;

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("x-api-key: " + cleanApiKey).c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

            // Add user agent for better API handling
            headers = curl_slist_append(headers, "User-Agent: SchedGUI/1.0");

            curl_easy_setopt(curl, CURLOPT_URL, CLAUDE_API_URL.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &apiResponse);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);      // Increased timeout
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

            Logger::get().logInfo("Sending request to Claude API...");

            CURLcode res = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &apiResponse.response_code);

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            Logger::get().logInfo("CURL result: " + to_string(res));
            Logger::get().logInfo("HTTP response code: " + to_string(apiResponse.response_code));

            if (res != CURLE_OK) {
                Logger::get().logError("Network error: " + string(curl_easy_strerror(res)));

                if (attempt < maxRetries) {
                    Logger::get().logInfo("Retrying in " + to_string(retryDelays[attempt-1]) + " seconds...");
                    this_thread::sleep_for(chrono::seconds(retryDelays[attempt-1]));
                    continue;
                } else {
                    response.hasError = true;
                    response.errorMessage = "Network error after " + to_string(maxRetries) + " attempts: " + string(curl_easy_strerror(res));
                    return response;
                }
            }

            // Handle different HTTP response codes
            if (apiResponse.response_code == 200) {
                // Success!
                Logger::get().logInfo("Claude API request successful on attempt " + to_string(attempt));

                if (apiResponse.data.empty()) {
                    Logger::get().logError("Empty response from Claude API");
                    response.hasError = true;
                    response.errorMessage = "Empty response from Claude API";
                    return response;
                }

                // Parse the response
                response = parseClaudeResponse(apiResponse.data);
                Logger::get().logInfo("Claude API request completed successfully");
                return response;

            } else if (apiResponse.response_code == 429 || apiResponse.response_code == 529) {
                // Rate limiting or overloaded
                Logger::get().logWarning("Claude API rate limited/overloaded (HTTP " + to_string(apiResponse.response_code) + ")");
                Logger::get().logWarning("Response: " + apiResponse.data.substr(0, 200) + "...");

                if (attempt < maxRetries) {
                    int delaySeconds = retryDelays[attempt-1] * 2; // Longer delay for rate limits
                    Logger::get().logInfo("Rate limited - retrying in " + to_string(delaySeconds) + " seconds...");
                    this_thread::sleep_for(chrono::seconds(delaySeconds));
                    continue;
                } else {
                    response.hasError = true;
                    response.errorMessage = "Claude API is currently overloaded. Please try again in a few minutes.";
                    return response;
                }

            } else if (apiResponse.response_code == 401) {
                // Authentication error - don't retry
                Logger::get().logError("Claude API authentication failed (HTTP 401)");
                Logger::get().logError("Response: " + apiResponse.data);
                response.hasError = true;
                response.errorMessage = "API authentication failed. Please check your ANTHROPIC_API_KEY.";
                return response;

            } else if (apiResponse.response_code == 400) {
                // Bad request - don't retry
                Logger::get().logError("Claude API bad request (HTTP 400)");
                Logger::get().logError("Response: " + apiResponse.data);
                response.hasError = true;
                response.errorMessage = "Invalid request sent to Claude API. Please check the query format.";
                return response;

            } else {
                // Other HTTP errors
                Logger::get().logError("Claude API returned HTTP " + to_string(apiResponse.response_code));
                Logger::get().logError("Response: " + apiResponse.data.substr(0, 500) + "...");

                if (attempt < maxRetries && apiResponse.response_code >= 500) {
                    // Server errors - retry
                    Logger::get().logInfo("Server error - retrying in " + to_string(retryDelays[attempt-1]) + " seconds...");
                    this_thread::sleep_for(chrono::seconds(retryDelays[attempt-1]));
                    continue;
                } else {
                    response.hasError = true;
                    response.errorMessage = "Claude API request failed with HTTP " + to_string(apiResponse.response_code);
                    return response;
                }
            }
        }

        // If we get here, all retries failed
        response.hasError = true;
        response.errorMessage = "Claude API request failed after " + to_string(maxRetries) + " attempts";
        return response;

    } catch (const exception& e) {
        Logger::get().logError("Exception in Claude API request: " + string(e.what()));
        response.hasError = true;
        response.errorMessage = "Request processing error: " + string(e.what());
    }

    return response;
}

BotQueryResponse ClaudeAPIClient::generateFallbackResponse(const BotQueryRequest& request) {
    BotQueryResponse response;

    Logger::get().logInfo("Generating fallback response for Claude API failure");

    string userMsg = request.userMessage;
    transform(userMsg.begin(), userMsg.end(), userMsg.begin(), ::tolower);

    // Simple pattern matching for common queries
    if (userMsg.find("start after") != string::npos || userMsg.find("begin after") != string::npos) {
        if (userMsg.find("10") != string::npos) {
            response.userMessage = "I understand you want schedules that start after 10:00 AM. I'll look for schedules where the earliest class begins after 10:00 AM.";
            response.sqlQuery = "SELECT schedule_index FROM schedule WHERE earliest_start > ?";
            response.queryParameters = {"600"}; // 10:00 AM = 600 minutes
            response.isFilterQuery = true;
        } else if (userMsg.find("9") != string::npos) {
            response.userMessage = "I'll find schedules that start after 9:00 AM for you.";
            response.sqlQuery = "SELECT schedule_index FROM schedule WHERE earliest_start > ?";
            response.queryParameters = {"540"}; // 9:00 AM = 540 minutes
            response.isFilterQuery = true;
        }
    } else if (userMsg.find("no early") != string::npos || userMsg.find("not early") != string::npos) {
        response.userMessage = "I'll find schedules with no early morning classes (before 8:30 AM).";
        response.sqlQuery = "SELECT schedule_index FROM schedule WHERE has_early_morning = ?";
        response.queryParameters = {"0"};
        response.isFilterQuery = true;
    } else if (userMsg.find("no morning") != string::npos) {
        response.userMessage = "I'll find schedules with no morning classes (before 10:00 AM).";
        response.sqlQuery = "SELECT schedule_index FROM schedule WHERE has_morning_classes = ?";
        response.queryParameters = {"0"};
        response.isFilterQuery = true;
    } else {
        // Generic fallback
        response.userMessage = "I'm currently experiencing high demand and cannot process complex queries. Please try a simpler request like 'no early morning classes' or 'start after 10 AM'.";
        response.isFilterQuery = false;
    }

    response.hasError = false;
    return response;
}

Json::Value ClaudeAPIClient::createRequestPayload(const BotQueryRequest& request) {
    Json::Value payload;

    // Set model and parameters
    payload["model"] = CLAUDE_MODEL;
    payload["max_tokens"] = 1024;

    // Create system prompt with schedule metadata
    string systemPrompt = createSystemPrompt(request.scheduleMetadata);
    payload["system"] = systemPrompt;

    // Create messages array
    Json::Value messages(Json::arrayValue);
    Json::Value userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = request.userMessage;
    messages.append(userMessage);

    payload["messages"] = messages;

    return payload;
}

string ClaudeAPIClient::createSystemPrompt(const string& scheduleMetadata) {
    string prompt = R"(
You are SchedBot, an expert schedule filtering assistant. Your job is to analyze user requests and generate SQL queries to filter class schedules.

<schedule_data>
)" + scheduleMetadata + R"(
</schedule_data>

<comprehensive_column_reference>
FILTERABLE COLUMNS WITH DESCRIPTIONS:

BASIC METRICS:
- schedule_index: INTEGER (1-based schedule number, unique identifier)
- amount_days: INTEGER (number of study days, 1-7)
- amount_gaps: INTEGER (total number of gaps between classes)
- gaps_time: INTEGER (total gap time in minutes)
- avg_start: INTEGER (average daily start time in minutes from midnight)
- avg_end: INTEGER (average daily end time in minutes from midnight)

TIME RANGE METRICS:
- earliest_start: INTEGER (earliest class start across all days, minutes from midnight)
- latest_end: INTEGER (latest class end across all days, minutes from midnight)
- longest_gap: INTEGER (longest single gap between classes in minutes)
- total_class_time: INTEGER (total minutes spent in actual classes)
- schedule_span: INTEGER (time from first to last class: latest_end - earliest_start)

DAY PATTERN METRICS:
- consecutive_days: INTEGER (longest streak of consecutive class days)
- weekend_classes: BOOLEAN (1 if has Saturday/Sunday classes, 0 if not)
- weekday_only: BOOLEAN (1 if only Monday-Friday, 0 if has weekends)

TIME PREFERENCE FLAGS (BOOLEAN: 1=true, 0=false):
- has_early_morning: BOOLEAN (classes before 8:30 AM / 510 minutes)
- has_morning_classes: BOOLEAN (classes before 10:00 AM / 600 minutes)
- has_evening_classes: BOOLEAN (classes after 6:00 PM / 1080 minutes)
- has_late_evening: BOOLEAN (classes after 8:00 PM / 1200 minutes)

DAILY INTENSITY METRICS:
- max_daily_hours: INTEGER (most hours of classes in any single day)
- min_daily_hours: INTEGER (fewest hours on days that have classes)
- avg_daily_hours: INTEGER (average hours per study day)

GAP AND BREAK PATTERNS:
- has_lunch_break: BOOLEAN (has gap between 12:00-14:00 PM / 720-840 minutes)
- max_daily_gaps: INTEGER (maximum number of gaps in any single day)
- avg_gap_length: INTEGER (average gap length when gaps exist)

EFFICIENCY METRICS:
- compactness_ratio: REAL (total_class_time / schedule_span, higher = more efficient)

SPECIFIC WEEKDAY FLAGS (BOOLEAN: 1=true, 0=false):
- has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday
</comprehensive_column_reference>

<time_conversion_quick_reference>
Common time conversions (minutes from midnight):
- 7:00 AM = 420    - 8:00 AM = 480    - 8:30 AM = 510    - 9:00 AM = 540
- 10:00 AM = 600   - 12:00 PM = 720   - 2:00 PM = 840    - 5:00 PM = 1020
- 6:00 PM = 1080   - 8:00 PM = 1200   - 9:00 PM = 1260   - 10:00 PM = 1320
</time_conversion_quick_reference>

<user_query_examples>
EXAMPLE QUERIES AND THEIR SQL:

"Find schedules with no early morning classes"
→ SELECT schedule_index FROM schedule WHERE has_early_morning = 0

"Show me schedules that start after 9 AM"
→ SELECT schedule_index FROM schedule WHERE earliest_start > 540

"I want schedules with maximum 4 study days and no gaps"
→ SELECT schedule_index FROM schedule WHERE amount_days <= 4 AND amount_gaps = 0

"Find schedules ending before 5 PM"
→ SELECT schedule_index FROM schedule WHERE latest_end <= 1020

"Show schedules with classes only on weekdays"
→ SELECT schedule_index FROM schedule WHERE weekday_only = 1

"I want compact schedules with good efficiency"
→ SELECT schedule_index FROM schedule WHERE compactness_ratio > 0.6

"Find schedules with a lunch break"
→ SELECT schedule_index FROM schedule WHERE has_lunch_break = 1

"Show me schedules with no Friday classes"
→ SELECT schedule_index FROM schedule WHERE has_friday = 0

"I want schedules with consecutive days but not too many"
→ SELECT schedule_index FROM schedule WHERE consecutive_days >= 2 AND consecutive_days <= 4

"Find schedules with light daily workload"
→ SELECT schedule_index FROM schedule WHERE max_daily_hours <= 6 AND avg_daily_hours <= 4
</user_query_examples>

<instructions>
When a user asks to filter schedules, you MUST respond in this EXACT format:

RESPONSE: [Your helpful explanation of what you're filtering for]
SQL: [The SQL query to execute]
PARAMETERS: [Comma-separated parameter values, or NONE]

For non-filtering questions, respond normally and set SQL to NONE.

ALWAYS:
- SELECT schedule_index FROM schedule WHERE [conditions]
- Use ? for parameters, never hardcode values
- Use boolean columns efficiently (=1 for true, =0 for false)
- Combine multiple conditions with AND/OR as needed
- Consider user intent - "early" usually means has_early_morning or has_morning_classes
- For time ranges, use earliest_start/latest_end for global times, avg_start/avg_end for averages
</instructions>

<common_user_intents>
"early morning" → has_early_morning = 0 OR earliest_start > 540
"late evening" → has_evening_classes = 0 OR latest_end < 1080
"compact schedule" → compactness_ratio > 0.5 OR schedule_span < 480
"spread out" → consecutive_days <= 2 OR amount_days <= 3
"intensive days" → max_daily_hours >= 6
"light days" → max_daily_hours <= 4
"no gaps" → amount_gaps = 0
"minimal gaps" → amount_gaps <= 2
"weekdays only" → weekday_only = 1
"free weekends" → weekend_classes = 0
</common_user_intents>

Remember: You MUST follow the exact response format with RESPONSE:, SQL:, and PARAMETERS: labels.
)";

    return prompt;
}

BotQueryResponse ClaudeAPIClient::parseClaudeResponse(const string& responseData) {
    BotQueryResponse botResponse;

    if (responseData.empty()) {
        Logger::get().logError("Empty response from Claude API");
        botResponse.hasError = true;
        botResponse.errorMessage = "Empty response from Claude API";
        return botResponse;
    }

    try {
        Json::Reader reader;
        Json::Value root;

        if (!reader.parse(responseData, root)) {
            Logger::get().logError("Failed to parse Claude JSON: " + reader.getFormattedErrorMessages());
            botResponse.hasError = true;
            botResponse.errorMessage = "Invalid JSON response from Claude API";
            return botResponse;
        }

        if (root.isMember("error")) {
            Json::Value error = root["error"];
            string errorMessage = error.isMember("message") ? error["message"].asString() : "Unknown error";
            Logger::get().logError("Claude API error: " + errorMessage);
            botResponse.hasError = true;
            botResponse.errorMessage = errorMessage;
            return botResponse;
        }

        if (!root.isMember("content") || !root["content"].isArray() || root["content"].empty()) {
            Logger::get().logError("Invalid content structure in Claude response");
            botResponse.hasError = true;
            botResponse.errorMessage = "Invalid response format from Claude API";
            return botResponse;
        }

        Json::Value firstContent = root["content"][0];
        if (!firstContent.isMember("text")) {
            Logger::get().logError("No text content in Claude response");
            botResponse.hasError = true;
            botResponse.errorMessage = "No text content in Claude API response";
            return botResponse;
        }

        string contentText = firstContent["text"].asString();
        if (contentText.empty()) {
            Logger::get().logError("Empty text content from Claude");
            botResponse.hasError = true;
            botResponse.errorMessage = "Empty text content from Claude API";
            return botResponse;
        }

        // Parse SQL query (minimal logging)
        string sqlQuery;
        vector<string> parameters;
        bool hasSql = extractSQLQuery(contentText, sqlQuery, parameters);

        if (hasSql) {
            botResponse.isFilterQuery = true;
            botResponse.sqlQuery = sqlQuery;
            botResponse.queryParameters = parameters;
            Logger::get().logInfo("sqlQuery: " + sqlQuery);

            if (parameters.empty()) {
                Logger::get().logInfo("Query Parameters: None");
            } else {
                Logger::get().logInfo("Query Parameters (" + std::to_string(parameters.size()) + " total):");
                for (size_t i = 0; i < parameters.size(); ++i) {
                    Logger::get().logInfo("  [" + std::to_string(i) + "]: " + parameters[i]);
                }
            }
        } else {
            botResponse.isFilterQuery = false;
        }

        // Extract user message
        auto trim = [](string str) {
            str.erase(0, str.find_first_not_of(" \t\n\r"));
            str.erase(str.find_last_not_of(" \t\n\r") + 1);
            return str;
        };

        string lowerContent = contentText;
        transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
        size_t responsePos = lowerContent.find("response:");

        if (responsePos != string::npos) {
            size_t responseStartPos = responsePos + 9;
            size_t responseEndPos = lowerContent.find("sql:", responseStartPos);
            if (responseEndPos == string::npos) {
                responseEndPos = contentText.length();
            }
            string responseContent = contentText.substr(responseStartPos, responseEndPos - responseStartPos);
            botResponse.userMessage = trim(responseContent);
        } else {
            botResponse.userMessage = contentText;
        }

        if (botResponse.userMessage.empty()) {
            Logger::get().logError("Empty message extracted from Claude response");
            botResponse.hasError = true;
            botResponse.errorMessage = "Empty message extracted from Claude response";
            return botResponse;
        }

    } catch (const exception& e) {
        Logger::get().logError("Exception parsing Claude response: " + string(e.what()));
        botResponse.hasError = true;
        botResponse.errorMessage = "Failed to parse Claude response: " + string(e.what());
    }

    return botResponse;
}

bool ClaudeAPIClient::extractSQLQuery(const string& content, string& sqlQuery, vector<string>& parameters) {
    auto trim = [](string str) {
        str.erase(0, str.find_first_not_of(" \t\n\r"));
        str.erase(str.find_last_not_of(" \t\n\r") + 1);
        return str;
    };

    string lowerContent = content;
    transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);

    size_t sqlPos = lowerContent.find("sql:");
    if (sqlPos == string::npos) {
        return false;
    }

    size_t sqlStartPos = sqlPos + 4;
    size_t sqlEndPos = lowerContent.find("parameters:", sqlStartPos);
    if (sqlEndPos == string::npos) {
        sqlEndPos = content.length();
    }

    string rawSql = content.substr(sqlStartPos, sqlEndPos - sqlStartPos);
    rawSql = trim(rawSql);

    string lowerSql = rawSql;
    transform(lowerSql.begin(), lowerSql.end(), lowerSql.begin(), ::tolower);
    if (lowerSql == "none" || rawSql.empty()) {
        return false;
    }

    sqlQuery = rawSql;

    // Extract parameters
    parameters.clear();
    size_t paramPos = lowerContent.find("parameters:");
    if (paramPos != string::npos) {
        size_t paramStartPos = paramPos + 11;
        size_t paramEndPos = content.find('\n', paramStartPos);
        if (paramEndPos == string::npos) {
            paramEndPos = content.length();
        }

        string rawParams = content.substr(paramStartPos, paramEndPos - paramStartPos);
        rawParams = trim(rawParams);

        string lowerParams = rawParams;
        transform(lowerParams.begin(), lowerParams.end(), lowerParams.begin(), ::tolower);
        if (lowerParams != "none" && !rawParams.empty()) {
            stringstream ss(rawParams);
            string param;
            while (getline(ss, param, ',')) {
                param = trim(param);
                if (!param.empty()) {
                    parameters.push_back(param);
                }
            }
        }
    }

    return true;
}