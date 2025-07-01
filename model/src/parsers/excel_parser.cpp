#include "excel_parser.h"

ExcelCourseParser::ExcelCourseParser() {
    dayMap = {
            {"א", 1},
            {"ב", 2},
            {"ג", 3},
            {"ד", 4},
            {"ה", 5},
            {"ו", 6},
            {"ש", 7}
    };

    initializeSessionTypeMap();
}

// Main parsing method

vector<Course> ExcelCourseParser::parseExcelFile(const string& filename) {
    vector<Course> courses;
    map<string, Course> courseMap; // Key: courseKey (courseId_s{semester})
    map<string, map<string, Group>> courseGroupMap; // Key: courseKey, Value: groupKey -> Group

    stats = ParsingStats(); // Reset statistics

    try {
        XLDocument doc;
        doc.open(filename);
        auto worksheet = doc.workbook().worksheet(1);

        for (uint32_t row = 2; row <= 10000; ++row) {
            stats.totalRows++;

            auto firstCell = worksheet.cell(row, 1);
            if (firstCell.value().type() == XLValueType::Empty) {
                break;
            }

            vector<string> rowData;
            for (uint32_t col = 1; col <= 10; ++col) {
                auto cell = worksheet.cell(row, col);
                string cellValue;
                try {
                    if (cell.value().type() != XLValueType::Empty) {
                        cellValue = cell.value().get<string>();
                    } else {
                        cellValue = "";
                    }
                } catch (...) {
                    cellValue = "";
                }
                rowData.push_back(cellValue);
            }

            while (rowData.size() < 10) {
                rowData.push_back("");
            }

            string period = rowData[0];
            string fullCode = rowData[1];
            string courseName = rowData[2];
            string sessionType = rowData[3];
            string timeSlot = rowData[4];
            string creditPoints = rowData[5];
            string hours = rowData[6];
            string teachers = rowData[7];
            string room = rowData[8];
            string notes = rowData[9];

            // Enhanced semester parsing
            int semesterNumber = getSemesterNumber(period);
            if (!isValidSemester(semesterNumber)) {
                stats.skippedRows++;
                parsingWarnings.push_back("Row " + to_string(row) + ": Invalid semester - " + period);
                continue;
            }

            auto [courseCode, groupCode] = parseCourseCode(fullCode);
            if (courseCode.empty() || courseCode.length() != 5) {
                stats.skippedRows++;
                parsingWarnings.push_back("Row " + to_string(row) + ": Invalid course code - " + fullCode);
                continue;
            }

            SessionType normalizedSessionType = getSessionType(sessionType);
            if (normalizedSessionType == SessionType::UNSUPPORTED) {
                stats.skippedRows++;
                parsingWarnings.push_back("Row " + to_string(row) + ": Unsupported session type - " + sessionType);
                continue;
            }

            // Enhanced session type name mapping
            string normalizedSessionTypeName = sessionTypeToString(normalizedSessionType);

            if (timeSlot.empty() || timeSlot.find("'") == string::npos) {
                stats.skippedRows++;
                parsingWarnings.push_back("Row " + to_string(row) + ": Invalid time slot - " + timeSlot);
                continue;
            }

            vector<Session> sessions = parseMultipleSessions(timeSlot, room, teachers);

            bool hasValidSessions = false;
            for (const Session &session : sessions) {
                if (session.day_of_week > 0 && !session.start_time.empty() && !session.end_time.empty()) {
                    hasValidSessions = true;
                    break;
                }
            }

            if (!hasValidSessions) {
                stats.skippedRows++;
                parsingWarnings.push_back("Row " + to_string(row) + ": No valid sessions found");
                continue;
            }

            // Create course key with semester
            int courseId = 0;
            try {
                courseId = stoi(courseCode);
            } catch (...) {
                stats.skippedRows++;
                parsingWarnings.push_back("Row " + to_string(row) + ": Invalid course ID - " + courseCode);
                continue;
            }

            string courseKey = to_string(courseId) + "_s" + to_string(semesterNumber);

            // Initialize course if not exists
            if (courseMap.find(courseKey) == courseMap.end()) {
                Course newCourse;
                initializeCourse(newCourse, courseId, courseCode, courseName, teachers, semesterNumber);
                courseMap[courseKey] = newCourse;
                stats.coursesBySemester[semesterNumber]++;
            }

            // Track session type statistics
            stats.sessionTypesCounted[normalizedSessionTypeName]++;

            // Group key for this specific course-semester combination
            string groupKey = fullCode + "_" + normalizedSessionTypeName;

            if (courseGroupMap[courseKey].find(groupKey) == courseGroupMap[courseKey].end()) {
                Group newGroup;
                newGroup.type = normalizedSessionType;
                courseGroupMap[courseKey][groupKey] = newGroup;
            }

            for (const Session &session : sessions) {
                if (session.day_of_week > 0 && !session.start_time.empty() && !session.end_time.empty()) {
                    courseGroupMap[courseKey][groupKey].sessions.push_back(session);
                }
            }
        }

        // Create final courses list with enhanced group assignment
        for (auto &[courseKey, course] : courseMap) {
            for (auto &[groupKey, group] : courseGroupMap[courseKey]) {
                if (group.sessions.empty()) continue;

                // Enhanced group type assignment
                switch (group.type) {
                    case SessionType::LECTURE:
                        course.Lectures.push_back(group);
                        break;
                    case SessionType::TUTORIAL:
                        course.Tirgulim.push_back(group);
                        break;
                    case SessionType::LAB:
                        course.labs.push_back(group);
                        break;
                    case SessionType::BLOCK:
                        course.blocks.push_back(group);
                        break;
                    case SessionType::DEPARTMENTAL_SESSION:
                        course.DepartmentalSessions.push_back(group);
                        break;
                    case SessionType::REINFORCEMENT:
                        course.Reinforcements.push_back(group);
                        break;
                    case SessionType::GUIDANCE:
                        course.Guidance.push_back(group);
                        break;
                    case SessionType::OPTIONAL_COLLOQUIUM:
                        course.OptionalColloquium.push_back(group);
                        break;
                    case SessionType::REGISTRATION:
                        course.Registration.push_back(group);
                        break;
                    case SessionType::THESIS:
                        course.Thesis.push_back(group);
                        break;
                    case SessionType::PROJECT:
                        course.Project.push_back(group);
                        break;
                    case SessionType::UNSUPPORTED:
                        break;
                }
            }

            // Validate course before adding
            if (validateParsedCourse(course)) {
                courses.push_back(course);
                stats.validCourses++;
            } else {
                parsingWarnings.push_back("Course validation failed for: " + course.name + " (" + courseKey + ")");
            }
        }

        doc.close();

    } catch (const std::exception& e) {
        parsingWarnings.push_back("Excel parsing error: " + string(e.what()));
        std::cerr << "Failed to parse Excel file: " << e.what() << std::endl;
    }

    return courses;
}

