#include "db_json_helpers.h"

// Group JSON Conversion Methods

string DatabaseJsonHelpers::groupsToJson(const vector<Group>& groups) {
    QJsonArray groupsArray;

    for (const auto& group : groups) {
        groupsArray.append(groupToJsonObject(group));
    }

    QJsonDocument doc(groupsArray);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

vector<Group> DatabaseJsonHelpers::groupsFromJson(const string& json) {
    vector<Group> groups;

    if (json.empty() || json == "[]") {
        return groups;
    }

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isArray()) {
        return groups;
    }

    QJsonArray groupsArray = doc.array();
    for (const auto& value : groupsArray) {
        if (value.isObject()) {
            groups.push_back(groupFromJsonObject(value.toObject()));
        }
    }

    return groups;
}

// Enhanced Schedule JSON Conversion

string DatabaseJsonHelpers::scheduleToJson(const InformativeSchedule& schedule) {
    QJsonObject scheduleObj;

    // Basic information
    scheduleObj["index"] = schedule.index;
    scheduleObj["semester"] = schedule.semester;

    // Convert week schedule
    QJsonArray weekArray;
    for (const auto& day : schedule.week) {
        weekArray.append(scheduleDayToJsonObject(day));
    }
    scheduleObj["week"] = weekArray;

    // Store all metadata (for potential future use in JSON queries)
    QJsonObject metadata;
    metadata["amount_days"] = schedule.amount_days;
    metadata["amount_gaps"] = schedule.amount_gaps;
    metadata["gaps_time"] = schedule.gaps_time;
    metadata["avg_start"] = schedule.avg_start;
    metadata["avg_end"] = schedule.avg_end;
    metadata["earliest_start"] = schedule.earliest_start;
    metadata["latest_end"] = schedule.latest_end;
    metadata["longest_gap"] = schedule.longest_gap;
    metadata["total_class_time"] = schedule.total_class_time;
    metadata["consecutive_days"] = schedule.consecutive_days;
    metadata["days_json"] = QString::fromStdString(schedule.days_json);
    metadata["weekend_classes"] = schedule.weekend_classes;
    metadata["has_morning_classes"] = schedule.has_morning_classes;
    metadata["has_early_morning"] = schedule.has_early_morning;
    metadata["has_evening_classes"] = schedule.has_evening_classes;
    metadata["has_late_evening"] = schedule.has_late_evening;
    metadata["max_daily_hours"] = schedule.max_daily_hours;
    metadata["min_daily_hours"] = schedule.min_daily_hours;
    metadata["avg_daily_hours"] = schedule.avg_daily_hours;
    metadata["has_lunch_break"] = schedule.has_lunch_break;
    metadata["max_daily_gaps"] = schedule.max_daily_gaps;
    metadata["avg_gap_length"] = schedule.avg_gap_length;
    metadata["schedule_span"] = schedule.schedule_span;
    metadata["compactness_ratio"] = schedule.compactness_ratio;
    metadata["weekday_only"] = schedule.weekday_only;
    metadata["has_monday"] = schedule.has_monday;
    metadata["has_tuesday"] = schedule.has_tuesday;
    metadata["has_wednesday"] = schedule.has_wednesday;
    metadata["has_thursday"] = schedule.has_thursday;
    metadata["has_friday"] = schedule.has_friday;
    metadata["has_saturday"] = schedule.has_saturday;
    metadata["has_sunday"] = schedule.has_sunday;

    scheduleObj["metadata"] = metadata;

    QJsonDocument doc(scheduleObj);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

InformativeSchedule DatabaseJsonHelpers::scheduleFromJson(const string& json, int id, int schedule_index,
                                                          int amount_days, int amount_gaps, int gaps_time,
                                                          int avg_start, int avg_end) {
    InformativeSchedule schedule;

    // Set basic metrics from parameters (these come from database columns)
    schedule.index = schedule_index;
    schedule.amount_days = amount_days;
    schedule.amount_gaps = amount_gaps;
    schedule.gaps_time = gaps_time;
    schedule.avg_start = avg_start;
    schedule.avg_end = avg_end;

    if (json.empty()) {
        return schedule;
    }

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isObject()) {
        return schedule;
    }

    QJsonObject scheduleObj = doc.object();

    // Read semester if available in JSON
    if (scheduleObj.contains("semester")) {
        schedule.semester = scheduleObj["semester"].toInt();
    }

    // Parse the week schedule
    if (scheduleObj.contains("week") && scheduleObj["week"].isArray()) {
        QJsonArray weekArray = scheduleObj["week"].toArray();
        for (const auto& value : weekArray) {
            if (value.isObject()) {
                schedule.week.push_back(scheduleDayFromJsonObject(value.toObject()));
            }
        }
    }

    // Read metadata if available (for backward compatibility or JSON-based queries)
    if (scheduleObj.contains("metadata")) {
        QJsonObject metadata = scheduleObj["metadata"].toObject();

        // Only read from JSON if not already set from database columns
        if (schedule.earliest_start == 0 && metadata.contains("earliest_start")) {
            schedule.earliest_start = metadata["earliest_start"].toInt();
        }
        if (schedule.latest_end == 0 && metadata.contains("latest_end")) {
            schedule.latest_end = metadata["latest_end"].toInt();
        }
        // Continue for other metadata fields as needed...
    }

    return schedule;
}

