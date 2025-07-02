#include "jsonParser.h"

string JsonParser::escapeJsonString(const string& input) {
    string result;
    for (size_t i = 0; i < input.length(); ++i) {
        unsigned char c = input[i];
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                // Handle UTF-8 encoded characters (including Hebrew)
                if (c < 32 || c == 127) {
                    // Control characters - escape as unicode
                    char buffer[7];
                    sprintf(buffer, "\\u%04x", c);
                    result += buffer;
                } else {
                    // Regular characters and UTF-8 sequences - keep as is
                    result += c;
                }
                break;
        }
    }
    return result;
}

string JsonParser::scheduleItemToJson(const ScheduleItem& item, int indentLevel) {
    string indent = string(indentLevel * 2, ' ');
    string nextIndent = string((indentLevel + 1) * 2, ' ');

    stringstream json;
    json << indent << "{\n";
    json << nextIndent << R"("courseName": ")" << escapeJsonString(item.courseName) << "\",\n";
    json << nextIndent << R"("raw_id": ")" << escapeJsonString(item.raw_id) << "\",\n";
    json << nextIndent << R"("type": ")" << escapeJsonString(item.type) << "\",\n";
    json << nextIndent << R"("start": ")" << escapeJsonString(item.start) << "\",\n";
    json << nextIndent << R"("end": ")" << escapeJsonString(item.end) << "\",\n";
    json << nextIndent << R"("building": ")" << escapeJsonString(item.building) << "\",\n";
    json << nextIndent << R"("room": ")" << escapeJsonString(item.room) << "\"\n";
    json << indent << "}";

    return json.str();
}