// Course creation and management

void ExcelCourseParser::initializeCourse(Course& course, int courseId, const string& rawId,
                                         const string& courseName, const string& teacherName, int semester) {
    course.id = courseId;
    course.raw_id = rawId;
    course.name = courseName;
    course.teacher = teacherName;
    course.semester = semester;

    // Generate course_key immediately
    course.generateCourseKey();
}

// Course validation

bool ExcelCourseParser::validateParsedCourse(const Course& course) {
    // Check basic fields
    if (course.id <= 0 || course.raw_id.empty() || course.name.empty()) {
        return false;
    }

    // Check semester validity
    if (!course.hasValidSemester()) {
        return false;
    }

    // Check course_key format
    string expectedKey = to_string(course.id) + "_s" + to_string(course.semester);
    if (course.course_key != expectedKey) {
        return false;
    }

    // Check that course has at least one group with sessions
    bool hasValidGroups = false;
    vector<vector<Group>*> allGroups = {
            const_cast<vector<Group>*>(&course.Lectures),
            const_cast<vector<Group>*>(&course.Tirgulim),
            const_cast<vector<Group>*>(&course.labs),
            const_cast<vector<Group>*>(&course.blocks),
            const_cast<vector<Group>*>(&course.DepartmentalSessions),
            const_cast<vector<Group>*>(&course.Reinforcements),
            const_cast<vector<Group>*>(&course.Guidance),
            const_cast<vector<Group>*>(&course.OptionalColloquium),
            const_cast<vector<Group>*>(&course.Registration),
            const_cast<vector<Group>*>(&course.Thesis),
            const_cast<vector<Group>*>(&course.Project)
    };

    for (auto* groupVec : allGroups) {
        for (const auto& group : *groupVec) {
            if (!group.sessions.empty()) {
                hasValidGroups = true;
                break;
            }
        }
        if (hasValidGroups) break;
    }

    return hasValidGroups;
}

