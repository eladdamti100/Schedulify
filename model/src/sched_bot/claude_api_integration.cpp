#include "claude_api_integration.h"
#include <algorithm>
#include <cctype>
#include <cstring>

struct APIResponse {
    string data;
    long response_code;
    bool success;

    APIResponse() : response_code(0), success(false) {}
};

namespace {
string extractWhereClause(string sql) {
    string lower = sql;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    size_t w = lower.find(" where ");
    if (w == string::npos) return "";
    size_t start = w + 7;
    size_t end = lower.find(" order by ", start);
    if (end == string::npos) end = sql.size();
    string where = sql.substr(start, end - start);
    where.erase(0, where.find_first_not_of(" \t\r\n"));
    where.erase(where.find_last_not_of(" \t\r\n") + 1);
    return where;
}

void substituteParameters(string& where, const vector<string>& params) {
    string result;
    result.reserve(where.size() + 64);
    size_t paramIndex = 0;
    for (size_t i = 0; i < where.size(); ++i) {
        if (where[i] == '?' && paramIndex < params.size()) {
            const string& p = params[paramIndex++];
            bool quote = (p.size() >= 2 && ((p.front() == '\'' && p.back() == '\'') || (p.front() == '"' && p.back() == '"')));
            result += quote ? p.substr(1, p.size() - 2) : p;
        } else {
            result += where[i];
        }
    }
    where = std::move(result);
}

bool parseCondition(const string& cond, string& column, string& op, string& valueStr) {
    string c = cond;
    c.erase(0, c.find_first_not_of(" \t"));
    c.erase(c.find_last_not_of(" \t") + 1);
    if (c.empty()) return false;
    const char* ops[] = { ">=", "<=", "!=", "=", ">", "<" };
    size_t opPos = string::npos;
    size_t opLen = 0;
    for (const char* o : ops) {
        size_t pos = c.find(o);
        if (pos != string::npos && (opPos == string::npos || pos < opPos)) {
            opPos = pos;
            opLen = strlen(o);
        }
    }
    if (opPos == string::npos) return false;
    column = c.substr(0, opPos);
    op = c.substr(opPos, opLen);
    valueStr = c.substr(opPos + opLen);
    auto trim = [](string& s) {
        s.erase(0, s.find_first_not_of(" \t'\""));
    };
    auto trimEnd = [](string& s) {
        size_t last = s.find_last_not_of(" \t'\"");
        if (last != string::npos) s.erase(last + 1); else s.clear();
    };
    trim(column);
    trimEnd(column);
    trim(valueStr);
    trimEnd(valueStr);
    return true;
}

double getMetricValue(const ScheduleFilterMetrics& m, const string& column) {
    if (column == "amount_days") return m.amount_days;
    if (column == "amount_gaps") return m.amount_gaps;
    if (column == "gaps_time") return m.gaps_time;
    if (column == "avg_start") return m.avg_start;
    if (column == "avg_end") return m.avg_end;
    if (column == "earliest_start") return m.earliest_start;
    if (column == "latest_end") return m.latest_end;
    if (column == "longest_gap") return m.longest_gap;
    if (column == "total_class_time") return m.total_class_time;
    if (column == "consecutive_days") return m.consecutive_days;
    if (column == "weekend_classes") return m.weekend_classes ? 1 : 0;
    if (column == "has_morning_classes") return m.has_morning_classes ? 1 : 0;
    if (column == "has_early_morning") return m.has_early_morning ? 1 : 0;
    if (column == "has_evening_classes") return m.has_evening_classes ? 1 : 0;
    if (column == "has_late_evening") return m.has_late_evening ? 1 : 0;
    if (column == "max_daily_hours") return m.max_daily_hours;
    if (column == "min_daily_hours") return m.min_daily_hours;
    if (column == "avg_daily_hours") return m.avg_daily_hours;
    if (column == "has_lunch_break") return m.has_lunch_break ? 1 : 0;
    if (column == "max_daily_gaps") return m.max_daily_gaps;
    if (column == "avg_gap_length") return m.avg_gap_length;
    if (column == "schedule_span") return m.schedule_span;
    if (column == "compactness_ratio") return m.compactness_ratio;
    if (column == "weekday_only") return m.weekday_only ? 1 : 0;
    if (column == "has_monday") return m.has_monday ? 1 : 0;
    if (column == "has_tuesday") return m.has_tuesday ? 1 : 0;
    if (column == "has_wednesday") return m.has_wednesday ? 1 : 0;
    if (column == "has_thursday") return m.has_thursday ? 1 : 0;
    if (column == "has_friday") return m.has_friday ? 1 : 0;
    if (column == "has_saturday") return m.has_saturday ? 1 : 0;
    if (column == "has_sunday") return m.has_sunday ? 1 : 0;
    return 0;
}

bool evaluateCondition(const ScheduleFilterMetrics& m, const string& column, const string& op, const string& valueStr, const string& /*semesterFilter*/) {
    if (column == "semester") {
        string v = valueStr;
        if (v.size() >= 2 && (v.front() == '\'' || v.front() == '"')) v = v.substr(1, v.size() - 2);
        bool match = (m.semester == v);
        if (op == "=") return match;
        if (op == "!=") return !match;
        return false;
    }
    double lhs = getMetricValue(m, column);
    double rhs = 0;
    try { rhs = std::stod(valueStr); } catch (...) { return false; }
    if (op == "=") return lhs == rhs;
    if (op == "!=") return lhs != rhs;
    if (op == ">") return lhs > rhs;
    if (op == ">=") return lhs >= rhs;
    if (op == "<") return lhs < rhs;
    if (op == "<=") return lhs <= rhs;
    return false;
}

vector<string> filterSchedulesInMemory(const vector<ScheduleFilterMetrics>& metrics,
                                       const string& sqlQuery, const vector<string>& queryParameters,
                                       const string& semester) {
    vector<string> result;
    string where = extractWhereClause(sqlQuery);
    if (where.empty()) {
        for (const auto& m : metrics) {
            if (m.semester == semester) result.push_back(m.unique_id);
        }
        return result;
    }
    substituteParameters(where, queryParameters);
    vector<string> conditions;
    string lowerWhere = where;
    std::transform(lowerWhere.begin(), lowerWhere.end(), lowerWhere.begin(), ::tolower);
    for (size_t pos = 0; ; ) {
        size_t next = lowerWhere.find(" and ", pos);
        string cond = (next == string::npos) ? where.substr(pos) : where.substr(pos, next - pos);
        conditions.push_back(cond);
        if (next == string::npos) break;
        pos = next + 5;
    }
    for (const auto& m : metrics) {
        if (m.semester != semester) continue;
        bool match = true;
        for (const string& cond : conditions) {
            string col, op, val;
            if (!parseCondition(cond, col, op, val)) continue;
            if (!evaluateCondition(m, col, op, val, semester)) {
                match = false;
                break;
            }
        }
        if (match) result.push_back(m.unique_id);
    }
    return result;
}
} // namespace