// Course JSON Conversion (for complete serialization if needed)

string DatabaseJsonHelpers::courseToJson(const Course& course) {
    QJsonObject courseObj;

    // Basic course information
    courseObj["id"] = course.id;
    courseObj["raw_id"] = QString::fromStdString(course.raw_id);
    courseObj["name"] = QString::fromStdString(course.name);
    courseObj["teacher"] = QString::fromStdString(course.teacher);
    courseObj["semester"] = course.semester;
    courseObj["uniqid"] = QString::fromStdString(course.uniqid);
    courseObj["course_key"] = QString::fromStdString(course.course_key);

    // Serialize all group types
    courseObj["lectures"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Lectures))).array();
    courseObj["tutorials"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Tirgulim))).array();
    courseObj["labs"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.labs))).array();
    courseObj["blocks"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.blocks))).array();
    courseObj["departmental_sessions"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.DepartmentalSessions))).array();
    courseObj["reinforcements"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Reinforcements))).array();
    courseObj["guidance"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Guidance))).array();
    courseObj["optional_colloquium"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.OptionalColloquium))).array();
    courseObj["registration"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Registration))).array();
    courseObj["thesis"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Thesis))).array();
    courseObj["project"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Project))).array();

    QJsonDocument doc(courseObj);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

Course DatabaseJsonHelpers::courseFromJson(const string& json, const string& uniqid, int course_id,
                                           const string& raw_id, const string& name, const string& teacher,
                                           int semester, const string& course_key) {
    Course course;

    // Set basic fields from parameters (these come from database columns)
    course.uniqid = uniqid;
    course.id = course_id;
    course.raw_id = raw_id;
    course.name = name;
    course.teacher = teacher;
    course.semester = semester;
    course.course_key = course_key;

    if (json.empty()) {
        return course;
    }

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isObject()) {
        return course;
    }

    QJsonObject courseObj = doc.object();

    // Parse all group types from JSON
    if (courseObj.contains("lectures")) {
        QJsonDocument lecturesDoc(courseObj["lectures"].toArray());
        course.Lectures = groupsFromJson(lecturesDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("tutorials")) {
        QJsonDocument tutorialsDoc(courseObj["tutorials"].toArray());
        course.Tirgulim = groupsFromJson(tutorialsDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("labs")) {
        QJsonDocument labsDoc(courseObj["labs"].toArray());
        course.labs = groupsFromJson(labsDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("blocks")) {
        QJsonDocument blocksDoc(courseObj["blocks"].toArray());
        course.blocks = groupsFromJson(blocksDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("departmental_sessions")) {
        QJsonDocument deptDoc(courseObj["departmental_sessions"].toArray());
        course.DepartmentalSessions = groupsFromJson(deptDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("reinforcements")) {
        QJsonDocument reinforcementsDoc(courseObj["reinforcements"].toArray());
        course.Reinforcements = groupsFromJson(reinforcementsDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("guidance")) {
        QJsonDocument guidanceDoc(courseObj["guidance"].toArray());
        course.Guidance = groupsFromJson(guidanceDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("optional_colloquium")) {
        QJsonDocument colloquiumDoc(courseObj["optional_colloquium"].toArray());
        course.OptionalColloquium = groupsFromJson(colloquiumDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("registration")) {
        QJsonDocument registrationDoc(courseObj["registration"].toArray());
        course.Registration = groupsFromJson(registrationDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("thesis")) {
        QJsonDocument thesisDoc(courseObj["thesis"].toArray());
        course.Thesis = groupsFromJson(thesisDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("project")) {
        QJsonDocument projectDoc(courseObj["project"].toArray());
        course.Project = groupsFromJson(projectDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    return course;
}

// Session Type Conversion Utilities

string DatabaseJsonHelpers::sessionTypeToString(SessionType type) {
    switch (type) {
        case SessionType::LECTURE: return "LECTURE";
        case SessionType::TUTORIAL: return "TUTORIAL";
        case SessionType::LAB: return "LAB";
        case SessionType::BLOCK: return "BLOCK";
        case SessionType::DEPARTMENTAL_SESSION: return "DEPARTMENTAL_SESSION";
        case SessionType::REINFORCEMENT: return "REINFORCEMENT";
        case SessionType::GUIDANCE: return "GUIDANCE";
        case SessionType::OPTIONAL_COLLOQUIUM: return "OPTIONAL_COLLOQUIUM";
        case SessionType::REGISTRATION: return "REGISTRATION";
        case SessionType::THESIS: return "THESIS";
        case SessionType::PROJECT: return "PROJECT";
        case SessionType::UNSUPPORTED: return "UNSUPPORTED";
        default: return "UNSUPPORTED";
    }
}

SessionType DatabaseJsonHelpers::sessionTypeFromString(const string& typeStr) {
    if (typeStr == "LECTURE") return SessionType::LECTURE;
    else if (typeStr == "TUTORIAL") return SessionType::TUTORIAL;
    else if (typeStr == "LAB") return SessionType::LAB;
    else if (typeStr == "BLOCK") return SessionType::BLOCK;
    else if (typeStr == "DEPARTMENTAL_SESSION") return SessionType::DEPARTMENTAL_SESSION;
    else if (typeStr == "REINFORCEMENT") return SessionType::REINFORCEMENT;
    else if (typeStr == "GUIDANCE") return SessionType::GUIDANCE;
    else if (typeStr == "OPTIONAL_COLLOQUIUM") return SessionType::OPTIONAL_COLLOQUIUM;
    else if (typeStr == "REGISTRATION") return SessionType::REGISTRATION;
    else if (typeStr == "THESIS") return SessionType::THESIS;
    else if (typeStr == "PROJECT") return SessionType::PROJECT;
    else return SessionType::UNSUPPORTED;
}

// Validation Helpers

bool DatabaseJsonHelpers::isValidJson(const string& json) {
    if (json.empty()) return true; // Empty is valid (will be treated as default)

    QJsonParseError error;
    QJsonDocument::fromJson(QByteArray::fromStdString(json), &error);
    return error.error == QJsonParseError::NoError;
}

bool DatabaseJsonHelpers::validateGroupsJson(const string& json) {
    if (!isValidJson(json)) return false;
    if (json.empty() || json == "[]") return true;

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    return doc.isArray();
}

bool DatabaseJsonHelpers::validateScheduleJson(const string& json) {
    if (!isValidJson(json)) return false;
    if (json.empty()) return false; // Schedule JSON should not be empty

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isObject()) return false;

    QJsonObject obj = doc.object();
    return obj.contains("week") && obj["week"].isArray();
}

// Private Helper Methods for JSON Object Creation

QJsonObject DatabaseJsonHelpers::sessionToJsonObject(const Session& session) {
    QJsonObject sessionObj;
    sessionObj["day_of_week"] = session.day_of_week;
    sessionObj["start_time"] = QString::fromStdString(session.start_time);
    sessionObj["end_time"] = QString::fromStdString(session.end_time);
    sessionObj["building_number"] = QString::fromStdString(session.building_number);
    sessionObj["room_number"] = QString::fromStdString(session.room_number);
    return sessionObj;
}

Session DatabaseJsonHelpers::sessionFromJsonObject(const QJsonObject& obj) {
    Session session;
    session.day_of_week = obj["day_of_week"].toInt();
    session.start_time = obj["start_time"].toString().toStdString();
    session.end_time = obj["end_time"].toString().toStdString();
    session.building_number = obj["building_number"].toString().toStdString();
    session.room_number = obj["room_number"].toString().toStdString();
    return session;
}

QJsonObject DatabaseJsonHelpers::groupToJsonObject(const Group& group) {
    QJsonObject groupObj;
    groupObj["type"] = QString::fromStdString(sessionTypeToString(group.type));

    QJsonArray sessionsArray;
    for (const auto& session : group.sessions) {
        sessionsArray.append(sessionToJsonObject(session));
    }
    groupObj["sessions"] = sessionsArray;

    return groupObj;
}

Group DatabaseJsonHelpers::groupFromJsonObject(const QJsonObject& obj) {
    Group group;
    group.type = sessionTypeFromString(obj["type"].toString().toStdString());

    if (obj.contains("sessions") && obj["sessions"].isArray()) {
        QJsonArray sessionsArray = obj["sessions"].toArray();
        for (const auto& sessionValue : sessionsArray) {
            if (sessionValue.isObject()) {
                group.sessions.push_back(sessionFromJsonObject(sessionValue.toObject()));
            }
        }
    }

    return group;
}

QJsonObject DatabaseJsonHelpers::scheduleItemToJsonObject(const ScheduleItem& item) {
    QJsonObject itemObj;
    itemObj["courseName"] = QString::fromStdString(item.courseName);
    itemObj["raw_id"] = QString::fromStdString(item.raw_id);
    itemObj["type"] = QString::fromStdString(item.type);
    itemObj["start"] = QString::fromStdString(item.start);
    itemObj["end"] = QString::fromStdString(item.end);
    itemObj["building"] = QString::fromStdString(item.building);
    itemObj["room"] = QString::fromStdString(item.room);
    return itemObj;
}

ScheduleItem DatabaseJsonHelpers::scheduleItemFromJsonObject(const QJsonObject& obj) {
    ScheduleItem item;
    item.courseName = obj["courseName"].toString().toStdString();
    item.raw_id = obj["raw_id"].toString().toStdString();
    item.type = obj["type"].toString().toStdString();
    item.start = obj["start"].toString().toStdString();
    item.end = obj["end"].toString().toStdString();
    item.building = obj["building"].toString().toStdString();
    item.room = obj["room"].toString().toStdString();
    return item;
}

QJsonObject DatabaseJsonHelpers::scheduleDayToJsonObject(const ScheduleDay& day) {
    QJsonObject dayObj;
    dayObj["day"] = QString::fromStdString(day.day);

    QJsonArray itemsArray;
    for (const auto& item : day.day_items) {
        itemsArray.append(scheduleItemToJsonObject(item));
    }
    dayObj["day_items"] = itemsArray;

    return dayObj;
}

ScheduleDay DatabaseJsonHelpers::scheduleDayFromJsonObject(const QJsonObject& obj) {
    ScheduleDay day;
    day.day = obj["day"].toString().toStdString();

    if (obj.contains("day_items") && obj["day_items"].isArray()) {
        QJsonArray itemsArray = obj["day_items"].toArray();
        for (const auto& itemValue : itemsArray) {
            if (itemValue.isObject()) {
                day.day_items.push_back(scheduleItemFromJsonObject(itemValue.toObject()));
            }
        }
    }

    return day;
}