vector<string> ExcelCourseParser::getParsingWarnings() const {
    return parsingWarnings;
}

// Parsing helper methods

vector<string> ExcelCourseParser::parseMultipleRooms(const string& roomStr) {
    vector<string> rooms;

    if (roomStr.empty()) {
        rooms.push_back("");
        return rooms;
    }

    // Try splitting by newlines first
    istringstream lineStream(roomStr);
    string line;
    vector<string> linesCandidates;

    while (getline(lineStream, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (!line.empty()) {
            linesCandidates.push_back(line);
        }
    }

    if (linesCandidates.size() > 1) {
        return linesCandidates;
    }

    // Try regex for space-separated room patterns
    string text = roomStr;
    regex roomPattern(R"(([א-ת\w]+)[-\s](\d+)\s*-\s*(\d+))");
    sregex_iterator start(text.begin(), text.end(), roomPattern);
    sregex_iterator end;

    vector<string> spaceSeparatedRooms;
    for (sregex_iterator i = start; i != end; ++i) {
        smatch match = *i;
        string roomMatch = match.str();
        roomMatch.erase(0, roomMatch.find_first_not_of(" \t\r\n"));
        roomMatch.erase(roomMatch.find_last_not_of(" \t\r\n") + 1);
        spaceSeparatedRooms.push_back(roomMatch);
    }

    // Manual parsing for edge cases - look for digit + space + Hebrew letter
    if (spaceSeparatedRooms.size() <= 1) {
        vector<size_t> splitPoints;
        for (size_t i = 1; i < text.length() - 1; ++i) {
            char prev = text[i-1];
            char curr = text[i];
            char next = text[i+1];

            if (isdigit(prev) && curr == ' ' && (unsigned char)next >= 0xD7) {
                splitPoints.push_back(i);
            }
        }

        if (!splitPoints.empty()) {
            spaceSeparatedRooms.clear();
            size_t start = 0;

            for (size_t splitPoint : splitPoints) {
                string roomPart = text.substr(start, splitPoint - start);
                roomPart.erase(0, roomPart.find_first_not_of(" \t\r\n"));
                roomPart.erase(roomPart.find_last_not_of(" \t\r\n") + 1);

                if (!roomPart.empty()) {
                    spaceSeparatedRooms.push_back(roomPart);
                }
                start = splitPoint + 1;
            }

            string lastPart = text.substr(start);
            lastPart.erase(0, lastPart.find_first_not_of(" \t\r\n"));
            lastPart.erase(lastPart.find_last_not_of(" \t\r\n") + 1);

            if (!lastPart.empty()) {
                spaceSeparatedRooms.push_back(lastPart);
            }
        }
    }

    if (spaceSeparatedRooms.size() > 1) {
        return spaceSeparatedRooms;
    }

    if (spaceSeparatedRooms.size() == 1) {
        rooms.push_back(spaceSeparatedRooms[0]);
    } else {
        rooms.push_back(roomStr);
    }

    return rooms;
}

vector<Session> ExcelCourseParser::parseMultipleSessions(const string& timeSlotStr, const string& roomStr, const string& teacher) {
    vector<Session> sessions;

    if (timeSlotStr.empty()) return sessions;

    istringstream iss(timeSlotStr);
    string timeSlotToken;
    vector<string> timeSlots;

    while (iss >> timeSlotToken) {
        timeSlots.push_back(timeSlotToken);
    }

    vector<string> rooms = parseMultipleRooms(roomStr);

    for (size_t i = 0; i < timeSlots.size(); ++i) {
        string currentRoom = "";
        if (!rooms.empty()) {
            if (i < rooms.size()) {
                currentRoom = rooms[i];
            } else {
                currentRoom = rooms.back();
            }
        }

        Session session = parseSingleSession(timeSlots[i], currentRoom, teacher);
        if (session.day_of_week > 0) {
            sessions.push_back(session);
        }
    }

    return sessions;
}

Session ExcelCourseParser::parseSingleSession(const string& timeSlotStr, const string& roomStr, const string& teacher) {
    Session session;
    session.day_of_week = 0;
    session.start_time = "";
    session.end_time = "";
    session.building_number = "";
    session.room_number = "";

    if (timeSlotStr.empty()) return session;

    // Parse Hebrew day format like "א'10:00-12:00"
    size_t apostrophePos = timeSlotStr.find('\'');
    if (apostrophePos == string::npos || apostrophePos == 0) {
        return session;
    }

    string dayPart = timeSlotStr.substr(0, apostrophePos);

    if (dayPart == "א") session.day_of_week = 1;
    else if (dayPart == "ב") session.day_of_week = 2;
    else if (dayPart == "ג") session.day_of_week = 3;
    else if (dayPart == "ד") session.day_of_week = 4;
    else if (dayPart == "ה") session.day_of_week = 5;
    else if (dayPart == "ו") session.day_of_week = 6;
    else if (dayPart == "ש") session.day_of_week = 7;
    else {
        // Handle UTF-8 Hebrew characters
        if (dayPart.length() >= 2) {
            unsigned char byte1 = (unsigned char)dayPart[0];
            unsigned char byte2 = (unsigned char)dayPart[1];

            if (byte1 == 215 && byte2 == 144) session.day_of_week = 1;
            else if (byte1 == 215 && byte2 == 145) session.day_of_week = 2;
            else if (byte1 == 215 && byte2 == 146) session.day_of_week = 3;
            else if (byte1 == 215 && byte2 == 147) session.day_of_week = 4;
            else if (byte1 == 215 && byte2 == 148) session.day_of_week = 5;
            else if (byte1 == 215 && byte2 == 149) session.day_of_week = 6;
            else if (byte1 == 215 && byte2 == 169) session.day_of_week = 7;
        }
    }

    string timePart = timeSlotStr.substr(apostrophePos + 1);
    regex timePattern(R"((\d{1,2}:\d{2})-(\d{1,2}:\d{2}))");
    smatch timeMatch;

    if (regex_search(timePart, timeMatch, timePattern)) {
        session.start_time = timeMatch[1].str();
        session.end_time = timeMatch[2].str();
    }

    // Parse room format: supports both "הנדסה-1104 - 243" and "וואהל 1401 - 4"
    if (!roomStr.empty()) {
        size_t dashPos = roomStr.find(" - ");
        if (dashPos != string::npos) {
            string buildingPart = roomStr.substr(0, dashPos);
            string roomPart = roomStr.substr(dashPos + 3);
            session.room_number = roomPart;

            // Handle building name-number separation (dash or space)
            size_t buildingDashPos = buildingPart.find("-");
            if (buildingDashPos != string::npos) {
                string buildingName = buildingPart.substr(0, buildingDashPos);
                string buildingNumber = buildingPart.substr(buildingDashPos + 1);
                session.building_number = buildingName + " " + buildingNumber;
            } else {
                size_t buildingSpacePos = buildingPart.find_last_of(" ");
                if (buildingSpacePos != string::npos) {
                    string buildingName = buildingPart.substr(0, buildingSpacePos);
                    string buildingNumber = buildingPart.substr(buildingSpacePos + 1);
                    session.building_number = buildingName + " " + buildingNumber;
                } else {
                    session.building_number = buildingPart;
                }
            }
        } else {
            // No room number format
            size_t dashPos2 = roomStr.find("-");
            if (dashPos2 != string::npos) {
                string buildingName = roomStr.substr(0, dashPos2);
                string buildingNumber = roomStr.substr(dashPos2 + 1);
                session.building_number = buildingName + " " + buildingNumber;
                session.room_number = "";
            } else {
                size_t spacePos = roomStr.find_last_of(" ");
                if (spacePos != string::npos && spacePos > 0) {
                    string buildingName = roomStr.substr(0, spacePos);
                    string buildingNumber = roomStr.substr(spacePos + 1);

                    if (!buildingNumber.empty() && isdigit(buildingNumber[0])) {
                        session.building_number = buildingName + " " + buildingNumber;
                        session.room_number = "";
                    } else {
                        session.building_number = roomStr;
                        session.room_number = "";
                    }
                } else {
                    session.building_number = roomStr;
                    session.room_number = "";
                }
            }
        }
    }

    return session;
}

void ExcelCourseParser::initializeSessionTypeMap() {
    sessionTypeMap = {
            {"הרצאה",          SessionType::LECTURE},
            {"תרגיל",          SessionType::TUTORIAL},
            {"מעבדה",          SessionType::LAB},
            {"ש.מחלקה",        SessionType::DEPARTMENTAL_SESSION},
            {"תגבור",          SessionType::REINFORCEMENT},
            {"הדרכה",          SessionType::GUIDANCE},
            {"קולוקויום רשות", SessionType::OPTIONAL_COLLOQUIUM},
            {"רישום",          SessionType::REGISTRATION},
            {"תיזה",           SessionType::THESIS},
            {"פרויקט",         SessionType::PROJECT},
            {"בלוק",           SessionType::BLOCK}  // Added block support
    };
}

SessionType ExcelCourseParser::getSessionType(const string& hebrewType) {
    if (sessionTypeMap.count(hebrewType)) {
        return sessionTypeMap[hebrewType];
    }
    return SessionType::LECTURE;
}

bool ExcelCourseParser::isValidSessionType(const string& sessionType) {
    return sessionTypeMap.find(sessionType) != sessionTypeMap.end();
}

string ExcelCourseParser::sessionTypeToString(SessionType type) {
    switch (type) {
        case SessionType::LECTURE: return "lecture";
        case SessionType::TUTORIAL: return "tutorial";
        case SessionType::LAB: return "lab";
        case SessionType::BLOCK: return "block";
        case SessionType::DEPARTMENTAL_SESSION: return "departmental_session";
        case SessionType::REINFORCEMENT: return "reinforcement";
        case SessionType::GUIDANCE: return "guidance";
        case SessionType::OPTIONAL_COLLOQUIUM: return "optional_colloquium";
        case SessionType::REGISTRATION: return "registration";
        case SessionType::THESIS: return "thesis";
        case SessionType::PROJECT: return "project";
        case SessionType::UNSUPPORTED: return "unsupported";
        default: return "unsupported";
    }
}

int ExcelCourseParser::getSemesterNumber(const string& period) {
    // Trim whitespace and convert to lowercase for comparison
    string trimmedPeriod = period;
    trimmedPeriod.erase(0, trimmedPeriod.find_first_not_of(" \t\r\n"));
    trimmedPeriod.erase(trimmedPeriod.find_last_not_of(" \t\r\n") + 1);

    // Hebrew semester parsing
    if (trimmedPeriod == "סמסטר א'" || trimmedPeriod == "סמסטר א" ||
        trimmedPeriod.find("סמסטר א") != string::npos) {
        return 1; // Semester A
    } else if (trimmedPeriod == "סמסטר ב'" || trimmedPeriod == "סמסטר ב" ||
               trimmedPeriod.find("סמסטר ב") != string::npos) {
        return 2; // Semester B
    } else if (trimmedPeriod == "סמסטר קיץ" || trimmedPeriod == "קיץ" ||
               trimmedPeriod.find("קיץ") != string::npos) {
        return 3; // Summer Semester
    } else if (trimmedPeriod == "שנתי" || trimmedPeriod.find("שנתי") != string::npos) {
        return 4; // Yearly course
    }

    // English fallbacks
    string lowerPeriod = trimmedPeriod;
    transform(lowerPeriod.begin(), lowerPeriod.end(), lowerPeriod.begin(), ::tolower);

    if (lowerPeriod.find("semester a") != string::npos || lowerPeriod.find("fall") != string::npos) {
        return 1;
    } else if (lowerPeriod.find("semester b") != string::npos || lowerPeriod.find("spring") != string::npos) {
        return 2;
    } else if (lowerPeriod.find("summer") != string::npos) {
        return 3;
    } else if (lowerPeriod.find("yearly") != string::npos || lowerPeriod.find("annual") != string::npos) {
        return 4;
    }

    parsingWarnings.push_back("Unknown semester format: " + period + " - defaulting to Semester A");
    return 1; // Default to Semester A if unknown
}

string ExcelCourseParser::getSemesterName(int semesterNumber) {
    switch (semesterNumber) {
        case 1: return "Semester A";
        case 2: return "Semester B";
        case 3: return "Summer";
        case 4: return "Year-long";
        default: return "Unknown";
    }
}

bool ExcelCourseParser::isValidSemester(int semesterNumber) {
    return semesterNumber >= 1 && semesterNumber <= 4;
}

pair<string, string> ExcelCourseParser::parseCourseCode(const string& fullCode) {
    size_t dashPos = fullCode.find('-');
    if (dashPos != string::npos && dashPos > 0) {
        string courseCode = fullCode.substr(0, dashPos);
        string groupCode = fullCode.substr(dashPos + 1);

        // Ensure courseCode is exactly 5 digits
        if (courseCode.length() == 5) {
            return {courseCode, groupCode};
        }
    }

    // If no dash found or invalid format, try to extract first 5 characters if they're digits
    if (fullCode.length() >= 5) {
        string potentialCode = fullCode.substr(0, 5);
        // Check if all characters are digits
        bool allDigits = true;
        for (char c : potentialCode) {
            if (!isdigit(c)) {
                allDigits = false;
                break;
            }
        }
        if (allDigits) {
            return {potentialCode, "01"};
        }
    }

    return {fullCode, "01"};
}

// Utility functions to translate hebrew

string getDayName(int dayOfWeek) {
    switch(dayOfWeek) {
        case 1: return "ראשון";
        case 2: return "שני";
        case 3: return "שלישי";
        case 4: return "רביעי";
        case 5: return "חמישי";
        case 6: return "שישי";
        case 7: return "שבת";
        default: return "לא ידוע";
    }
}

namespace SemesterUtils {
    string getHebrewSemesterName(int semester) {
        switch (semester) {
            case 1: return "סמסטר א'";
            case 2: return "סמסטר ב'";
            case 3: return "סמסטר קיץ";
            case 4: return "שנתי";
            default: return "לא ידוע";
        }
    }

    string getEnglishSemesterName(int semester) {
        switch (semester) {
            case 1: return "Semester A";
            case 2: return "Semester B";
            case 3: return "Summer";
            case 4: return "Year-long";
            default: return "Unknown";
        }
    }

    bool isValidAcademicSemester(int semester) {
        return semester >= 1 && semester <= 4;
    }
}