BotQueryResponse ClaudeAPIClient::ActivateBot(const BotQueryRequest& request) {
    BotQueryResponse response;

    try {
        Logger::get().logInfo("ActivateBot: Processing bot query for semester: " + request.semester);

        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("ActivateBot: Database not connected");
            response.hasError = true;
            response.errorMessage = "Database connection unavailable";
            return response;
        }

        // Create enhanced request with semester-specific metadata
        BotQueryRequest enhancedRequest = request;
        enhancedRequest.scheduleMetadata = db.schedules()->getSchedulesMetadataForBot();

        // Add semester info to metadata
        enhancedRequest.scheduleMetadata += "\n\nCURRENT SEMESTER FILTER: " + request.semester;
        enhancedRequest.scheduleMetadata += "\nNOTE: Only schedules from semester " + request.semester + " are available for filtering.";
        enhancedRequest.scheduleMetadata += "\nIMPORTANT: Always SELECT unique_id FROM schedule for filtering, not schedule_index.";

        // Try Claude API
        ClaudeAPIClient claudeClient;
        response = claudeClient.processScheduleQuery(enhancedRequest);

        // Handle rate limiting with fallback
        if (response.hasError &&
            (response.errorMessage.find("overloaded") != std::string::npos ||
             response.errorMessage.find("rate limit") != std::string::npos ||
             response.errorMessage.find("429") != std::string::npos ||
             response.errorMessage.find("529") != std::string::npos)) {

            Logger::get().logWarning("ActivateBot: Claude API overloaded - using fallback");
            response = generateFallbackResponse(enhancedRequest);

            if (!response.hasError) {
                response.userMessage = "⚠️ Claude API is currently busy, using simplified pattern matching.\n\n" + response.userMessage;
            }
        }

        if (response.hasError) {
            Logger::get().logError("ActivateBot: Claude processing failed: " + response.errorMessage);
            return response;
        }

        // Execute SQL filter: use in-memory when we have view metrics (schedules in UI), else DB
        if (response.isFilterQuery && !response.sqlQuery.empty()) {
            vector<string> filteredUniqueIds;

            if (!request.viewScheduleMetrics.empty()) {
                // Filter in memory over the schedules currently in the view (no DB)
                Logger::get().logInfo("ActivateBot: Filtering in memory over " +
                                      std::to_string(request.viewScheduleMetrics.size()) + " schedules in view");
                filteredUniqueIds = filterSchedulesInMemory(request.viewScheduleMetrics,
                                                           response.sqlQuery,
                                                           response.queryParameters,
                                                           request.semester);
                for (const string& uid : filteredUniqueIds) {
                    for (size_t i = 0; i < request.viewScheduleMetrics.size(); ++i) {
                        if (request.viewScheduleMetrics[i].unique_id == uid) {
                            response.filteredScheduleIds.push_back(request.availableScheduleIds[i]);
                            break;
                        }
                    }
                }
            } else {
                // DB path when view metrics not provided
                SQLValidator::ValidationResult validation = SQLValidator::validateScheduleQuery(response.sqlQuery);
                if (!validation.isValid) {
                    Logger::get().logError("ActivateBot: Generated query failed validation: " + validation.errorMessage);
                    response.hasError = true;
                    response.errorMessage = "Generated query failed security validation: " + validation.errorMessage;
                    return response;
                }

                string semesterFilteredQuery = response.sqlQuery;
                size_t pos = semesterFilteredQuery.find("schedule_index");
                if (pos != string::npos) {
                    semesterFilteredQuery.replace(pos, 14, "unique_id");
                }
                string lowerQuery = semesterFilteredQuery;
                std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
                if (lowerQuery.find("where") != string::npos) {
                    semesterFilteredQuery += " AND semester = ?";
                } else {
                    semesterFilteredQuery += " WHERE semester = ?";
                }

                vector<string> enhancedParameters = response.queryParameters;
                enhancedParameters.push_back(request.semester);

                Logger::get().logInfo("Executing semester-filtered query: " + semesterFilteredQuery);
                vector<string> matchingUniqueIds = db.schedules()->executeCustomQueryForUniqueIds(semesterFilteredQuery, enhancedParameters);

                vector<string> availableUniqueIds;
                for (int scheduleIndex : request.availableScheduleIds) {
                    string uniqueId = db.schedules()->getUniqueIdByScheduleIndex(scheduleIndex, request.semester);
                    if (!uniqueId.empty()) {
                        availableUniqueIds.push_back(uniqueId);
                    }
                }

                std::set<string> availableSet(availableUniqueIds.begin(), availableUniqueIds.end());
                for (const string& uniqueId : matchingUniqueIds) {
                    if (availableSet.find(uniqueId) != availableSet.end()) {
                        filteredUniqueIds.push_back(uniqueId);
                    }
                }

                vector<int> filteredIds = db.schedules()->getScheduleIndicesByUniqueIds(filteredUniqueIds);
                response.filteredScheduleIds = filteredIds;
            }

            response.filteredUniqueIds = filteredUniqueIds;

            if (filteredUniqueIds.empty()) {
                response.userMessage += "\n\n❌ No schedules match your criteria in semester " + request.semester + ".";
            } else {
                response.userMessage += "\n\n✅ Found " + std::to_string(filteredUniqueIds.size()) +
                                        " matching schedules in semester " + request.semester + ".";
            }
        }

        Logger::get().logInfo("ActivateBot: Successfully processed query for semester " + request.semester);

    } catch (const std::exception& e) {
        Logger::get().logError("ActivateBot: Exception processing query: " + std::string(e.what()));
        response.hasError = true;
        response.errorMessage = "An error occurred while processing your query: " + std::string(e.what());
        response.isFilterQuery = false;
    }

    return response;
}

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

    // Simple pattern matching for common queries - UPDATED to use unique_id
    if (userMsg.find("start after") != string::npos || userMsg.find("begin after") != string::npos) {
        if (userMsg.find("10") != string::npos) {
            response.userMessage = "I understand you want schedules that start after 10:00 AM. I'll look for schedules where the earliest class begins after 10:00 AM.";
            response.sqlQuery = "SELECT unique_id FROM schedule WHERE earliest_start > ?";
            response.queryParameters = {"600"}; // 10:00 AM = 600 minutes
            response.isFilterQuery = true;
        } else if (userMsg.find("9") != string::npos) {
            response.userMessage = "I'll find schedules that start after 9:00 AM for you.";
            response.sqlQuery = "SELECT unique_id FROM schedule WHERE earliest_start > ?";
            response.queryParameters = {"540"}; // 9:00 AM = 540 minutes
            response.isFilterQuery = true;
        }
    } else if (userMsg.find("no early") != string::npos || userMsg.find("not early") != string::npos) {
        response.userMessage = "I'll find schedules with no early morning classes (before 8:30 AM).";
        response.sqlQuery = "SELECT unique_id FROM schedule WHERE has_early_morning = ?";
        response.queryParameters = {"0"};
        response.isFilterQuery = true;
    } else if (userMsg.find("no morning") != string::npos) {
        response.userMessage = "I'll find schedules with no morning classes (before 10:00 AM).";
        response.sqlQuery = "SELECT unique_id FROM schedule WHERE has_morning_classes = ?";
        response.queryParameters = {"0"};
        response.isFilterQuery = true;
    } else if ((userMsg.find("max") != string::npos || userMsg.find("maximum") != string::npos) &&
               (userMsg.find("day") != string::npos || userMsg.find("days") != string::npos)) {
        // "max 2 days learning" -> amount_days <= 2
        for (int n = 1; n <= 7; ++n) {
            if (userMsg.find(std::to_string(n)) != string::npos) {
                response.userMessage = "I'll find schedules with at most " + std::to_string(n) + " study days.";
                response.sqlQuery = "SELECT unique_id FROM schedule WHERE amount_days <= ?";
                response.queryParameters = {std::to_string(n)};
                response.isFilterQuery = true;
                break;
            }
        }
        if (!response.isFilterQuery) {
            response.userMessage = "I'll find schedules with a limited number of study days.";
            response.sqlQuery = "SELECT unique_id FROM schedule WHERE amount_days <= 4";
            response.queryParameters = {};
            response.isFilterQuery = true;
        }
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

CRITICAL: Always use unique_id for filtering, NOT schedule_index!

BASIC METRICS:
- unique_id: TEXT (unique identifier for each schedule - USE THIS FOR FILTERING)
- schedule_index: INTEGER (display number only - DO NOT USE for filtering)
- semester: TEXT (A, B, or SUMMER)
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
EXAMPLE QUERIES AND THEIR SQL (ALWAYS USE unique_id!):

"Find schedules with no early morning classes"
→ SELECT unique_id FROM schedule WHERE has_early_morning = 0

"Show me schedules that start after 9 AM"
→ SELECT unique_id FROM schedule WHERE earliest_start > 540

"I want schedules with maximum 4 study days and no gaps"
→ SELECT unique_id FROM schedule WHERE amount_days <= 4 AND amount_gaps = 0

"Find schedules ending before 5 PM"
→ SELECT unique_id FROM schedule WHERE latest_end <= 1020

"Show schedules with classes only on weekdays"
→ SELECT unique_id FROM schedule WHERE weekday_only = 1

"I want compact schedules with good efficiency"
→ SELECT unique_id FROM schedule WHERE compactness_ratio > 0.6

"Find schedules with a lunch break"
→ SELECT unique_id FROM schedule WHERE has_lunch_break = 1

"Show me schedules with no Friday classes"
→ SELECT unique_id FROM schedule WHERE has_friday = 0

"I want schedules with consecutive days but not too many"
→ SELECT unique_id FROM schedule WHERE consecutive_days >= 2 AND consecutive_days <= 4

"Find schedules with light daily workload"
→ SELECT unique_id FROM schedule WHERE max_daily_hours <= 6 AND avg_daily_hours <= 4
</user_query_examples>

<instructions>
When a user asks to filter schedules, you MUST respond in this EXACT format:

RESPONSE: [Your helpful explanation of what you're filtering for]
SQL: [The SQL query to execute]
PARAMETERS: [Comma-separated parameter values, or NONE]

For non-filtering questions, respond normally and set SQL to NONE.

CRITICAL RULES:
- ALWAYS SELECT unique_id FROM schedule WHERE [conditions]
- NEVER use schedule_index in SELECT statements
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
CRITICAL: Always use unique_id in SELECT statements, never schedule_index!
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