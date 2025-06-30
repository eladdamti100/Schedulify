#ifndef EXCEL_PARSER_H
#define EXCEL_PARSER_H

// Include OpenXLSX first to avoid conflicts
#include <OpenXLSX.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <regex>
#include <unordered_map>
#include <sstream>
#include <algorithm>

// Only include Windows headers if absolutely necessary and use guards
#ifdef _WIN32
// Avoid conflicts by defining these before including Windows headers
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Avoid the std::byte conflict
#define byte win_byte_override
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <locale>
#undef byte
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif

#include "model_interfaces.h"

using namespace std;
using namespace OpenXLSX;

class ExcelCourseParser {
private:
    map<string, int> dayMap;
    map<string, SessionType> sessionTypeMap;

    // Helper method to determine semester number from Hebrew period string
    int getSemesterNumber(const string& period);

    // Parse multiple rooms from single cell
    vector<string> parseMultipleRooms(const string& roomStr);

    // Parse multiple time slots and match each with corresponding room
    vector<Session> parseMultipleSessions(const string& timeSlotStr, const string& roomStr, const string& teacher);

    // Parse a single session from time slot and room strings
    Session parseSingleSession(const string& timeSlotStr, const string& roomStr, const string& teacher);

    // Get session type from Hebrew string
    SessionType getSessionType(const string& hebrewType);

    // Parse course code from full code string
    pair<string, string> parseCourseCode(const string& fullCode);

public:
    ExcelCourseParser();

    // Main parsing method
    vector<Course> parseExcelFile(const string& filename);
};

// Utility function to get Hebrew day name
string getDayName(int dayOfWeek);

#endif // EXCEL_PARSER_H