string JsonParser::scheduleItemsToJsonArray(const vector<ScheduleItem>& items, int indentLevel) {
    string indent = string(indentLevel * 2, ' ');
    string nextIndent = string((indentLevel + 1) * 2, ' ');

    stringstream json;
    json << "[\n";

    for (size_t i = 0; i < items.size(); ++i) {
        json << nextIndent << scheduleItemToJson(items[i], indentLevel + 1);
        if (i < items.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << indent << "]";
    return json.str();
}

string JsonParser::scheduleDayToJson(const ScheduleDay& day, int indentLevel) {
    string indent = string(indentLevel * 2, ' ');
    string nextIndent = string((indentLevel + 1) * 2, ' ');

    stringstream json;
    json << indent << "{\n";
    json << nextIndent << R"("day": ")" << escapeJsonString(day.day) << "\",\n";
    json << nextIndent << "\"day_items\": " << scheduleItemsToJsonArray(day.day_items, indentLevel + 1) << "\n";
    json << indent << "}";

    return json.str();
}

string JsonParser::scheduleDaysToJsonArray(const vector<ScheduleDay>& days, int indentLevel) {
    string indent = string(indentLevel * 2, ' ');
    string nextIndent = string((indentLevel + 1) * 2, ' ');

    stringstream json;
    json << "[\n";

    for (size_t i = 0; i < days.size(); ++i) {
        json << nextIndent << scheduleDayToJson(days[i], indentLevel + 1);
        if (i < days.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << indent << "]";
    return json.str();
}

string JsonParser::informativeScheduleToJsonCompact(const InformativeSchedule& schedule) {
    stringstream json;
    json << "{";
    json << "\"index\":" << schedule.index + 1 << ",";
    json << "\"amount_days\":" << schedule.amount_days << ",";
    json << "\"amount_gaps\":" << schedule.amount_gaps << ",";
    json << "\"gaps_time\":" << schedule.gaps_time << ",";
    json << "\"avg_start\":" << schedule.avg_start << ",";
    json << "\"avg_end\":" << schedule.avg_end << ",";
    json << "\"week\":[";

    for (size_t i = 0; i < schedule.week.size(); ++i) {
        const ScheduleDay& day = schedule.week[i];
        json << "{";
        json << "\"day\":\"" << escapeJsonString(day.day) << "\",";
        json << "\"day_items\":[";

        for (size_t j = 0; j < day.day_items.size(); ++j) {
            const ScheduleItem& item = day.day_items[j];
            json << "{";
            json << "\"courseName\":\"" << escapeJsonString(item.courseName) << "\",";
            json << "\"raw_id\":\"" << escapeJsonString(item.raw_id) << "\",";
            json << "\"type\":\"" << escapeJsonString(item.type) << "\",";
            json << "\"start\":\"" << escapeJsonString(item.start) << "\",";
            json << "\"end\":\"" << escapeJsonString(item.end) << "\",";
            json << "\"building\":\"" << escapeJsonString(item.building) << "\",";
            json << "\"room\":\"" << escapeJsonString(item.room) << "\"";
            json << "}";
            if (j < day.day_items.size() - 1) json << ",";
        }

        json << "]}";
        if (i < schedule.week.size() - 1) json << ",";
    }

    json << "]}";
    return json.str();
}

string JsonParser::informativeScheduleToJson(const InformativeSchedule& schedule, int indentLevel) {
    string indent = string(indentLevel * 2, ' ');
    string nextIndent = string((indentLevel + 1) * 2, ' ');

    stringstream json;
    json << indent << "{\n";
    json << nextIndent << "\"index\": " << schedule.index + 1 << ",\n";
    json << nextIndent << "\"amount_days\": " << schedule.amount_days << ",\n";
    json << nextIndent << "\"amount_gaps\": " << schedule.amount_gaps << ",\n";
    json << nextIndent << "\"gaps_time\": " << schedule.gaps_time << ",\n";
    json << nextIndent << "\"avg_start\": " << schedule.avg_start << ",\n";
    json << nextIndent << "\"avg_end\": " << schedule.avg_end << ",\n";
    json << nextIndent << "\"week\": " << scheduleDaysToJsonArray(schedule.week, indentLevel + 1) << "\n";
    json << indent << "}";

    return json.str();
}

bool JsonParser::writeJsonlToFile(const string& jsonlContent, const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file " << filename << " for writing." << endl;
        return false;
    }

    // Write UTF-8 BOM (Byte Order Mark) to ensure proper encoding recognition
    file.write("\xEF\xBB\xBF", 3);

    // Write the JSONL content
    file << jsonlContent;
    file.close();

    if (file.fail()) {
        cerr << "Error: Failed to write to file " << filename << endl;
        return false;
    }

    return true;
}

bool JsonParser::writeJsonToFile(const string& jsonContent, const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file " << filename << " for writing." << endl;
        return false;
    }

    // Write UTF-8 BOM (Byte Order Mark) to ensure proper encoding recognition
    file.write("\xEF\xBB\xBF", 3);

    // Write the JSON content
    file << jsonContent;
    file.close();

    if (file.fail()) {
        cerr << "Error: Failed to write to file " << filename << endl;
        return false;
    }

    return true;
}

bool JsonParser::ConvertToJsonl(const vector<InformativeSchedule>& schedules, const string& outputFilename) {
    try {
        stringstream jsonlContent;

        // Each schedule becomes one line in the JSONL file
        for (size_t i = 0; i < schedules.size(); ++i) {
            jsonlContent << informativeScheduleToJsonCompact(schedules[i]) << "\n";
        }

        // Write to file
        bool success = writeJsonlToFile(jsonlContent.str(), outputFilename);

        if (success) {
            cout << "Successfully converted " << schedules.size()
                 << " schedules to JSONL file: " << outputFilename << endl;
        }

        return success;

    } catch (const exception& e) {
        cerr << "Error during JSONL conversion: " << e.what() << endl;
        return false;
    }
}

string JsonParser::ConvertToJsonlString(const vector<InformativeSchedule>& schedules) {
    try {
        stringstream jsonlContent;

        // Each schedule becomes one line in the JSONL string
        for (size_t i = 0; i < schedules.size(); ++i) {
            jsonlContent << informativeScheduleToJsonCompact(schedules[i]) << "\n";
        }

        return jsonlContent.str();

    } catch (const exception& e) {
        cerr << "Error during JSONL string conversion: " << e.what() << endl;
        return "";
    }
}

bool JsonParser::ConvertToJson(const vector<InformativeSchedule>& schedules, const string& outputFilename) {
    try {
        stringstream jsonContent;
        jsonContent << "{\n";
        jsonContent << "  \"schedules\": [\n";

        for (size_t i = 0; i < schedules.size(); ++i) {
            jsonContent << "    " << informativeScheduleToJson(schedules[i], 2);
            if (i < schedules.size() - 1) {
                jsonContent << ",";
            }
            jsonContent << "\n";
        }

        jsonContent << "  ]\n";
        jsonContent << "}";

        // Write to file
        bool success = writeJsonToFile(jsonContent.str(), outputFilename);

        if (success) {
            cout << "Successfully converted " << schedules.size()
                 << " schedules to JSON file: " << outputFilename << endl;
        }

        return success;

    } catch (const exception& e) {
        cerr << "Error during JSON conversion: " << e.what() << endl;
        return false;
    }
}
