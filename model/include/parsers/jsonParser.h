#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "model_interfaces.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

class JsonParser {
private:
    // Helper method to escape special characters in JSON strings
    static std::string escapeJsonString(const std::string& input);

    // Helper method to convert a single ScheduleItem to JSON
    static std::string scheduleItemToJson(const ScheduleItem& item, int indentLevel = 0);

    // Helper method to convert a vector of ScheduleItems to JSON array
    static std::string scheduleItemsToJsonArray(const std::vector<ScheduleItem>& items, int indentLevel = 0);

    // Helper method to convert a single ScheduleDay to JSON
    static std::string scheduleDayToJson(const ScheduleDay& day, int indentLevel = 0);

    // Helper method to convert a vector of ScheduleDays to JSON array
    static std::string scheduleDaysToJsonArray(const std::vector<ScheduleDay>& days, int indentLevel = 0);

    // Helper method to convert a single InformativeSchedule to JSON (formatted)
    static std::string informativeScheduleToJson(const InformativeSchedule& schedule, int indentLevel = 0);

    // Helper method to convert a single InformativeSchedule to compact JSON (for JSONL)
    static std::string informativeScheduleToJsonCompact(const InformativeSchedule& schedule);

    // Helper method to write JSON string to file
    static bool writeJsonToFile(const std::string& jsonContent, const std::string& filename);

    // Helper method to write JSONL string to file
    static bool writeJsonlToFile(const std::string& jsonlContent, const std::string& filename);

public:
    JsonParser() = default;

    ~JsonParser() = default;

    JsonParser(const JsonParser&) = delete;

    JsonParser& operator=(const JsonParser&) = delete;

    JsonParser(JsonParser&&) = default;

    JsonParser& operator=(JsonParser&&) = default;

    // Main method to convert vector of InformativeSchedule to JSON file
    static bool ConvertToJson(const std::vector<InformativeSchedule>& schedules,
                              const std::string& outputFilename = "schedules.json");

    // Main method to convert vector of InformativeSchedule to JSONL file
    static bool ConvertToJsonl(const std::vector<InformativeSchedule>& schedules,
                               const std::string& outputFilename = "schedules.jsonl");

    // Main method that returns JSON as string
    static std::string ConvertToJsonString(const std::vector<InformativeSchedule>& schedules);

    // Main method that returns JSONL as string
    static std::string ConvertToJsonlString(const std::vector<InformativeSchedule>& schedules);
};

#endif // JSON_PARSER_H