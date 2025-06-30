#include "excel_parser.h"

// Constructor implementation
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
            {"פרויקט",         SessionType::PROJECT}
    };
}

// Helper method to determine semester number from Hebrew period string
int ExcelCourseParser::getSemesterNumber(const string& period) {
    if (period == "סמסטר א'") {
        return 1; // Semester A
    } else if (period == "סמסטר ב'") {
        return 2; // Semester B
    } else if (period == "סמסטר קיץ") {
        return 3; // Summer Semester
    } else if (period == "שנתי") {
        return 4; // Yearly course
    }
    return 0; // Unknown/unsupported semester
}

// Parse multiple rooms from single cell - handles both newlines and space-separated rooms
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

// Parse multiple time slots and match each with corresponding room
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

SessionType ExcelCourseParser::getSessionType(const string& hebrewType) {
    if (sessionTypeMap.count(hebrewType)) {
        return sessionTypeMap[hebrewType];
    }
    return SessionType::LECTURE;
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

vector<Course> ExcelCourseParser::parseExcelFile(const string& filename) {
    vector<Course> courses;
    // Use courseCode + semester as key to create separate courses
    map<string, Course> courseMap; // Key: courseCode_semester
    map<string, map<string, Group>> courseGroupMap; // Key: courseCode_semester

    try {
        XLDocument doc;
        doc.open(filename);
        auto worksheet = doc.workbook().worksheet(1);

        for (uint32_t row = 2; row <= 10000; ++row) {
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

            // Get semester number and filter for supported semesters
            int semesterNumber = getSemesterNumber(period);
            if (semesterNumber == 0) {
                continue; // Skip unsupported semester types
            }

            auto [courseCode, groupCode] = parseCourseCode(fullCode);
            if (courseCode.empty()) continue;

            SessionType normalizedSessionType = getSessionType(sessionType);
            if (normalizedSessionType == SessionType::UNSUPPORTED) {
                continue;
            }

            string normalizedSessionTypeName;
            switch (normalizedSessionType) {
                case SessionType::LECTURE:
                    normalizedSessionTypeName = "lecture";
                    break;
                case SessionType::TUTORIAL:
                    normalizedSessionTypeName = "tutorial";
                    break;
                case SessionType::LAB:
                    normalizedSessionTypeName = "lab";
                    break;
                case SessionType::BLOCK:
                    normalizedSessionTypeName = "block";
                    break;
                case SessionType::DEPARTMENTAL_SESSION:
                    normalizedSessionTypeName = "departmental_session";
                    break;
                case SessionType::REINFORCEMENT:
                    normalizedSessionTypeName = "reinforcement";
                    break;
                case SessionType::GUIDANCE:
                    normalizedSessionTypeName = "guidance";
                    break;
                case SessionType::OPTIONAL_COLLOQUIUM:
                    normalizedSessionTypeName = "optional_colloquium";
                    break;
                case SessionType::REGISTRATION:
                    normalizedSessionTypeName = "registration";
                    break;
                case SessionType::THESIS:
                    normalizedSessionTypeName = "thesis";
                    break;
                case SessionType::PROJECT:
                    normalizedSessionTypeName = "project";
                    break;
                case SessionType::UNSUPPORTED:
                    normalizedSessionTypeName = "unsupported";
                    break;
            }

            if (timeSlot.empty() || timeSlot.find("'") == string::npos) {
                continue; // Skip rows without valid time slots
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
                continue;
            }

            // Create unique course key: courseCode + semester
            string courseKey = courseCode + "_sem" + to_string(semesterNumber);

            if (courseMap.find(courseKey) == courseMap.end()) {
                Course newCourse;
                try {
                    newCourse.id = stoi(courseCode);
                } catch (...) {
                    newCourse.id = 0;
                }
                newCourse.raw_id = courseCode;
                newCourse.name = courseName;
                newCourse.teacher = teachers;
                newCourse.semester = semesterNumber; // Each course gets its specific semester
                courseMap[courseKey] = newCourse;
            }

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

        // Create final courses list
        for (auto &[courseKey, course] : courseMap) {
            for (auto &[groupKey, group] : courseGroupMap[courseKey]) {
                if (group.sessions.empty()) continue;

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

            courses.push_back(course);
        }

        doc.close();
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse Excel file: " << e.what() << std::endl;
    }

    return courses;
}

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